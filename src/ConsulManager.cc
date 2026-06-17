#include "../include/ConsulManager.h"
#include <chrono>
#include <iostream>
#include <ppconsul/consul.h>
#include <ppconsul/agent.h>
#include <ppconsul/health.h>
#include <string>
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/Workflow.h>

using namespace std;
using ppconsul::Consul;
using namespace ppconsul::agent;
using ppconsul::health::Health;
using namespace ppconsul::kw;

ConsulManager &ConsulManager::getInstance()
{
    static ConsulManager instance;
    return instance;
}

// 注册
bool ConsulManager::registerService(string id,string name,string ip,int port)
{
    // 发送HTTP请求给Consul
    // 采用TTL健康检测机制，生存期为10秒
    // 如果10秒之内服务没有主动向Consul报平安，Consul就会认为该服务挂了，标记为不健康
    _agent.registerService(
        kw::id = id,
        kw::name = name,
        kw::address = ip,
        kw::port = port,
        kw::check = TtlCheck{std::chrono::seconds(10)}
    );
    return true;
}

// 心跳续期
// 显示告诉Consul健康
void ConsulManager::servicePass(string service_id)
{
    // 刷新Consul对应的service_id的TTL计时器，重新倒计时
    _agent.servicePass(service_id);
}
// 根据服务名查找ip+port
ServiceNode ConsulManager::discoverService(string service_name)
{
    // 向Consul获取该服务名下的所有通过了健康检测的实例列表
    // services是个容器
    auto services = _health.service(service_name);
    if(services.empty())
    {
        return {"",0};
    }
    // 拿第一个实例
    auto &serviceInfo = std::get<1>(services.front());
    // 轮询负载均衡版（但没看懂）
    // size_t index = request_count.fetch_add(1) % services.size();
    // auto &serviceInfo = std::get<1>(services[index]);
    // 返回address:port
    return {serviceInfo.address,serviceInfo.port};
}
// 注销，当服务主动关闭时，通知Consul把自己从服务列表中删除
void ConsulManager::deregisterService(const string &service_id)
{
    // 移除该节点
    _agent.deregisterService(service_id);
}
// 自动检测的回调函数
void ConsulManager::timer_callback(WFTimerTask *task,string service_id)
{
    if(task->get_state() != WFT_STATE_SUCCESS)
    {
        return;
    }
    // 向Consul报平安
    ConsulManager::getInstance().servicePass(service_id);
    WFTimerTask *next = WFTaskFactory::create_timer_task(
        "health-check",9,0,
        std::bind(timer_callback,std::placeholders::_1,service_id)
    );
    // 将定时器放在当前序列中
    series_of(task)->push_back(next);
}
// 自动检测
void ConsulManager::startHeartBeat(const string &service_id)
{
    // 防止刚注册完还没到第一次定时器触发就超时
    servicePass(service_id);
    WFTimerTask *task = WFTaskFactory::create_timer_task(
        "health-check",9,0,
        std::bind(timer_callback,std::placeholders::_1,service_id)
    );
    task->start();
}

// 构造函数
ConsulManager::ConsulManager()
:_consul("http://127.0.0.1:8500",dc="dc1")
,_agent(_consul)
,_health(_consul)
{
}
