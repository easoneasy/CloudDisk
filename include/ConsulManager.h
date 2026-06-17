#pragma once
#include <workflow/WFTaskFactory.h>
#include <ppconsul/consul.h>
#include <ppconsul/agent.h>
#include <ppconsul/health.h>
#include <string>

using namespace std;
using ppconsul::Consul;
using namespace ppconsul::agent;
using ppconsul::health::Health;
using namespace ppconsul::kw;

struct ServiceNode
{
    string host;
    unsigned short port;
};

class ConsulManager
{
    public:
        // 获取单例对象
        static ConsulManager &getInstance();
        // 注册，把微服务的信息记录到Consul注册中心里
        bool registerService(string id,string name,string ip,int port);
        // 心跳续期
        void servicePass(string service_id);
        // 根据服务名查找ip+port
        ServiceNode discoverService(string service_name);
        // 注销id
        void deregisterService(const string &service_id);

        // 自动检测的回调函数
        static void timer_callback(WFTimerTask *task,string service_id);

        // 自动检测
        void startHeartBeat(const string &service_id);
    private:
        // 构造函数
        ConsulManager();
        // 删除复制类型函数
        ConsulManager(const ConsulManager&) = delete;
        ConsulManager &operator=(const ConsulManager&) = delete;
    private:
        Consul _consul;
        Agent _agent;
        Health _health;
};
