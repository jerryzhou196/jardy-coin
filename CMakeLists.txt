cmake_minimum_required(VERSION 3.15)
project(distributed_blockchain CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

# Layer 1 sources — grows each layer
set(SOURCES
    src/main.cpp
    src/block.cpp
    src/crypto.cpp
)

add_executable(blockchain ${SOURCES})

target_include_directories(blockchain PRIVATE
    src/
    third_party/
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(blockchain PRIVATE OpenSSL::Crypto)
