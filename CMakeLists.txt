cmake_minimum_required(VERSION 3.28)
project (wmata)

message(STATUS "Source directory is " ${CMAKE_SOURCE_DIR})
message(STATUS "Build directory is " ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "WT_INCLUDE: " ${WT_INCLUDE})
message(STATUS "BOOST_INCLUDE_DIR: " ${BOOST_INCLUDE_DIR})
message(STATUS "BOOST_LIB_DIRS: " ${BOOST_LIB_DIRS})

include_directories(${BOOST_INCLUDE_DIR})
include_directories(${WT_INCLUDE})
add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)

#//////////////////////////
# asio
#//////////////////////////

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DASIO_STANDALONE)
add_definitions(-DASIO_HAS_STD_ADDRESSOF)
add_definitions(-DASIO_HAS_STD_ARRAY)
add_definitions(-DASIO_HAS_CSTDINT)
add_definitions(-DASIO_HAS_STD_SHARED_PTR)
add_definitions(-DASIO_HAS_STD_TYPE_TRAITS)
add_definitions(-DASIO_HAS_VARIADIC_TEMPLATES)
add_definitions(-DASIO_HAS_STD_FUNCTION)
add_definitions(-DASIO_HAS_STD_CHRONO)
add_definitions(-DBOOST_ALL_NO_LIB)
if (MSVC)
  add_definitions(-D_WIN32_WINNT=0x0501)
  add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

include_directories(ext/asio-1.30.2/asio/include)

#//////////////////////////
# link with libraries
# lib_dep contains a cascade definition of all the libraries needed to link
#//////////////////////////

set(lib_dep ${lib_dep})

#//////////////////////////
# OpenSSL
#//////////////////////////

if (MSVC)
  if (OPEN_SSL_SOURCE)
    set(OPENSSL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/openssl-3.0.5)
    set(OPENSSL_INCLUDE_DIR ${OPENSSL_ROOT_DIR}/include)
    set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_ROOT_DIR}/libcrypto.lib)
    set(OPENSSL_SSL_LIBRARY ${OPENSSL_ROOT_DIR}/libssl.lib)
  else()
    set(OPENSSL_ROOT_DIR $ENV{OPENSSL_ROOT_DIR})
    message(STATUS "OpenSSL root folder from environment, (OPENSSL_ROOT_DIR): ${OPENSSL_ROOT_DIR}")
  endif()
endif()

find_package(OpenSSL REQUIRED)

message(STATUS "OpenSSL root: ${OPENSSL_ROOT_DIR}")
message(STATUS "OpenSSL include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libs: ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY}")

if(UNIX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

include_directories(${OPENSSL_INCLUDE_DIR})

set(lib_dep ${lib_dep} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
if (MSVC)
  set(lib_dep ${lib_dep} crypt32.lib)
endif()

#//////////////////////////
# source files 
#//////////////////////////

set(src ${src})
set(src ${src} src/ssl_read.hh)
set(src ${src} src/ssl_read.cc)
set(src ${src} src/get.hh)
set(src ${src} src/get.cc)
set(src ${src} src/get.cc)

#//////////////////////////
# create static library from common source files
#//////////////////////////

add_library(get STATIC ${src})
target_link_libraries(get ${lib_dep})

#//////////////////////////
# http_client executable
#//////////////////////////

add_executable(http_client src/http_client.cc)
target_link_libraries (http_client get ${lib_dep})

add_executable(parser src/parse.cc src/map.cc src/map.hh)

#//////////////////////////
# copy config file to build folder
#//////////////////////////

message(STATUS "Copying resources to: ${CMAKE_BINARY_DIR}")
file(COPY "${CMAKE_SOURCE_DIR}/config.json" DESTINATION ${CMAKE_BINARY_DIR})

#//////////////////////////
# Wt web client 
#//////////////////////////

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/wt-4.12.0/src")

add_executable(wmata src/wmata.cc src/map.cc src/map.hh)

if (MSVC)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT wmata)
  set_property(TARGET wmata PROPERTY VS_DEBUGGER_COMMAND_ARGUMENTS "--http-address=0.0.0.0 --http-port=8080  --docroot=.")
  set_property(TARGET wmata PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

#//////////////////////////
# Linux/Mac
#//////////////////////////

if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

if(UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations")
endif()

if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations -Wno-deprecated-copy -Wno-deprecated-copy-dtor")
endif()

message(STATUS "Compiler flags: ${CMAKE_CXX_FLAGS}")

#//////////////////////////
# LINUX
# order of the link libraries matters; pthread dl
#//////////////////////////

if(LINUX)
  set(lib_dep ${lib_dep} pthread dl)
  set(lib_dep ${lib_dep} stdc++fs) 
  find_program(LSB_RELEASE_EXEC lsb_release)
  execute_process(COMMAND ${LSB_RELEASE_EXEC} -is OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "Building in " ${LSB_RELEASE_ID_SHORT})
endif()

if (MSVC)
  set(lib_dep ${lib_dep} ${CMAKE_CURRENT_SOURCE_DIR}/build/wt-4.12.0/src/Debug/wtd.lib)
  set(lib_dep ${lib_dep} ${CMAKE_CURRENT_SOURCE_DIR}/build/wt-4.12.0/src/http/Debug/wthttpd.lib)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_filesystem-vc143-mt-gd-x64-1_88.lib)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_thread-vc143-mt-gd-x64-1_88.lib)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_program_options-vc143-mt-gd-x64-1_88.lib)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_chrono-vc143-mt-gd-x64-1_88.lib)
endif()

if(APPLE)
  set(lib_dep ${lib_dep} ${CMAKE_CURRENT_SOURCE_DIR}/install/wt/lib/libwt.dylib)
  set(lib_dep ${lib_dep} ${CMAKE_CURRENT_SOURCE_DIR}/install/wt/lib/libwthttp.dylib)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_filesystem-clang-darwin17-mt-x64-1_88.a)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_thread-clang-darwin17-mt-x64-1_88.a)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_program_options-clang-darwin17-mt-x64-1_88.a)
endif()

if (LINUX)
  set(lib_dep ${lib_dep} ${CMAKE_CURRENT_SOURCE_DIR}/install/wt/lib/libwt.so)
  set(lib_dep ${lib_dep} ${CMAKE_CURRENT_SOURCE_DIR}/install/wt/lib/libwthttp.so)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_filesystem-gcc13-mt-x64-1_88.a)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_thread-gcc13-mt-x64-1_88.a)
  set(lib_dep ${lib_dep} ${BOOST_LIB_DIRS}/libboost_program_options-gcc13-mt-x64-1_88.a)
endif()

#//////////////////////////
#  link
#//////////////////////////

message(STATUS "lib_dep: " ${lib_dep})
target_link_libraries (wmata get ${lib_dep})

set(DATA_FILES
  "resources/stations_BL.json"
  "resources/stations_GR.json"
  "resources/stations_OR.json"
  "resources/stations_RD.json"
  "resources/stations_SV.json"
  "resources/stations_YL.json"
  "resources/line_BL.json"
  "resources/line_GR.json"
  "resources/line_OR.json"
  "resources/line_RD.json"
  "resources/line_SV.json"
  "resources/line_YL.json"
  "resources/ward-2012.geojson"
  "resources/rail_shapes.geojson"
)

message(STATUS "Copying data files to: ${CMAKE_BINARY_DIR}/data")
foreach(DATA_FILE ${DATA_FILES})
  file(COPY "${CMAKE_SOURCE_DIR}/${DATA_FILE}" DESTINATION ${CMAKE_BINARY_DIR}/data)
endforeach()

#//////////////////////////
# GTFS data files
#//////////////////////////

file(GLOB GTFS_FILES "${CMAKE_SOURCE_DIR}/resources/gtfs/*")
message(STATUS "Copying GTFS files to: ${CMAKE_BINARY_DIR}/data/gtfs")
file(COPY ${GTFS_FILES} DESTINATION ${CMAKE_BINARY_DIR}/data/gtfs)

