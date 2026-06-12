#pragma once
#include <SimpleAmqpClient/BasicMessage.h>
#include <SimpleAmqpClient/Channel.h>
#include <SimpleAmqpClient/Envelope.h>

using namespace std;
using namespace AmqpClient;

// 单例
class RabbitMQ
{
public:
    // 创建单例对象
    static RabbitMQ &getInstance();
    // 生产者函数
    bool RabbitProducer(const string &fileName,const string localPath);
    // 消费者函数
    Envelope::ptr_t RabbitConsumer();

    // 创建消费者函数
    void startConsumer();

    // ACK接口
    void Ack(Envelope::ptr_t env);

private:
    // 构造函数
    RabbitMQ();
    // 析构函数
    ~RabbitMQ() = default;
    // 删除复制控制函数
    RabbitMQ(const RabbitMQ&) = delete;
    RabbitMQ &operator=(const RabbitMQ&) = delete;

private:
    string _uri;
    Channel::ptr_t _channel;
    string _consumerTag;
};
