// SPDX-License-Identifier: GPL-2.0
//
// Userspace test program for the hello_rust kernel module.
//
// This program demonstrates how to interact with a kernel module from
// userspace by:
//   1. Loading/unloading the module via /proc/modules
//   2. Reading module parameters via /sys/module/
//   3. Checking kernel log messages via /dev/kmsg
//
// Usage:
//   # First load the kernel module:
//   insmod /home/qwert/build/samples/rust/hello_rust.ko greeting_count=5
//
//   # Then run this test:
//   /home/qwert/linux-lab/examples/rust/rust_learn/user/test_hello

use std::fs;
use std::io::{self, BufRead, BufReader};
use std::path::Path;

const MODULE_NAME: &str = "hello_rust";
const SYSFS_PATH: &str = "/sys/module/hello_rust";
const PARAM_PATH: &str = "/sys/module/hello_rust/parameters/greeting_count";

fn check_module_loaded() -> io::Result<bool> {
    let content = fs::read_to_string("/proc/modules")?;
    Ok(content.lines().any(|line| line.starts_with(MODULE_NAME)))
}

fn read_parameter(path: &str) -> io::Result<String> {
    fs::read_to_string(path).map(|s| s.trim().to_string())
}

#[allow(dead_code)]
fn read_recent_kmsg(filter: &str) -> io::Result<Vec<String>> {
    let file = fs::File::open("/dev/kmsg")?;
    let reader = BufReader::new(file);
    let mut matches = Vec::new();

    // /dev/kmsg is non-blocking after current position; read what's available
    for line in reader.lines() {
        match line {
            Ok(l) => {
                if l.contains(filter) {
                    // kmsg format: "priority,sequence,timestamp,-;message"
                    if let Some(msg) = l.split(';').nth(1) {
                        matches.push(msg.trim().to_string());
                    }
                }
            }
            Err(e) if e.kind() == io::ErrorKind::WouldBlock => break,
            Err(_) => break,
        }
    }
    Ok(matches)
}

fn main() {
    println!("=== Hello Rust Module Test ===\n");

    // Check if module is loaded
    print!("Checking if '{}' module is loaded... ", MODULE_NAME);
    match check_module_loaded() {
        Ok(true) => println!("[OK] Module is loaded"),
        Ok(false) => {
            println!("[FAIL] Module not found");
            println!("\nHint: Load the module first:");
            println!("  insmod /home/qwert/build/samples/rust/hello_rust.ko");
            println!("  insmod /home/qwert/build/samples/rust/hello_rust.ko greeting_count=5");
            std::process::exit(1);
        }
        Err(e) => {
            println!("[ERROR] {}", e);
            std::process::exit(1);
        }
    }

    // Check sysfs presence
    print!("Checking sysfs at {}... ", SYSFS_PATH);
    if Path::new(SYSFS_PATH).exists() {
        println!("[OK]");
    } else {
        println!("[FAIL] sysfs directory not found");
        std::process::exit(1);
    }

    // Read module parameter
    print!("Reading greeting_count parameter... ");
    match read_parameter(PARAM_PATH) {
        Ok(value) => {
            println!("[OK] greeting_count = {}", value);
            if let Ok(count) = value.parse::<i32>() {
                println!("  The module printed {} greetings on load", count);
            }
        }
        Err(e) => {
            println!("[FAIL] {}", e);
        }
    }

    // List all module parameters
    let params_dir = format!("{}/parameters", SYSFS_PATH);
    if Path::new(&params_dir).exists() {
        println!("\nModule parameters:");
        if let Ok(entries) = fs::read_dir(&params_dir) {
            for entry in entries.flatten() {
                let name = entry.file_name();
                let path = entry.path();
                if let Ok(val) = fs::read_to_string(&path) {
                    println!("  {} = {}", name.to_string_lossy(), val.trim());
                }
            }
        }
    }

    // Show module info from /proc/modules
    println!("\nModule info from /proc/modules:");
    if let Ok(content) = fs::read_to_string("/proc/modules") {
        for line in content.lines() {
            if line.starts_with(MODULE_NAME) {
                let fields: Vec<&str> = line.split_whitespace().collect();
                if fields.len() >= 3 {
                    println!("  Name: {}", fields[0]);
                    println!("  Size: {} bytes", fields[1]);
                    println!("  Used by: {} module(s)", fields[2]);
                }
                break;
            }
        }
    }

    // Try reading kernel messages
    println!("\nKernel messages (from dmesg):");
    if let Ok(output) = std::process::Command::new("dmesg")
        .output()
    {
        let stdout = String::from_utf8_lossy(&output.stdout);
        let rust_msgs: Vec<&str> = stdout
            .lines()
            .filter(|l| l.contains("hello_rust") || l.contains("Hello from Rust") || l.contains("Greeting #"))
            .collect();
        if rust_msgs.is_empty() {
            println!("  (no matching messages found)");
        } else {
            for msg in rust_msgs.iter().rev().take(10).rev() {
                println!("  {}", msg);
            }
        }
    }

    println!("\n=== Test complete! ===");
}
