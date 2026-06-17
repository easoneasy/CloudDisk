#pragma once
#include "../include/userHandler.srpc.h"
#include "../include/fileHandler.srpc.h"
#include "../include/ConsulManager.h"
#include "ConsulManager.h"
#include "userHandler.srpc.h"
#include <stdexcept>
#include <wfrest/HttpServer.h>
#include <iostream>

using namespace std;
using namespace std::placeholders;
using namespace wfrest;
using namespace protocol;

// 面向对象：将专业的事情交给专业的“人”去做

// 设计原则：武学心法
// 设计模式：招数

// 设计原则：组合优于继承
//     组合：有选择的复用代码
//     继承：会复用基类的所有代码，代码复用的技术！

// 装饰器模式（Wrapper）: 组合
//     保持接口一致，可以降低用户的学习成本
class CloudDiskServer {
    public:
        CloudDiskServer() { }

        // 注册路由
        void register_routes();

        // 包装了一层: 要保证包装后的接口与原来的接口一致！
        int start(unsigned short port)
        {
            return server_.start(port);
        }

        void stop() { server_.stop(); }

        void list_routes() { server_.list_routes(); }

        // 获取SRPC客户端
        static userHandler::SRPCClient &userSRPCClient()
        {
            auto node = ConsulManager::getInstance().discoverService("user-service");
            static userHandler::SRPCClient client(node.host.c_str(),node.port);
            // 判断是否获取ip、port成功
            if(node.host.empty())
            {
                throw runtime_error("user-service not found");
            }
            return client;
        }
        // 获取fileSRPC客户端
        static fileHandler::SRPCClient &fileSRPCClient()
        {
            auto node = ConsulManager::getInstance().discoverService("file-service");
            static fileHandler::SRPCClient client(node.host.c_str(),node.port);
            // 判断是否获取ip、port成功
            if(node.host.empty())
            {
                throw runtime_error("file-service not found");
            }
            return client;
        }

    private:
        // 注册路由
        void register_www_module();
        void register_auth_module();
        void register_user_module();
        void register_file_module();

    private:
        // 名字中最好不要带具体的实现细节
        // 方便以后修改具体的实现
        wfrest::HttpServer server_; // 组合
};
