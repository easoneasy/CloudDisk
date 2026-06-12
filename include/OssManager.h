#pragma once
#include <iostream>
#include <alibabacloud/oss/OssClient.h>
#include <istream>

using namespace std;
using namespace std::placeholders;
using namespace AlibabaCloud::OSS;

class OSSManager
{
    public:
        static OSSManager &getInstance();
        // 析构函数
        ~OSSManager();
		// 上传
        bool upload(const string &objectName,const string &data);
		// 下载
        bool download(const string &objectName,string &data);

    private:
        // 删除复制控制的函数
        OSSManager(const OSSManager &) = delete;
        OSSManager &operator=(const OSSManager &) = delete;

        // 构造函数私有
        OSSManager();

    private:
        // 保存一个OssClient对象
        // 保证这个对象在OSSManager整个生命周期内一直存在
        unique_ptr<OssClient> _client;
        string _bucketName;
};
