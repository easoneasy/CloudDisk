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
# ========== HTTP 网盘服务 ==========
add_executable(server
    main/main.cc
    src/CloudDiskServer.cc
    src/UserHandler.cc
    src/FileHandler.cc
    src/CryptoUtil.cc
    src/OssManager.cc
    src/RabbitMQ.cc
    src/ConsulManager.cc
    src/userHandler.pb.cc
    src/fileHandler.pb.cc
)

# ========== SRPC 用户服务端 ==========
add_executable(userServer
    main/userServer_main.cc
    src/CryptoUtil.cc
    src/ConsulManager.cc
    src/userHandler.pb.cc
)

# ========== SRPC 文件服务端 ==========
add_executable(fileServer
    main/fileServer_main.cc
    src/ConsulManager.cc
    src/fileHandler.pb.cc
)

# ========== OSS 消费者 ==========
add_executable(oss_consumer
    main/OssWorker_main.cc
    src/OssManager.cc
    src/RabbitMQ.cc
    src/CryptoUtil.cc
)

# ========== 头文件路径 ==========
target_include_directories(server PRIVATE include)
target_include_directories(userServer PRIVATE include)
target_include_directories(fileServer PRIVATE include)
target_include_directories(oss_consumer PRIVATE include)

# ========== 编译选项 ==========
target_compile_options(server PRIVATE -g -fno-rtti)
target_compile_options(userServer PRIVATE -g -fno-rtti)
target_compile_options(fileServer PRIVATE -g -fno-rtti)
target_compile_options(oss_consumer PRIVATE -g -fno-rtti)

# ========== 链接 ==========
target_link_libraries(server PRIVATE
    wfrest
    srpc workflow protobuf
    mysqlclient
    crypto ssl jwt
    alibabacloud-oss-cpp-sdk curl
    SimpleAmqpClient
    ppconsul
    lz4 snappy pthread
)

target_link_libraries(userServer PRIVATE
    srpc workflow protobuf
    mysqlclient
    crypto ssl jwt
    ppconsul
    lz4 snappy pthread
)

target_link_libraries(fileServer PRIVATE
    srpc workflow protobuf
    mysqlclient
    ppconsul
    lz4 snappy pthread
)

target_link_libraries(oss_consumer PRIVATE
    crypto ssl jwt
    alibabacloud-oss-cpp-sdk curl
    SimpleAmqpClient
    pthread
)
