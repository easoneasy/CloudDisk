#include "../include/fileHandler.srpc.h"
#include "../include/ConsulManager.h"
#include "workflow/WFFacilities.h"
#include <csignal>
#include <ppconsul/consul.h>
#include <srpc/rpc_define.h>
#include <vector>
#include <workflow/MySQLMessage.h>
#include <workflow/MySQLResult.h>
#include <workflow/WFGlobal.h>
#include <workflow/WFTask.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/Workflow.h>
#include <workflow/mysql_types.h>

using namespace srpc;
using namespace std;
using namespace protocol;

static WFFacilities::WaitGroup wait_group(1);
static const string DatabaseURL = "mysql://root:123456@localhost/CloudDisk";
static const int retry_max = 3;

void sig_handler(int signo)
{
    wait_group.done();
}

class fileHandlerService : public fileHandler::Service
{
    public:

        void fileInfo(fileInfoRequest *request, fileListResponse *response, srpc::RPCContext *ctx) override
        {
            // 解析请求
            // 获取uid
            int uid = request->uid();
            // 处理业务逻辑
            // 从数据库中查找文件信息
            WFMySQLTask *mysqlTask = WFTaskFactory::create_mysql_task(DatabaseURL,retry_max,[response,uid](WFMySQLTask *task)
                {
                    // 判断任务是否成功
                    int state = task->get_state();
                    if(state != WFT_STATE_SUCCESS)
                    {
                        response->set_code(500);
                        response->set_msg("服务器内部错误");
                        cerr << WFGlobal::get_error_string(state, task->get_error()) << endl;
                        return;
                    }
                    // 判断返回mysql包的类型
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
                        vector<MySQLCell> record;
                        // 遍历结果集
                        while (cursor.fetch_row(record)) {
                            auto *file = response->add_files();
                            file->set_fileid(record[0].as_int());
                            file->set_filename(record[1].as_string());
                            file->set_filesize(record[2].as_int());
                            file->set_createdat(record[3].as_datetime());
                            file->set_updatedat(record[4].as_datetime());
                        }
                    response->set_code(200);
                    response->set_msg("获取文件列表成功");
                    }
                });

            MySQLRequest *req = mysqlTask->get_req();
            string sql = "select id,filename,size,created_at,last_update from tbl_file where uid = '" + to_string(uid) + "';";
            req->set_query(sql);
            SeriesWork *series = ctx->get_series();
            series->push_back(mysqlTask);
        }
};

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    // 注册信号处理函数
    signal(SIGINT,sig_handler);
    // 使用默认参数，创建srpc服务器
    SRPCServer server;
    // 注册服务
    fileHandlerService service;
    server.add_service(&service);
    if(server.start(8002) == 0)
    {
        // 启动成功，注册服务的信息
        ConsulManager::getInstance().registerService(
            "file-service-1", "file-service", "127.0.0.1", 8002);
        ConsulManager::getInstance().startHeartBeat("file-service-1");
        wait_group.wait();
        ConsulManager::getInstance().deregisterService("file-service-1");
        server.stop();
    }else{
        cerr << "Error : Server start Failed" << endl;
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
