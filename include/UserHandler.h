#pragma once
#include <wfrest/HttpServer.h>

using namespace wfrest;


// 注册
void register_handler(const HttpReq *req, HttpResp *resp);

// 登录
void login_handler(const HttpReq *req, HttpResp *resp);

// 获取用户信息
void user_handler(const HttpReq *req, HttpResp *resp);
