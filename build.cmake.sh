#!/bin/bash
set -e
path_wt="$(pwd)/install/wt"
path_boost="$(pwd)/build/boost_1_88_0"
echo "Wt at: $path_wt"
echo "Boost at: $path_boost"
sleep 1

# check if ext/asio directory exists, if not clone it
if [ ! -d "ext/asio-1.30.2" ]; then
    echo "asio not found in ext/, cloning from GitHub..."
    mkdir -p ext
    pushd ext
    git -c advice.detachedHead=false clone --branch asio-1-30-2 --depth 1 https://github.com/chriskohlhoff/asio.git asio-1.30.2
    if [ $? -ne 0 ]; then
        echo "Failed to clone asio repository"
        exit 1
    fi
    popd
else
    echo "asio already exists in ext/"
fi

# build directory 
mkdir -p build/wmata
pushd build
pushd wmata

cmake ../.. --fresh \
    -DWT_INCLUDE="$path_wt/include" \
    -DBOOST_INCLUDE_DIR="$path_boost/include/boost-1_88" \
    -DBOOST_LIB_DIRS="$path_boost/lib"
cmake --build . --config Debug --verbose

echo "open browser http://localhost:8080"
if [[ "$OSTYPE" == "msys"* ]]; then
./Debug/web --http-address=0.0.0.0 --http-port=8080  --docroot=.
else
./web --http-address=0.0.0.0 --http-port=8080  --docroot=.
fi

popd
popd


exit
