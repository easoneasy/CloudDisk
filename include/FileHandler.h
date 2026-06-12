#pragma once
#include <wfrest/HttpServer.h>

using namespace std;
using namespace wfrest;

// 查看文件列表
void file_list_handler(const HttpReq *req,HttpResp *resp);

// 上传文件
void file_upload_handler(const HttpReq *req,HttpResp *resp);

// 下载文件
void file_download_handler(const HttpReq *req,HttpResp *resp);
