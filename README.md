# Washington Metropolitan Area Transit Authority (WMATA) API Client

A C++ application that fetches data from the WMATA API using HTTPS.

https://developer.wmata.com/apis

## Prerequisites

This project requires CMake, a C++ compiler, OpenSSL, and the ASIO library.

## Installation Instructions

### macOS

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install cmake openssl asio
export OPENSSL_ROOT_DIR=$(brew --prefix openssl)
export PKG_CONFIG_PATH=$(brew --prefix openssl)/lib/pkgconfig:$PKG_CONFIG_PATH
```

### Linux (Ubuntu/Debian)

```bash
sudo apt install build-essential cmake libssl-dev libasio-dev
```

### Windows 

1. Install CMake from https://cmake.org/download/
2. Download OpenSSL pre-compiled binaries from https://slproweb.com/products/Win32OpenSSL.html
3 Set environment variable:

```cmd
set OPENSSL_ROOT_DIR=C:\OpenSSL-Win64
```

## Building the Project

On a bash shell (on Windows, Git bash shell), run 
```bash
chmod a+x build.cmake.sh 
build.cmake.sh
```

## Setup

1. Edit file `config.json` file in the project root with your WMATA API key. Obtain key from 

https://developer.wmata.com/


```json
{
  "API_KEY": "MY_API_KEY"
}

```

2. Run the application:
```bash
./wmata 
```
