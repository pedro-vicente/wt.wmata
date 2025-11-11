# WMATA Red Line Live Train Tracker

A real-time web application for tracking Washington DC Metro Red Line trains with interpolated positioning between stations. 

https://developer.wmata.com/apis

Built with C++ (Wt framework) and MapLibre GL JS.

[Wt](https://www.webtoolkit.eu/wt) is a C++ library for developing web applications. 

[MapLibre](https://www.maplibre.org/) is a Javascript library for developing web mapping applications. 


![image](https://github.com/user-attachments/assets/0e7cb7b1-0797-44ee-b4af-11ab5ff99c0e)

## Features

### Real-Time Train Tracking
- **Live train positions** with 2-minute refresh intervals
- **Interpolated positioning** - trains appear between stations based on arrival times
- **Direction-aware** - calculates correct position based on train direction (Glenmont ↔ Shady Grove)
- **Interactive markers** - hover over trains to see destination, next stop, arrival time, and car count

### Map Visualization
- **DC Ward boundaries** with color-coded overlay
- **Metro stations** displayed as colored circles by line
- **Red Line path** visualization
- **Station information** - hover over stations for name, line, and address
- **Responsive map controls** with zoom and navigation

## Prerequisites

This project requires CMake, a C++ compiler, OpenSSL, and the ASIO and Boost C++ libraries.

## Installation Instructions 

### macOS

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake openssl 
export OPENSSL_ROOT_DIR=$(brew --prefix openssl)
export PKG_CONFIG_PATH=$(brew --prefix openssl)/lib/pkgconfig:$PKG_CONFIG_PATH
```

### Linux (Ubuntu/Debian)

```bash
sudo apt install build-essential cmake libssl-dev 
```

### Windows 

1. Install CMake from https://cmake.org/download/
2. Download OpenSSL pre-compiled binaries from https://slproweb.com/products/Win32OpenSSL.html
3. Set environment variable:

```cmd
set OPENSSL_ROOT_DIR=C:\OpenSSL-Win64
```

## Building the Project

On a bash shell (on Windows, Git bash shell), run:

```bash
build.boost.sh
build.wt.sh
build.cmake.sh
```

## Setup

1. Edit the `config.json` file in the project root with your WMATA API key. Obtain key from:

https://developer.wmata.com/

```json
{
  "API_KEY": "MY_API_KEY"
}
```

## Running

```bash
./wmata --docroot . --http-address 0.0.0.0 --http-port 8080
```

Access at: `http://localhost:8080`
