# 设置CMake的最小版本
cmake_minimum_required(VERSION 3.15)

# 项目名称
#   项目可以包含多个目标(target)
#   target: 可执行程序, 静态库, 动态库
project(CloudDisk)

# 指定C++的版本: -std=c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置可执行程序生成的路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../)

# 生成的可执行程序
add_executable(server
    src/main.cc
    src/CloudDiskServer.cc
    src/CryptoUtil.cc
    src/UserHandler.cc
    src/FileHandler.cc
    src/OssManager.cc
    src/RabbitMQ.cc
)

add_executable(oss_consumer
    src/OssWorker_Consumer.cc
    src/OssManager.cc
    src/RabbitMQ.cc
    src/CryptoUtil.cc
)

target_include_directories(server PRIVATE include)

target_include_directories(oss_consumer PRIVATE include)

# 设置编译选项
# PRIVATE: 传播属性，一般填PRIVATE
target_compile_options(server PRIVATE
    -g
    -fno-rtti
)

target_link_libraries(oss_consumer PRIVATE
    crypto ssl jwt alibabacloud-oss-cpp-sdk
    curl pthread SimpleAmqpClient
)

# -lcrypto -lssl -ljwt -lwfrest
target_link_libraries(server PRIVATE
    crypto
    ssl
    jwt
    wfrest
    alibabacloud-oss-cpp-sdk
    curl
    pthread
    SimpleAmqpClient
)
