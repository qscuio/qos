use std::collections::BTreeMap;
use std::io::{self, BufRead, Write};
use qos_kernel::QOS_NET_IPV4_LOCAL;
use qos_userspace::net::{ensure_kernel_booted, fmt_ipv4, parse_ipv4, ping_via_syscall_raw, wget_via_syscall_stream};

const PATH_MAX: usize = 128;
const EXEC_PATHS: &[(&str, &str)] = &[
    ("/bin/ls", "ls"),
    ("/bin/cat", "cat"),
    ("/bin/echo", "echo"),
    ("/bin/mkdir", "mkdir"),
    ("/bin/rm", "rm"),
    ("/bin/touch", "touch"),
    ("/bin/edit", "edit"),
    ("/bin/ps", "ps"),
    ("/bin/ping", "ping"),
    ("/bin/ip", "ip"),
    ("/bin/wget", "wget"),
    ("/usr/bin/ls", "ls"),
    ("/usr/bin/cat", "cat"),
    ("/usr/bin/echo", "echo"),
    ("/usr/bin/mkdir", "mkdir"),
    ("/usr/bin/rm", "rm"),
    ("/usr/bin/touch", "touch"),
    ("/usr/bin/edit", "edit"),
    ("/usr/bin/ps", "ps"),
    ("/usr/bin/ping", "ping"),
    ("/usr/bin/ip", "ip"),
    ("/usr/bin/wget", "wget"),
];

#[derive(Default)]
struct ShellState {
    paths: Vec<String>,
    files: BTreeMap<String, String>,
    env: BTreeMap<String, String>,
    cwd: String,
}

impl ShellState {
    fn new() -> Self {
        let mut env = BTreeMap::new();
        env.insert("PATH".to_string(), "/bin:/usr/bin".to_string());
        Self {
            paths: vec!["/tmp".to_string(), "/proc".to_string(), "/data".to_string()],
            files: BTreeMap::new(),
            env,
            cwd: "/".to_string(),
        }
    }

    fn add_path(&mut self, path: &str) -> bool {
        if !path.starts_with('/') || path.len() >= PATH_MAX || self.paths.iter().any(|p| p == path) {
            return false;
        }
        self.paths.push(path.to_string());
        true
    }

    fn remove_path(&mut self, path: &str) -> bool {
        if let Some(idx) = self.paths.iter().position(|p| p == path) {
            self.paths.remove(idx);
            self.files.remove(path);
            return true;
        }
        false
    }

    fn path_exists(&self, path: &str) -> bool {
        path == "/" || self.paths.iter().any(|p| p == path)
    }
}

fn is_builtin(cmd: &str) -> bool {
    matches!(cmd, "help" | "exit" | "echo" | "cd" | "pwd" | "export" | "unset" | "ip")
}

fn resolve_exec_path(path: &str) -> Option<&'static str> {
    for (p, cmd) in EXEC_PATHS {
        if path == *p {
            return Some(*cmd);
        }
    }
    None
}

fn resolve_external_from_path(state: &ShellState, cmd: &str) -> Option<&'static str> {
    let path = state.env.get("PATH")?;
    for seg in path.split(':') {
        if seg.is_empty() {
            continue;
        }
        let candidate = if seg.ends_with('/') {
            format!("{seg}{cmd}")
        } else {
            format!("{seg}/{cmd}")
        };
        if let Some(resolved) = resolve_exec_path(&candidate) {
            return Some(resolved);
        }
    }
    None
}

fn resolve_command_name(state: &ShellState, raw: &str) -> Option<String> {
    if raw.is_empty() {
        return None;
    }
    if is_builtin(raw) {
        return Some(raw.to_string());
    }
    if raw.contains('/') {
        return resolve_exec_path(raw).map(|s| s.to_string());
    }
    resolve_external_from_path(state, raw).map(|s| s.to_string())
}

fn tokenize(line: &str) -> Vec<String> {
    let chars: Vec<char> = line.chars().collect();
    let mut tokens = Vec::new();
    let mut i = 0usize;

    while i < chars.len() {
        while i < chars.len() && chars[i].is_ascii_whitespace() {
            i += 1;
        }
        if i >= chars.len() {
            break;
        }
        match chars[i] {
            '|' | '<' => {
                tokens.push(chars[i].to_string());
                i += 1;
            }
            '>' => {
                if i + 1 < chars.len() && chars[i + 1] == '>' {
                    tokens.push(">>".to_string());
                    i += 2;
                } else {
                    tokens.push(">".to_string());
                    i += 1;
                }
            }
            '\'' | '"' => {
                let quote = chars[i];
                i += 1;
                let mut tok = String::new();
                while i < chars.len() && chars[i] != quote {
                    tok.push(chars[i]);
                    i += 1;
                }
                if i < chars.len() && chars[i] == quote {
                    i += 1;
                }
                tokens.push(tok);
            }
            _ => {
                let mut tok = String::new();
                while i < chars.len()
                    && !chars[i].is_ascii_whitespace()
                    && chars[i] != '|'
                    && chars[i] != '<'
                    && chars[i] != '>'
                {
                    tok.push(chars[i]);
                    i += 1;
                }
                tokens.push(tok);
            }
        }
    }

    tokens
}

fn parse_proc_status_pid(path: &str) -> Option<u32> {
    if !path.starts_with("/proc/") || !path.ends_with("/status") {
        return None;
    }
    let pid_str = &path[6..path.len().saturating_sub(7)];
    if pid_str.is_empty() {
        return None;
    }
    let mut pid = 0u32;
    for b in pid_str.bytes() {
        if !b.is_ascii_digit() {
            return None;
        }
        pid = pid.saturating_mul(10).saturating_add((b - b'0') as u32);
    }
    if pid == 0 { None } else { Some(pid) }
}

fn print_help() -> String {
    [
        "qos-sh commands:",
        "  help",
        "  ls [path]",
        "  cat <path>",
        "  echo <text>",
        "  mkdir <path>",
        "  rm <path>",
        "  touch <path>",
        "  edit <path> [text]",
        "  cd <path>",
        "  pwd",
        "  export KEY=VAL",
        "  unset KEY",
        "  ps",
        "  ping <ip>",
        "  ip addr",
        "  ip route",
        "  wget <url>",
        "  exit",
        "",
    ]
    .join("\n")
}

fn execute_segment(state: &mut ShellState, tokens: &[String], input: Option<&str>) -> (String, bool) {
    let mut args: Vec<String> = Vec::new();
    let mut redir_in: Option<String> = None;
    let mut redir_out: Option<(String, bool)> = None;
    let mut i = 0usize;
    while i < tokens.len() {
        match tokens[i].as_str() {
            "<" | ">" | ">>" if i + 1 < tokens.len() => {
                if tokens[i] == "<" {
                    redir_in = Some(tokens[i + 1].clone());
                } else {
                    redir_out = Some((tokens[i + 1].clone(), tokens[i] == ">>"));
                }
                i += 2;
            }
            _ => {
                args.push(tokens[i].clone());
                i += 1;
            }
        }
    }

    if args.is_empty() {
        return (String::new(), false);
    }

    let mut input_data = input.unwrap_or("").to_string();
    if let Some(path) = redir_in {
        input_data = state.files.get(&path).cloned().unwrap_or_default();
    }

    let cmd = match resolve_command_name(state, &args[0]) {
        Some(c) => c,
        None => {
            return (format!("unknown command: {}\n", args[0]), false);
        }
    };
    let mut out = String::new();
    let mut should_exit = false;

    match cmd.as_str() {
        "help" => out.push_str(&print_help()),
        "exit" => should_exit = true,
        "ls" => {
            out.push_str("bin\n");
            for p in &state.paths {
                out.push_str(p);
                out.push('\n');
            }
        }
        "cat" => {
            if args.len() < 2 {
                if !input_data.is_empty() {
                    out.push_str(&input_data);
                } else {
                    out.push_str("cat: missing path\n");
                }
            } else if args[1] == "/proc/meminfo" {
                out.push_str("MemTotal:\t16384 kB\nMemFree:\t8192 kB\nProcCount:\t1\n");
            } else if let Some(pid) = parse_proc_status_pid(&args[1]) {
                out.push_str(&format!("Pid:\t{pid}\nState:\tRunning\n"));
            } else if let Some(data) = state.files.get(&args[1]) {
                out.push_str(data);
            } else if state.path_exists(&args[1]) {
                out.push_str(&args[1]);
                out.push('\n');
            } else {
                out.push_str(&format!("cat: not found: {}\n", args[1]));
            }
        }
        "echo" => {
            if args.len() > 1 {
                out.push_str(&args[1..].join(" "));
                out.push('\n');
            } else if !input_data.is_empty() {
                out.push_str(&input_data);
            } else {
                out.push('\n');
            }
        }
        "mkdir" => {
            if let Some(path) = args.get(1) {
                if state.add_path(path) {
                    out.push_str(&format!("created {path}\n"));
                } else {
                    out.push_str(&format!("mkdir: failed {path}\n"));
                }
            } else {
                out.push_str("mkdir: failed (null)\n");
            }
        }
        "rm" => {
            if let Some(path) = args.get(1) {
                if state.remove_path(path) {
                    out.push_str(&format!("removed {path}\n"));
                } else {
                    out.push_str(&format!("rm: failed {path}\n"));
                }
            } else {
                out.push_str("rm: failed (null)\n");
            }
        }
        "touch" => {
            if let Some(path) = args.get(1) {
                if path.starts_with('/') {
                    state.files.entry(path.clone()).or_default();
                    out.push_str(&format!("created file {path}\n"));
                } else {
                    out.push_str(&format!("touch: failed {path}\n"));
                }
            } else {
                out.push_str("touch: failed (null)\n");
            }
        }
        "edit" => {
            if let Some(path) = args.get(1) {
                if path.starts_with('/') {
                    let content = if args.len() > 2 {
                        args[2..].join(" ")
                    } else {
                        input_data.clone()
                    };
                    state.files.insert(path.clone(), content);
                    out.push_str(&format!("edited {path}\n"));
                } else {
                    out.push_str(&format!("edit: failed {path}\n"));
                }
            } else {
                out.push_str("edit: failed (null)\n");
            }
        }
        "cd" => {
            if let Some(path) = args.get(1) {
                if state.path_exists(path) {
                    state.cwd = path.clone();
                } else {
                    out.push_str(&format!("cd: not found: {path}\n"));
                }
            } else {
                out.push_str("cd: missing path\n");
            }
        }
        "pwd" => {
            out.push_str(&state.cwd);
            out.push('\n');
        }
        "export" => {
            if let Some(spec) = args.get(1) {
                if let Some((k, v)) = spec.split_once('=') {
                    state.env.insert(k.to_string(), v.to_string());
                    out.push_str(&format!("set {k}={v}\n"));
                } else {
                    out.push_str("export: invalid\n");
                }
            } else {
                out.push_str("export: invalid\n");
            }
        }
        "unset" => {
            if let Some(key) = args.get(1) {
                state.env.remove(key);
                out.push_str(&format!("unset {key}\n"));
            } else {
                out.push_str("unset: missing key\n");
            }
        }
        "ps" => out.push_str("PID PPID STATE CMD\n1 0 Running sh\n"),
        "ip" => {
            if args.len() < 2 {
                out.push_str("ip: usage: ip addr|route\n");
            } else if args[1] == "addr" {
                out.push_str("1: eth0    inet 10.0.2.15/24\n");
            } else if args[1] == "route" {
                out.push_str("default via 10.0.2.2 dev eth0\n");
                out.push_str("10.0.2.0/24 dev eth0 src 10.0.2.15\n");
            } else {
                out.push_str("ip: usage: ip addr|route\n");
            }
        }
        "ping" => {
            if let Some(host) = args.get(1) {
                if let Some(dst_ip) = parse_ipv4(host) {
                    let payload = [b'q', b'o', b's'];
                    if ping_via_syscall_raw(dst_ip, &payload).is_some() {
                        out.push_str(&format!(
                            "PING {host}\n64 bytes from {host}: icmp_seq=1 ttl=64 time=1ms\n1 packets transmitted, 1 received\n"
                        ));
                    } else {
                        out.push_str(&format!(
                            "PING {host}\nFrom {} icmp_seq=1 Destination Host Unreachable\n1 packets transmitted, 0 received\n",
                            fmt_ipv4(QOS_NET_IPV4_LOCAL)
                        ));
                    }
                } else {
                    out.push_str("ping: invalid host\n");
                }
            } else {
                out.push_str("ping: missing host\n");
            }
        }
        "wget" => {
            if let Some(url) = args.get(1) {
                if let Some(resp) = wget_via_syscall_stream(url) {
                    out.push_str(&resp);
                } else {
                    out.push_str("wget: failed\n");
                }
            } else {
                out.push_str("wget: missing url\n");
            }
        }
        _ => out.push_str(&format!("unknown command: {cmd}\n")),
    }

    if let Some((path, append)) = redir_out {
        if append {
            state.files.entry(path).or_default().push_str(&out);
        } else {
            state.files.insert(path, out.clone());
        }
        out.clear();
    }

    (out, should_exit)
}

fn run_command(state: &mut ShellState, line: &str) -> bool {
    let tokens = tokenize(line);
    if tokens.is_empty() {
        return false;
    }

    if let Some(pipe_idx) = tokens.iter().position(|t| t == "|") {
        if pipe_idx > 0 && pipe_idx + 1 < tokens.len() {
            let (left_out, left_exit) = execute_segment(state, &tokens[..pipe_idx], None);
            let (right_out, right_exit) = execute_segment(state, &tokens[pipe_idx + 1..], Some(&left_out));
            if !right_out.is_empty() {
                print!("{right_out}");
            }
            return left_exit || right_exit;
        }
    }

    let (out, should_exit) = execute_segment(state, &tokens, None);
    if !out.is_empty() {
        print!("{out}");
    }
    should_exit
}

fn main() {
    let mut args = std::env::args();
    let _prog = args.next();
    let mut state = ShellState::new();
    ensure_kernel_booted();

    if let Some(flag) = args.next() {
        if flag == "--once" {
            let one = args.collect::<Vec<_>>().join(" ");
            let _ = run_command(&mut state, &one);
            return;
        }
    }

    let stdin = io::stdin();
    let mut lines = stdin.lock().lines();
    let mut stdout = io::stdout();

    loop {
        print!("qos-sh:{}> ", state.cwd);
        let _ = stdout.flush();

        let line = match lines.next() {
            Some(Ok(l)) => l,
            Some(Err(_)) | None => {
                println!();
                return;
            }
        };

        if run_command(&mut state, &line) {
            return;
        }
    }
}
