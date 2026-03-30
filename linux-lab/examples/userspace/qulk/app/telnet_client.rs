use std::io::{self, Read, Write};
use std::net::TcpStream;
use std::sync::{Arc, Mutex};
use std::thread;

// Telnet 协议定义
const IAC: u8 = 255;  // Interpret As Command
const DONT: u8 = 254; // 拒绝选项
const DO: u8 = 253;   // 请求对方启用选项
const WONT: u8 = 252; // 拒绝启用选项
const WILL: u8 = 251; // 同意启用选项
const SB: u8 = 250;   // Subnegotiation Begin
const SE: u8 = 240;   // Subnegotiation End

fn handle_telnet_option(stream: &mut TcpStream, buffer: &[u8]) {
	let mut i = 0;
	while i < buffer.len() {
		if buffer[i] == IAC {
			if i + 2 >= buffer.len() {
				break; // 防止越界
			}

			let command = buffer[i + 1];
			let option = buffer[i + 2];
			match command {
				DO => match option {
					1 | 3 => {
						// 同意服务器请求的 Echo 和 Suppress Go Ahead
						stream.write_all(&[IAC, WILL, option]).unwrap();
					}
					24 => {
						// 处理终端类型
						println!("Request for terminal type...");
						stream.write_all(&[IAC, WILL, option]).unwrap();
						stream.write_all(&[IAC, SB, option, 0]).unwrap();
						stream.write_all(b"xterm").unwrap();
						stream.write_all(&[IAC, SE]).unwrap();
					}
					_ => send_rejection(stream, option),
				},
				DONT => {
					println!("DONT option: {}", option);
					send_rejection(stream, option);
				}
				WILL => {
					println!("WILL option: {}", option);
					stream.write_all(&[IAC, DONT, option]).unwrap();
				}
				WONT => {
					println!("WONT option: {}", option);
					stream.write_all(&[IAC, DONT, option]).unwrap();
				}
				SB => {
					println!("Subnegotiation...");
					while i < buffer.len() && buffer[i] != SE {
						i += 1; // 跳过子选项内容
					}
				}
				_ => {
					println!("Unknown command: {}", command);
					send_rejection(stream, option);
				}
			}
			i += 2; // 跳过命令和选项
		}
		i += 1;
	}
}

fn send_rejection(stream: &mut TcpStream, option: u8) {
	stream.write_all(&[IAC, WONT, option]).unwrap();
}

fn main() -> io::Result<()> {
	let args: Vec<String> = std::env::args().collect();
	if args.len() != 3 {
		eprintln!("Usage: {} <hostname> <port>", args[0]);
		std::process::exit(1);
	}

	let hostname = &args[1];
	let port = &args[2];
	let address = format!("{}:{}", hostname, port);

	// 连接到服务器
	let mut stream = TcpStream::connect(&address)?;
	println!("Connected to {}", address);

    let mut reader = stream.try_clone()?;
    let mut writer = stream;

	thread::spawn(move || {
		let mut buffer = [0; 1024];
		loop {
			match reader.read(&mut buffer) {
				Ok(n) if n > 0 => {
					println!("Received data: {:?}", &buffer[..n]);
                    let mut writer_clone = reader.try_clone().unwrap();
					handle_telnet_option(&mut writer_clone, &buffer[..n]);

					for &byte in &buffer[..n] {
						if byte != IAC {
							print!("{}", byte as char);
						}
					}
					io::stdout().flush().unwrap();
				}
				Ok(_) => break, // Connection closed
				Err(e) => {
					eprintln!("Error reading from server: {}", e);
					break;
				}
			}
		}
	});

	loop {
		let mut input = String::new();
		io::stdin().read_line(&mut input)?;

		// 确保换行符为 \r\n
		if !input.ends_with("\n") {
			input.push('\n');
		}
		input = input.replace("\n", "\r\n");

		if let Err(e) = writer.write_all(input.as_bytes()) {
			eprintln!("Error writing to server: {}", e);
			break;
		}
	}

	Ok(())
}
