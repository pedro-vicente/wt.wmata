#!/bin/bash
set -e
remote=$(git config --get remote.origin.url)
echo "remote repository: $remote"
if [ ! -d "ext/wt-4.15.0" ]; then
    git -c advice.detachedHead=false clone -b 4.15.0 https://github.com/emweb/wt.git ext/wt-4.15.0
    rm -rf build/wt-4.15.0
else
    echo "ext/wt-4.15.0 already exists, skipping clone"
fi

sleep 2
mkdir -p build/wt-4.15.0
pushd build
pushd wt-4.15.0

echo "At: $(pwd)"

if [[ "$OSTYPE" == msys* || "$OSTYPE" == cygwin* ]]; then

path_boost="$(pwd)/../boost_1_92_0"
echo "Boost at: $path_boost"

cmake ../../ext/wt-4.15.0 --fresh \
 -G "Visual Studio 18 2026" -A x64 \
 -DSSL_PREFIX="${OPENSSL_ROOT_DIR:-C:/Program Files/OpenSSL-Win64}" \
 -DBUILD_TESTS=OFF \
 -DCMAKE_INSTALL_PREFIX="$(pwd)/../../install/wt" \
 -DBOOST_PREFIX="$path_boost" \
 -DBUILD_EXAMPLES=OFF \
 -DENABLE_QT5=OFF -DENABLE_QT6=OFF 
cmake --build .  --config Debug --parallel 
cmake --install . --config Debug

elif [[ "$OSTYPE" == "darwin"* ]]; then

cmake ../../ext/wt-4.15.0 \
 -DCMAKE_INSTALL_PREFIX="$(pwd)/../../install/wt" \
 -DBUILD_EXAMPLES=OFF \
 -DCMAKE_CXX_FLAGS="-Wno-deprecated -Wno-deprecated-declarations -Wno-deprecated-copy" \
 -DCMAKE_BUILD_TYPE=Release \
 -DBUILD_SHARED_LIBS=OFF \
 -DCMAKE_FIND_LIBRARY_SUFFIXES=".a" \
 -DBoost_USE_STATIC_LIBS=ON 
 
cmake --build . --config Release --parallel 
cmake --install . --config Release

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then

cmake ../../ext/wt-4.15.0  \
 -DCMAKE_INSTALL_PREFIX="$(pwd)/../../install/wt" \
 -DBUILD_EXAMPLES=OFF \
 -DCMAKE_BUILD_TYPE=Release \
 -DOPENSSL_ROOT_DIR=/usr \
 -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.so \
 -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.so \
 -DENABLE_SSL:BOOL=ON

cmake --build . --config Release -j$(nproc)
cmake --install . --config Release

fi

popd 
popd