// #include "../include/CryptoUtil.h"
#include "../include/CloudDiskServer.h"
#include "../include/UserHandler.h"
#include "../include/FileHandler.h"
#include <wfrest/HttpServer.h>
#include <wfrest/HttpDef.h>
#include <wfrest/HttpMsg.h>
#include <wfrest/PathUtil.h>
#include <wfrest/MysqlUtil.h>
#include <nlohmann/json.hpp>
#include <alibabacloud/oss/OssClient.h>


using namespace std;
using namespace std::placeholders;
using namespace wfrest;
using namespace protocol;
using json = nlohmann::json;
using namespace AlibabaCloud::OSS;

// 数据库的URL
static const string DatabaseURL = "mysql://root:123456@localhost/CloudDisk";
static const int RetryMax = 3;
static const string STORAGE_DIR ="./storage";

void CloudDiskServer::register_routes()
{
    // 设置静态资源的路由
    register_www_module();
    register_auth_module();
    register_user_module();
    register_file_module();
}

void CloudDiskServer::register_www_module()
{
    server_.Static("/", "./www/index.html");
    server_.Static("/static", "./www/static");
}

// 注册认证模块的路由
void CloudDiskServer::register_auth_module()
{
    server_.POST("/api/v1/auth/register",register_handler);
    server_.POST("/api/v1/auth/login",login_handler);
}

void CloudDiskServer::register_user_module()
{
    server_.GET("/api/v1/user/me",user_handler);
}

void CloudDiskServer::register_file_module()
{
    server_.GET("/api/v1/files",file_list_handler);
    server_.POST("/api/v1/files",file_upload_handler);
    server_.GET("/api/v1/file/{id}",file_download_handler);
}
