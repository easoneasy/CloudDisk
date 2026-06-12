#include "../include/CryptoUtil.h"
#include "../include/UserHandler.h"
#include <wfrest/HttpServer.h>
#include <wfrest/HttpDef.h>
#include <wfrest/HttpMsg.h>
#include <wfrest/PathUtil.h>
#include <wfrest/MysqlUtil.h>
#include <nlohmann/json.hpp>

using namespace std;
using namespace wfrest;
using namespace protocol;
using json = nlohmann::json;

static const string DatabaseURL = "mysql://root:123456@localhost/CloudDisk";
static const int RetryMax = 3;

// 辅助函数
// 发送成功内容
static void send_success(HttpResp *resp,const int status_code,const string msg,const json data)
{
    json body;
    body["status"] = "success";
    body["message"] = msg;
    body["data"] = data;
    resp->set_status(status_code);
    resp->add_header_pair("Content-Type","application/json");
    resp->append_output_body(body.dump());
}
// 发送失败内容
static void send_error(HttpResp *resp,const int status_code,const string msg)
{
    json body;
    body["status"] = "error";
    body["message"] = msg;
    resp->set_status(status_code);
    resp->add_header_pair("Content-Type","application/json");
    resp->append_output_body(body.dump());
}


// 注册
void register_handler(const HttpReq *req,HttpResp *resp)
{
    // 客户端发错格式(不是json格式)
    if(req->content_type() != wfrest::APPLICATION_JSON)
    {
        resp->set_status(HttpStatusBadRequest);
        json msg = {{"status","error"},{"message","请求格式有误"}};
        resp->append_output_body(msg.dump());
        return;
    }
    // 获取请求体
    // string body = req->body();
    // json data = json::parse(body);
    auto &data = req->json();
    string username = data["username"];
    string password = data["password"];
    string confirm = data["confirm"];

    // 错误校验
    if(confirm != password)
    {
        resp->set_status(HttpStatusBadRequest);
        json msg = {{"status","error"},{"message","两次输入的密码不一致"}};
        resp->append_output_body(msg.dump());
        return;
    }
    if(username.empty() || password.empty())
    {
        resp->set_status(HttpStatusBadRequest);
        json msg={{"status","error"},{"message","用户名和密码不能为空"}};
        resp->append_output_body(msg.dump());
        return;
    }
    // 随机生成盐值
    string salt = CryptoUtil::generate_salt();
    // 将盐值与密码哈希
    string hashcode = CryptoUtil::hash_password(password, salt);
    // 存入数据库，写一个sql语句
    string sql = "insert into tbl_user (username,password,salt) values ('"
        + username +"','"
        + hashcode + "','"
        + salt +"');";
    resp->MySQL(DatabaseURL,sql,[resp,username](MySQLResultCursor *cursor)
        {
            if(cursor->get_cursor_status() == MYSQL_STATUS_OK && cursor->get_affected_rows()==1)
            {
                // 成功
                json data;
                data["userId"]= cursor->get_insert_id();
                data["username"] = username;
                send_success(resp,201,"注册成功",data);
            }else {
                // 失败
                send_error(resp, 409, "用户名已存在");
            }
        });
}

// 登录
void login_handler(const HttpReq *req,HttpResp *resp)
{
    // 判断客户端发送格式
    if(req->content_type() != wfrest::APPLICATION_JSON)
    {
        send_error(resp, 400, "请求格式有误");
        return;
    }
    // 获取请求体
    // string body = req->body();
    // json data = json::parse(body);
    auto &data = req->json();
    string username = data["username"];
    string password = data["password"];
    // 通过用户名查找hashcode、salt
    string sql = "select * from tbl_user where username = '" + username +"';";

    resp->MySQL(DatabaseURL,sql,[resp,password](MySQLResultCursor *cursor)
        {
            if(cursor->get_cursor_status() != MYSQL_STATUS_GET_RESULT ||cursor->get_rows_count()!=1)
            {
                send_error(resp, 500, "内部服务器错误");
                return;
            }
            // 获取数据库中的记录
            vector<MySQLCell> record;
            if(cursor->fetch_row(record))
            {
                User user;
                user.id = record[0].as_int();
                user.username = record[1].as_string();
                user.password = record[2].as_string();
                user.salt = record[3].as_string();
                user.createdAt = record[4].as_datetime();
                // 用户名或密码为空
                if(user.username.empty() || user.password.empty())
                {
                    send_error(resp, 400, "用户名和密码不能为空");
                }
                string hashcode = CryptoUtil::hash_password(password, user.salt);
                if(hashcode == user.password)
                {
                    // 密码校对正确，返回token
                    string token = CryptoUtil::generate_token(user);
                    json data;
                    data["accessToken"] = token;
                    data["tokenType"] = "Bearer";
                    data["user"]["userId"] = user.id;
                    data["user"]["username"] = user.username;
                    send_success(resp, 200, "登录成功", data);
                    return;
                }else{
                    send_error(resp, 401, "用户名或密码错误");
                }
            }
            // 用户不存在/空结果集/密码错误
            // SQL返回空结果集
            send_error(resp, 401, "用户名或密码错误");
        });
}

void user_handler(const HttpReq *req,HttpResp *resp)
{
    // 获取头部的Authorization的值 "Bearer token...."
    const string &auth = req->header("Authorization");
    // 没有令牌/令牌的类型不是"Bearer"
    if(auth.size() < 8 || auth.substr(0,7) != "Bearer ")
    {
        send_error(resp, 401, "无效的访问令牌");
        return;
    }
    // 获取token  去掉"Bearer "
    string token = auth.substr(7);
    // 创建user对象
    User user;
    // 验证token
    // 检查JWT有没有被篡改、解析token的用户数据，填充给user对象
    // 在生成token时，就已经把用户的信息放入token中了，省去了访问数据库的步骤
    if(!CryptoUtil::verify_token(token, user))
    {
        send_error(resp, 401, "无效的访问令牌");
        return;
    }
    // 走到这user对象已经有用户数据了
    // 返回用户信息
    json data;
    data["userId"] = user.id;
    data["username"] = user.username;
    data["createdAt"] = user.createdAt;
    send_success(resp, 200, "获取个人信息成功", data);
}
