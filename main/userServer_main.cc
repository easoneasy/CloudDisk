#include "../include/userHandler.srpc.h"
#include "../include/CryptoUtil.h"
#include "../include/ConsulManager.h"
#include "workflow/WFFacilities.h"
#include <csignal>
#include <iostream>
#include <srpc/rpc_define.h>
#include <workflow/MySQLMessage.h>
#include <workflow/MySQLResult.h>
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/Workflow.h>
#include <workflow/mysql_types.h>

using namespace srpc;
using namespace std;
using namespace protocol;

static const string DatabaseURL = "mysql://root:123456@localhost/CloudDisk";
static int retry_max = 3;

static WFFacilities::WaitGroup wait_group(1);

void sig_handler(int signo)
{
    wait_group.done();
}

class userHandlerService : public userHandler::Service
{
    public:
        void Register(RegisterRequest *request, RegisterResponse *response, srpc::RPCContext *ctx) override
        {
            // 解析请求
            string username = request->username();
            string password = request->password();
            // 处理业务逻辑
            // 创建MySQL任务
            WFMySQLTask *mysqlTask = WFTaskFactory::create_mysql_task(DatabaseURL,retry_max,
                [response,username](WFMySQLTask *task)
                {
                    // 判断任务是否成功
                    int state = task->get_state();
                    if(state != WFT_STATE_SUCCESS)
                    {
                        // mysql任务失败，返回错误代码500
                        response->set_code(500);
                        response->set_msg("服务器内部错误");
                        cerr << WFGlobal::get_error_string(state, task->get_error()) << endl;
                        return ;
                    }
                    // 判断返回包的类型 检查SQL是否执行失败
                    MySQLResponse *resp = task->get_resp();
                    if(resp->get_packet_type() == MYSQL_PACKET_ERROR)
                    {
                        response->set_code(409);
                        response->set_msg("用户已存在");
                        cerr << "error_code: " << resp->get_error_code()
                        << ", error_msg: " << resp->get_error_msg() << endl;
                        return;
                    }
                    // 处理结果集
                    MySQLResultCursor cursor(resp);
                    if(cursor.get_cursor_status() == MYSQL_STATUS_OK && cursor.get_affected_rows() == 1)
                    {
                        // 处理成功
                        int user_id = cursor.get_insert_id(); // 获取插入记录的id
                        // 返回给client
                        response->set_user_id(user_id);
                        response->set_username(username);
                        response->set_code(200);
                        response->set_msg("注册成功");
                    }
                });
            // 设置任务：指定SQL语句
            MySQLRequest *req = mysqlTask->get_req();
            // 随机生成盐值
            string salt = CryptoUtil::generate_salt();
            // 将盐值与密码哈希
            string hashcode = CryptoUtil::hash_password(password, salt);
            // 存入数据库，写一个sql语句
            string sql = "insert into tbl_user (username,password,salt) values ('"
                + username +"','"
                + hashcode + "','"
                + salt +"');";
            req->set_query(sql);
            // 添加到任务序列中
            // 通过上下文获取处理RPC请求的任务序列
            SeriesWork *series = ctx->get_series();
            series->push_back(mysqlTask);
        }

        void Login(LoginRequest *request, LoginResponse *response, srpc::RPCContext *ctx) override
        {
            // 获取用户名、密码
            string username = request->username();
            string password = request->password();
            // 创建MySQL任务
            WFMySQLTask *mysqlTask = WFTaskFactory::create_mysql_task(DatabaseURL,retry_max,
                [response,username,password](WFMySQLTask *task)
                {
                    // 判断任务是否成功
                    int state = task->get_state();
                    if(state != WFT_STATE_SUCCESS)
                    {
                        // mysql任务失败，返回错误代码500
                        response->set_code(500);
                        response->set_msg("服务器内部错误");
                        cerr << WFGlobal::get_error_string(state, task->get_error()) << endl;
                        return ;
                    }
                    // 判断返回包的类型 检查SQL是否执行失败
                    MySQLResponse *resp = task->get_resp();
                    if(resp->get_packet_type() == MYSQL_PACKET_ERROR)
                    {
                        response->set_code(500);
                        response->set_msg("服务器内部错误");
                        cerr << "error_code: " << resp->get_error_code()
                        << ", error_msg: " << resp->get_error_msg() << endl;
                        return;
                    }
                    // 处理结果集
                    MySQLResultCursor cursor(resp);
                    if(cursor.get_cursor_status() == MYSQL_STATUS_GET_RESULT)
                    {
                        // 处理成功
                        // 使用MySQLResultCursor遍历结果集
                        vector<MySQLCell> record;
                        if(!cursor.fetch_row(record))
                        {
                            response->set_code(401);
                            response->set_msg("用户名或密码错误");
                            return;
                        }
                        string db_password = record[1].as_string();
                        string salt = record[2].as_string();
                        string hashcode = CryptoUtil::hash_password(password, salt);
                        if(hashcode != db_password)
                        {
                            // 密码错误，返回错误信息
                            response->set_code(401);
                            response->set_msg("用户名或密码错误");
                            return;
                        }
                        User user;
                        user.id = record[0].as_int();
                        user.username = username;
                        user.password = record[1].as_string();
                        user.salt = record[2].as_string();
                        user.createdAt = record[3].as_datetime();
                        // 密码校对正确，返回token
                        string token = CryptoUtil::generate_token(user);
                        response->set_token(token);
                        response->set_tokentype("Bearer");
                        response->set_code(200);
                        response->set_msg("登录成功");
                        response->set_user_id(user.id);
                        response->set_username(username);
                        return;
                    }
                });
            // 根据用户名查找hashcode、salt
            // 根据找到的salt，与password哈希得到hashcode对比两个hashcode是否相同
            // 相同返回token，不同报错
            // 设置任务：指定SQL语句
            MySQLRequest *req = mysqlTask->get_req();
            // 存入数据库，写一个sql语句
            string sql = "select id,password,salt,created_at from tbl_user where username = '" + username +"';";
            req->set_query(sql);
            // 添加到任务序列中
            // 通过上下文获取处理RPC请求的任务序列
            SeriesWork *series = ctx->get_series();
            series->push_back(mysqlTask);
        }

        void UserInfo(UserInfoRequest *request, UserInfoResponse *response, srpc::RPCContext *ctx) override
        {
            // TODO: fill server logic here
        }
};

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    signal(SIGINT, sig_handler);
    srand(time(NULL)); // 设置随机种子

    // 注册信号处理函数
    signal(SIGINT, sig_handler);
    // 使用默认参数，创建srpc服务器
    SRPCServer server;
    // 注册服务
    userHandlerService service;
    server.add_service(&service);

    // 启动SRPCServer
    if(server.start(8001) == 0)
    {
        ConsulManager::getInstance().registerService(
            "user-service-1", "user-service", "127.0.0.1", 8001);
        ConsulManager::getInstance().startHeartBeat("user-service-1");
        wait_group.wait();
        ConsulManager::getInstance().deregisterService("user-service-1");
        server.stop();
    }else{
        cerr << "Error : Server Start Failed" << endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
