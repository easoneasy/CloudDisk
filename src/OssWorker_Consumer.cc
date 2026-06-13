#include "../include/OssManager.h"
#include "../include/RabbitMQ.h"
#include <csignal>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

// 这个变量随时可能会被外部修改，每次都重新读内存
// static volatile bool running = true;

// void sig_handler(int)
// {
//     running = false;
// }

int main(int argc,char *argv[])
{
    // signal(SIGINT, sig_handler);
    // 初始化Rabbit消费者
    RabbitMQ::getInstance().startConsumer();
    cout << "OSS worker start ..." << endl;
    while(1)
    {
        auto env = RabbitMQ::getInstance().RabbitConsumer();
        if(!env)
        {
            continue;
        }
        // 将env格式的数据序列化成json
        auto data = json::parse(env->Message()->Body());
        // 获取文件名和文件路径
        string filename = data["filename"];
        string path = data["path"];

        // 调用OSSManager上传文件
        bool isUpload = OSSManager::getInstance().upload(filename, path);
        // 判断是否上传成功
        if(isUpload)
        {
            // success
            // 返回ack
            RabbitMQ::getInstance().Ack(env);
            cout << "[consumer] ： upload success ！" << endl;
        }else{
            // failed
            // 给客户端发送云备份失败提示？
            cout << "[consumer] ： upload failed ！" << endl;
        }

    }

    return 0;
}
