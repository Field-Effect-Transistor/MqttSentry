# MqttSentry

A high-performance, asynchronous C++ monitor that bridges MQTT machine data with Telegram notifications. Designed specifically to run reliably on resource-constrained hardware like **Raspberry Pi 1 Model B+**.

## Key Features

*   **Asynchronous MQTT Engine:** Powered by `boost::mqtt5` and `Boost.Asio` for non-blocking I/O.
*   **Machine Watchdog System:** Real-time tracking of machine "heartbeats" with configurable timeout limits.
*   **Telegram Integration:** Reliable alerts via `tgbot-cpp`.
*   **Advanced Architecture:** Implements the **Producer-Consumer** pattern with a thread-safe queue to ensure MQTT stability even during slow network requests to Telegram.
*   **Resource Efficient:** Optimized for single-core ARMv6 systems (512MB RAM).
*   **Reliable Configuration:** JSON-based dynamic configuration with thread-safe access.

## Architecture

The application is split into three main execution contexts:
1.  **Main Thread (Event Loop):** Manages MQTT communication, system signals (SIGINT/SIGTERM), and Watchdog timers using `io_context`.
2.  **Telegram Thread:** Dedicated to `LongPoll` operations to handle incoming user commands without blocking the system.
3.  **Worker Thread:** A dedicated consumer that processes the alert queue and sends HTTP requests to Telegram API, preventing network latency from affecting MQTT performance.

## Stack

*   **Language:** Modern C++17
*   **Networking:** [Boost.Asio](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html), [Boost.MQTT5](https://github.com/boostorg/mqtt5)
*   **Telegram API:** [tgbot-cpp](https://github.com/reo7sp/tgbot-cpp)
*   **JSON:** [nlohmann/json](https://github.com/nlohmann/json)
*   **Build System:** CMake + Ninja
*   **Deployment:** Docker (Cross-compilation) + Systemd

## Installation & Build

### Prerequisites
*   Docker with `buildx` support
*   Git submodules initialized: `git submodule update --init --recursive`

### Cross-Compilation for Raspberry Pi 1 (ARMv6)
Since RPi 1 is too weak for native compilation, use the provided Docker environment to build a **fully static** binary:

```bash
# Build the static binary on your PC
docker buildx build --platform linux/arm/v6 -t mqtt-sentry-builder --load .

# Extract the binary
docker create --name extract mqtt-sentry-builder
docker cp extract:/app/build/tgBot ./tgBot_static
docker rm extract

# Strip debug symbols to reduce memory footprint
strip ./tgBot_static
```

## Configuration

Create a `config.json` in the application directory:

```json
{
    "tg": {
        "token": "YOUR_BOT_TOKEN",
        "users": ["USER1_ID", "USER2_ID"]
    },
    "mqtt": {
        "broker": "BROKER",
        "port": 1883,
        "client_id": "rpi_monitor",
        "topic": ["NordFrost/MACHINE_ID/#"]
    },
    "logic": {
        "timeout": 60,
        "timeout_limit": 3,
        "code": {
            "0": "System OK",
            "100": "Critical Overheat"
        }
    }
}
```

## Usage

### Running as a Daemon
To ensure the bot starts on boot and restarts automatically, use the provided `systemd` service:

1. Copy the binary and config to `/home/pie/`.
2. Create service file: `sudo nano /etc/systemd/system/tgbot.service`
3. Paste the following:

```ini
[Unit]
Description=MqttSentry Service
After=network-online.target

[Service]
Type=simple
User=pie
WorkingDirectory=/home/pie
ExecStart=/home/pie/tgBot_static config.json
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

4. Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable tgbot
sudo systemctl start tgbot
```
