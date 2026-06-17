#pragma once
#include <wfrest/HttpServer.h>
#include <workflow/Workflow.h>

using namespace wfrest;


// 注册
void register_handler(const HttpReq *req, HttpResp *resp,SeriesWork *series);

// 登录
void login_handler(const HttpReq *req, HttpResp *resp,SeriesWork *series);

// 获取用户信息
void user_handler(const HttpReq *req, HttpResp *resp);
