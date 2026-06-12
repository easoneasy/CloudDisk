#include "../include/RabbitMQ.h"
#include <iostream>
#include <SimpleAmqpClient/BasicMessage.h>
#include <SimpleAmqpClient/Channel.h>
#include <SimpleAmqpClient/Envelope.h>
#include <fcntl.h>
#include <nlohmann/json.hpp>

using namespace std;
using namespace AmqpClient;
using json = nlohmann::json;

RabbitMQ &RabbitMQ::getInstance()
{
    static RabbitMQ instance;
    return instance;
}

// 构造函数
RabbitMQ::RabbitMQ()
:_uri("amqp://guest:guest@localhost:5672/%2f")
{
    // 创建channel
    _channel = Channel::CreateFromUri(_uri);
}

// 生产者函数
bool RabbitMQ::RabbitProducer(const string &fileName,const string localPath)
{
    json data;
            data["filename"] = fileName;
            data["path"] = localPath;
            // 构建消息
            BasicMessage::ptr_t message = BasicMessage::Create(data.dump());
            // 发送消息
            string exchange = "oss.direct";
            string routingKey = "oss";
            // 发布消息
            _channel->BasicPublish(exchange, routingKey, message);
            return true;
}

// 消费者函数
// typedef boost::shared_ptr<Envelope> ptr_t;
//ptr_t 是类型别名（指针的类型别名）
Envelope::ptr_t RabbitMQ::RabbitConsumer()
{
    // 检查是否有消费者对象
    if(_consumerTag.empty())
    {
        return nullptr;
    }
    // 订阅队列
    Envelope::ptr_t envelope;
    // 把消息放入envelope
    _channel->BasicConsumeMessage(_consumerTag,envelope);
    // 把收到的消息返回给调用者
    return envelope;
}


// 创建消费者函数
void RabbitMQ::startConsumer()
{
    _consumerTag = _channel->BasicConsume("oss.queue","",false,false,false,1);
}

// ACK接口
void RabbitMQ::Ack(Envelope::ptr_t env)
{
    _channel->BasicAck(env);
}
