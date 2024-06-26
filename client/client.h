//
// Created by ghost-him on 24-5-5.
//

#ifndef SWIFTTRANSFER_CLIENT_H
#define SWIFTTRANSFER_CLIENT_H

#include <grpcpp/grpcpp.h>
#include "../grpc/service.grpc.pb.h"
#include <string>
#include <queue>


using namespace grpc;
using namespace Transfer;

#define Default_Send_Thread 5
#define Screen_Refresh_Per_Sec 5;

class StreamServiceClient {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    StreamServiceClient(StreamServiceClient const&) = delete;
    StreamServiceClient& operator=(StreamServiceClient const&) = delete;
    // 提供一个全局唯一的接口
    static StreamServiceClient& getInstance() {
        static StreamServiceClient instance;
        return instance;
    }

    void connect(const std::string& address);

    // 获取配置文件
    void getConfiguration();
    // 获取文件列表
    FileList getFileList(uint32_t page);
    // 获取全局唯一id
    uint32_t getUniqueID();

    /*
     * 上传文件相关
     */
    void uploadFile(const std::filesystem::path& path);

    /*
     * 下载文件相关
     */
    void downloadFile(uint32_t fileID);

    void initFileDirectory();

    void refreshScreen(uint32_t transferID);


private:
    const std::string fileDirectoryPath = "./files/";
    StreamServiceClient();
    struct SendFileList {
        std::queue<uint32_t> FileBlockToSend;
        std::mutex lock;
    };
    /*
     * 上传文件相关
     */
    // 发送文件的信息
    uint32_t sendUploadFileInfo(const std::filesystem::path& path);
    // 上传文件
    bool sendUploadFileBlock(uint32_t transferID);
    // 发送效验文件
    bool transferUploadFileSHA(uint32_t transferID);
    // 结束一个文件的上传
    bool endSendUploadFile(uint32_t transferID);
    // 获取下一个应该发送的文件块的编号
    uint32_t getNextBlockIndex(std::shared_ptr<SendFileList> ptr);
    // 检测是否存在没发送完的
    bool isTransferEnd(std::shared_ptr<SendFileList> sendFileList);


    /*
     * 下载文件相关
     */
    // 开始获得要下载文件的信息
    uint32_t startDownloadServerFile(uint32_t fileIndex);
    // 下载文件
    bool transferDownloadFile(uint32_t transferID);
    // 结束传输
    void endDownloadFile(uint32_t transferID);
    //  发送文件校验
    int transferDownloadFileSHA(uint32_t transferID);

    /*
     * 传输显示相关
     */
    struct FileViewInfo {
        std::chrono::steady_clock::time_point lastPrintTime =  std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::atomic<uint32_t> sendBlockNumber;
        std::mutex lock;
    };

    // 用于辅助计算传输的速度
    std::unordered_map<uint32_t, std::shared_ptr<FileViewInfo>> viewMap;


    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<StreamService::Stub> stub_;

    std::unordered_map<uint32_t, std::shared_ptr<SendFileList>> sendFileListMap;

    // first: 数值,second: 1:B, 2: KB 3: MB 4: GB
    std::pair<double, std::wstring> unitConvert(double number);
};


#endif //SWIFTTRANSFER_CLIENT_H
