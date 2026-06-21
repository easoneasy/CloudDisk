# CloudDisk

CloudDisk 是一个基于 C++17 的高性能分布式云盘系统，采用微服务架构，支持用户注册登录、文件上传下载、文件列表管理以及阿里云 OSS 异步云备份等功能。

## 核心特性

- **微服务架构** — HTTP 网关、用户服务、文件服务独立部署，通过 SRPC 通信
- **服务注册与发现** — 基于 Consul 实现服务自动注册、健康检查和动态发现
- **JWT 鉴权** — 用户登录后颁发 JWT Token，所有 API 请求统一验证身份
- **密码安全** — SHA-256 + 随机盐值哈希存储密码
- **异步云备份** — 文件上传后通过 RabbitMQ 消息队列异步同步至阿里云 OSS
- **现代化 Web 前端** — 单页应用，支持分页文件列表、拖拽上传、实时进度展示
- **高性能异步框架** — 基于 Sogou Workflow 的纯异步非阻塞 IO

## 项目架构

```
┌──────────────┐
│   浏览器      │
│  (SPA 前端)   │
└──────┬───────┘
       │ HTTP (8888)
┌──────▼───────────────────────────────────────┐
│              server (HTTP 网关)               │
│  CloudDiskServer + wfrest                    │
│  路由: /api/v1/auth/* /api/v1/user/*         │
│        /api/v1/files/* /static               │
└───┬───────────────┬──────────────────┬───────┘
    │ SRPC          │ SRPC             │ 直接操作
    ▼               ▼                  ▼
┌─────────┐  ┌────────────┐  ┌──────────────┐
│userServer│  │ fileServer │  │   MySQL      │
│ :8001   │  │ :8002      │  │  (CloudDisk) │
└────┬────┘  └──┬─────────┘  └──────────────┘
     │          │
     └────┬─────┘
          │ 服务发现
     ┌────▼─────┐
     │  Consul  │
     └──────────┘

┌───────────────────────────────────────────────┐
│  文件上传异步流程:                              │
│                                               │
│  server 本地落盘 ──► RabbitMQ ──► oss_consumer │
│                                       │        │
│                                 阿里云 OSS      │
└───────────────────────────────────────────────┘
```

## 技术栈

| 层次 | 技术 |
|------|------|
| **语言** | C++17 |
| **HTTP 框架** | [wfrest](https://github.com/wfrest/wfrest)（基于 Sogou Workflow） |
| **RPC 框架** | [srpc](https://github.com/sogou/srpc)（Sogou RPC） |
| **序列化** | Protocol Buffers (proto3) |
| **数据库** | MySQL（通过 Workflow 的异步 MySQL 任务） |
| **服务发现** | Consul（ppconsul） |
| **消息队列** | RabbitMQ（SimpleAmqpClient） |
| **对象存储** | 阿里云 OSS C++ SDK |
| **认证加密** | JWT（libjwt）+ OpenSSL（SHA-256） |
| **JSON 解析** | [nlohmann/json](https://github.com/nlohmann/json) |
| **压缩** | LZ4, Snappy |
| **构建系统** | CMake >= 3.15 |
| **前端** | HTML5 + CSS3 + 原生 JavaScript（SPA） |

## 项目结构

```
CloudDisk/
├── CMakeLists.txt            # CMake 构建配置
├── build.sh                  # 一键构建脚本
├── config.json               # 阿里云 OSS 配置
├── proto/                    # Protobuf 定义
│   ├── userHandler.proto     # 用户服务 RPC 接口
│   └── fileHandler.proto     # 文件服务 RPC 接口
├── include/                  # 头文件
│   ├── CloudDiskServer.h     # HTTP 网关服务
│   ├── UserHandler.h         # 用户路由处理器
│   ├── FileHandler.h         # 文件路由处理器
│   ├── CryptoUtil.h          # 加密工具（JWT/SHA-256）
│   ├── OssManager.h          # 阿里云 OSS 管理器
│   ├── RabbitMQ.h            # RabbitMQ 生产者/消费者
│   └── ConsulManager.h       # Consul 服务注册/发现
├── src/                      # 源文件
│   ├── CloudDiskServer.cc    # 路由注册
│   ├── UserHandler.cc        # 注册/登录/用户信息处理
│   ├── FileHandler.cc        # 文件列表/上传/下载处理
│   ├── CryptoUtil.cc         # 密码哈希/JWT 实现
│   ├── OssManager.cc         # OSS 上传/下载实现
│   ├── RabbitMQ.cc           # 消息队列实现
│   ├── ConsulManager.cc      # 服务发现实现
│   ├── userHandler.pb.cc     # Protobuf 生成代码
│   └── fileHandler.pb.cc     # Protobuf 生成代码
├── main/                     # 入口文件
│   ├── main.cc               # HTTP 网关入口（端口 8888）
│   ├── userServer_main.cc    # 用户微服务入口（端口 8001）
│   ├── fileServer_main.cc    # 文件微服务入口（端口 8002）
│   └── OssWorker_main.cc     # OSS 消费 Worker 入口
├── www/                      # 前端资源
│   ├── index.html            # 主页面（SPA）
│   └── static/
│       ├── style.css         # 样式
│       ├── api.js            # API 封装
│       ├── login.html        # 登录页
│       └── register.html     # 注册页
└── storage/                  # 本地文件存储目录（运行时生成）
```

## API 接口

### 认证模块

| 方法 | 路径 | 说明 |
|------|------|------|
| POST | `/api/v1/auth/register` | 用户注册 |
| POST | `/api/v1/auth/login` | 用户登录 |

### 用户模块

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/user/me` | 获取当前用户信息（需 Bearer Token） |

### 文件模块

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/api/v1/files` | 获取文件列表 |
| POST | `/api/v1/files` | 上传文件（multipart/form-data） |
| GET | `/api/v1/file/{id}` | 下载文件 |

### 静态资源

| 方法 | 路径 | 说明 |
|------|------|------|
| GET | `/` | 主页面（SPA） |
| GET | `/static/*` | 静态资源（CSS/JS/HTML） |

## 快速开始

### 依赖环境

- CMake >= 3.15
- g++ 支持 C++17
- MySQL 8.0+
- RabbitMQ 3.x
- Consul
- 阿里云 OSS 账号（用于云备份）

### 安装依赖库

```bash
# Ubuntu/Debian
sudo apt install libmysqlclient-dev libssl-dev libjwt-dev \
                 liblz4-dev libsnappy-dev libprotobuf-dev \
                 protobuf-compiler libcurl4-openssl-dev

# 以下依赖建议通过 vcpkg 或源码编译安装:
# - workflow + wfrest (HTTP 框架)
# - srpc (RPC 框架)
# - ppconsul (Consul 客户端)
# - SimpleAmqpClient (RabbitMQ 客户端)
# - alibabacloud-oss-cpp-sdk (阿里云 OSS SDK)
```

### 构建

```bash
# 使用构建脚本
./build.sh

# 或手动构建
mkdir -p build && cd build
cmake ..
make -j$(nproc)
cd ..
```

构建产物将生成在项目根目录：

- `server` — HTTP 网关
- `userServer` — 用户微服务
- `fileServer` — 文件微服务
- `oss_consumer` — OSS 消费 Worker

### 配置

编辑 `config.json` 填入阿里云 OSS 信息：

```json
{
    "endpoint": "oss-cn-wuhan-lr.aliyuncs.com",
    "accessKeyId": "你的AccessKeyId",
    "accessKeySecret": "你的AccessKeySecret",
    "bucketName": "你的Bucket名称",
    "region": "cn-wuhan"
}
```

### 初始化数据库

```sql
CREATE DATABASE IF NOT EXISTS CloudDisk;
USE CloudDisk;

CREATE TABLE IF NOT EXISTS tbl_user (
    id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    salt VARCHAR(32) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS tbl_file (
    id INT PRIMARY KEY AUTO_INCREMENT,
    uid INT NOT NULL,
    filename VARCHAR(255) NOT NULL,
    hashcode VARCHAR(255) NOT NULL,
    size BIGINT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_update TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (uid) REFERENCES tbl_user(id)
);
```

### 启动服务

```bash
# 1. 启动 Consul
consul agent -dev

# 2. 启动 RabbitMQ
rabbitmq-server

# 3. 启动微服务（自动注册到 Consul）
./userServer &      # 端口 8001
./fileServer &      # 端口 8002

# 4. 启动 HTTP 网关
./server &          # 端口 8888

# 5. 启动 OSS 消费者（可选，用于云备份）
./oss_consumer &
```

访问 `http://localhost:8888` 即可使用。

## 核心数据流

### 用户注册
```
浏览器 ──POST /api/v1/auth/register──► server
                                         │ SRPC Register
                                         ▼
                                      userServer ──MySQL──► 写入 tbl_user
```

### 用户登录
```
浏览器 ──POST /api/v1/auth/login──► server
                                      │ SRPC Login
                                      ▼
                                   userServer ──MySQL──► 查询用户
                                      │                    │
                                      └── 验证密码 + 签发 JWT ◄─┘
```

### 文件上传
```
浏览器 ──POST /api/v1/files──► server
                                 │
                           本地落盘 storage/
                           写入 tbl_file
                                 │
                           发送到 RabbitMQ ──► oss_consumer
                                                 │
                                           阿里云 OSS 上传
```

## 许可证

本项目基于 [MIT License](LICENSE) 开源。
`
