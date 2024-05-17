#include "server.h"
#include "../AppConfig/AppConfig.h"
#include "ServerView/ServerView.h"
#include <grpcpp/grpcpp.h>
#include "../ThreadPool/threadpool.h"
#include "../FileManager/FileManager.h"
#include "../database/database.h"

using namespace grpc;

int main() {

    ThreadPool& threadPool = ThreadPool::getInstance();
    AppConfig& config = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();
    /////////////
    StreamServiceImpl service;
    service.initFileDirectory();

    database::getInstance().init();
    // 初始化文件管理器
    threadPool.commit([&](){
        fileManager.writeThread();
    });

    ////////////////
    ServerView::getInstance().printInitPage();

    std::string server_address("127.0.0.1:12345");

    ServerBuilder builder;

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(reinterpret_cast<Service *>(&service));
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
    fileManager.close();
}