# C++11 Multithreaded Chat Server/Client

## Features
- Multithreaded server with thread pool.
- Multiple clients can chat concurrently.
- Custom protocol (length-prefixed framing).
- Basic commands: `/nick`, `/list`, `/msg`, `/quit`.
- Configurable via `config.json`.
- Logging with timestamps.

## Build
```bash
mkdir build && cd build
cmake ..
make -j
