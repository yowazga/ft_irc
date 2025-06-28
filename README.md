# ft_irc - Internet Relay Chat Server
**A C++98 implementation of an RFC-compliant IRC server**



## 📜 Table of Contents
1. [Project Overview](#-project-overview)
2. [Features](#-features)
3. [Technical Specifications](#-technical-specifications)
4. [Installation](#-installation)
5. [Usage](#-usage)
6. [Commands](#-commands)
7. [Architecture](#-architecture)
8. [Testing](#-testing)
9. [Bonus](#-bonus)
10. [Resources](#-resources)

---

## 🌐 Project Overview
This project implements a complete IRC (Internet Relay Chat) server compliant with RFC 1459 standards. The server handles multiple client connections simultaneously, manages channels, and processes standard IRC commands - all written in C++98 following strict system programming guidelines.

Key challenges addressed:
- Non-blocking I/O with `poll()`
- Client connection management
- Channel operations and modes
- Message routing and protocol parsing
- Operator privileges system

---

## ✨ Features

### Core Functionality
✅ Multi-client simultaneous connections  
✅ Secure password authentication  
✅ Nickname/user registration  
✅ Public channels with configurable modes  
✅ Private messaging between users  
✅ Channel operator commands  

### Implemented Commands
| Category       | Commands                          |
|----------------|-----------------------------------|
| Connection     | PASS, NICK, USER, QUIT           |
| Channels       | JOIN, PART, LIST, NAMES          |
| Messaging      | PRIVMSG, NOTICE                  |
| Server Queries | PING, PONG                       |
| Operator       | KICK, INVITE, TOPIC, MODE        |

### Channel Modes
- `+i`: Invite-only
- `+t`: Topic protection
- `+k`: Password protected
- `+o`: Operator privileges
- `+l`: User limit

---

## ⚙️ Technical Specifications

### Requirements
- C++98 standard compliance
- Non-blocking I/O using `poll()` (or equivalent)
- TCP/IPv4 or IPv6 communication
- No forking - single process architecture
- Memory leak-free (valgrind clean)

### System Calls Used
```c++
socket(), bind(), listen(), accept()
poll()/select()/kqueue()/epoll()
fcntl() (MacOS only for non-blocking)
send(), recv()
```
### Compilation
```bash
make              # Compiles server
make clean        # Cleans object files
make fclean       # Full clean
make re 
```
## 📦 Installation
### Dependencies
- C++98 compatible compiler (g++/clang++)
- IRC client (HexChat, Irssi, or nc)
### Building
```bash
cd ft_irc
make
```
## 🖥️ Usage
### Starting the Server
```bash
./ircserv <port> <password>
# Example:
./ircserv 6667 securepassword
```
### Connecting Clients
```bash
# Using netcat (basic testing):
nc localhost 6667

# Using IRC client (recommended):
hexchat -> Server: localhost/6667, Password: securepassword
```
## 🏗️ Architecture
### Server Design
![Image](https://github.com/user-attachments/assets/a7b0d6c5-d80f-4fa3-a6fa-df70ac71dacb)
### Key Components
- **Network Core:** Handles all socket operations

- **Client Manager:** Tracks connected users

- **Channel Manager:** Manages channel states

- **Command Processor:** Executes IRC commands

- **Message Bus:** Routes messages between clients
## 🧪 Testing
### Automated Tests
```bash
# Run basic connection tests
./tests/connection_test.sh

# Stress testing
./tests/stress_test.py 100  # 100 simulated clients
```
### Manual Testing
1. Connect multiple clients

2. Verify channel creation/joining

3. Test message routing

4. Validate operator commands

5. Check error handling
### Test Cases
- Partial command handling

* Invalid command rejection

- Channel mode changes

+ Operator privilege escalation

+ High-load scenarios
## 🏆 Bonus
### Implemented Extras
🔹 File transfer support (DCC)

🔹 Interactive bot with custom commands

🔹 SSL/TLS encrypted connections

🔹 Server-to-server linking
## 📚 Resources
### RFC References
[RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459): IRC Protocol

[RFC 2812](https://datatracker.ietf.org/doc/html/rfc2812): Modern IRC spec
### Libraries
Standard C++98 libraries only

System calls: **<sys/socket.h>**, **<poll.h>**
### IRC Clients for Testing
[HexChat](https://hexchat.github.io/)

[Irssi](https://irssi.org/)

[Weechat](https://weechat.org/)

> [!TIP]
> "Talk is cheap. Show me the code." 

