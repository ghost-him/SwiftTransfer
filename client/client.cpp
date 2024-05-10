//
// Created by ghost-him on 24-5-5.
//

#include "client.h"
#include "../AppConfig/AppConfig.h"
#include "../FileManager/FileManager.h"
#include "../database/database.h"
#include <codecvt>
#include "../ThreadPool/threadpool.h"


void StreamServiceClient::getConfiguration() {
    Placeholder placeHolder;
    Configuration reply;
    ClientContext context;

    Status status = stub_->getConfiguration(&context, placeHolder, &reply);
    if (status.ok()) {
        // 调用成功
        AppConfig& config = AppConfig::getInstance();
        config.set("fileBlockSize", std::to_string(reply.fileblocksize()));
        config.set("clientID", std::to_string(reply.clientid()));
    } else {
        throw std::runtime_error("grpc request failed: " + std::to_string(status.error_code()) + ", error message: " + status.error_message());
    }
}

FileList StreamServiceClient::getFileList(uint32_t page) {
    GetFileListPage getFileListPage;
    FileList reply;
    ClientContext context;

    getFileListPage.set_page(page);

    Status status = stub_->getFileList(&context, getFileListPage, &reply);
    if (status.ok()) {
        // 获取文件列表成功
    } else {
        throw std::runtime_error("grpc request failed: " + std::to_string(status.error_code()) + ", error message: " + status.error_message());
    }
    return std::move(reply);
}

uint32_t StreamServiceClient::getUniqueID() {
    Placeholder placeHolder;
    UniqueID reply;
    ClientContext context;

    uint32_t ret = 0;
    Status status = stub_->getUniqueID(&context, placeHolder, &reply);
    if (status.ok()) {
        // 获取ID成功
        ret = reply.uniqueid();
    } else {
        throw std::runtime_error("grpc request failed: " + std::to_string(status.error_code()) + ", error message: " + status.error_message());
    }
    return ret;
}

void StreamServiceClient::HeartbeatSignal() {
    Placeholder placeHolder;
    Placeholder reply;
    ClientContext context;

    AppConfig& config = AppConfig::getInstance();
    context.AddMetadata("clientID", config.get("clientID"));
    Status status = stub_->HeartbeatSignal(&context, placeHolder, &reply);
    if (status.ok()) {
       // 发送心跳成功
    } else {
        throw std::runtime_error("grpc request failed: " + std::to_string(status.error_code()) + ", error message: " + status.error_message());
    }
}

void StreamServiceClient::uploadFile(const std::filesystem::path &path) {
    AppConfig& config = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();
    uint32_t fileBlockSize = std::stoi(config.get("fileBlockSize"));
    uint32_t transferID = sendUploadFileInfo(path);
    database& db = database::getInstance();
    if (transferID == -1) {
        return ;
    }

    Transfer::FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    std::shared_ptr<SendFileList> sendFileList= std::make_shared<SendFileList>();
    sendFileListMap[transferID] = sendFileList;

    // 根据文件的大小，初始化要发送的文件块
    uint32_t totalFileBlockSize = (fileInfo.filesize() / fileBlockSize) + 1;

    fileManager.createFileDigestList(fileInfo.fileid(), totalFileBlockSize);
    ThreadPool::getInstance().commit(std::bind_front(&FileManager::updateFileDigestThread, &FileManager::getInstance()), fileInfo.fileid());

    for (int i = 0; i < totalFileBlockSize; i ++) {
        sendFileList->FileBlockToSend.push(i);
    }

    while(true) {
        if (!sendUploadFileBlock(transferID)) break;
        if (!sendFileVerification(transferID)) break;

        // 检测当前是否还有没发送完的
        if (isTransferEnd(sendFileList)) {
            break;
        }
    }

    endSendUploadFile(transferID);

    fileManager.deleteFileDigestList(fileInfo.fileid());
    sendFileListMap.erase(transferID);
}

void StreamServiceClient::downloadFile(uint32_t fileID) {

}

uint32_t StreamServiceClient::sendUploadFileInfo(const std::filesystem::path& path) {
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    database& db = database::getInstance();
    // 获取一个传输id
    uint32_t transferID = getUniqueID();
    // 初始化文件管理器
    fileManager.startReadFile(path, transferID);
    uint64_t fileSize = fileManager.getFileSize(transferID);
    uint32_t clientID = std::stoi(appConfig.get("clientID"));
    uint32_t fileID = getUniqueID();

    std::wstring wfileName = path.filename().wstring();
    // 将wstring 转为 string
    std::string fileName = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(wfileName);

    Transfer::StartTransfer startTransfer;
    startTransfer.set_clientid(clientID);
    startTransfer.set_transferid(transferID);
    Transfer::FileInfo* fileInfo = startTransfer.mutable_fileinfo();
    fileInfo->set_filesize(fileSize);
    fileInfo->set_filename(fileName);
    fileInfo->set_fileid(fileID);

    db.createTransfer(transferID, *fileInfo);
    Transfer::Placeholder placeholder;
    ClientContext context;
    Status status = stub_->sendUploadFileInfo(&context, startTransfer, &placeholder);
    if (status.ok()) {
        return transferID;
    } else {
        return -1;
    }
}

bool StreamServiceClient::sendUploadFileBlock(uint32_t transferID) {
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    database& db = database::getInstance();

    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));
    Transfer::FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    // 文件的总块数
    uint32_t blockNumber = (fileInfo.filesize() / fileBlockSize) + 1;

    ClientContext context;
    Placeholder placeholder;

    std::unique_ptr<ClientWriter<FileBlock>> writer(stub_->sendUploadFileBlock(&context, &placeholder));
    std::shared_ptr<std::thread> threads[Default_Send_Thread];

    std::mutex serverWriteLock;
    for (int i = 0; i < Default_Send_Thread; i ++) {
        std::shared_ptr<std::thread> ptr = std::make_shared<std::thread>([&]{
            while(true) {
                uint32_t fileBlockIndex = getNextBlockIndex(sendFileListMap[transferID]);
                if (fileBlockIndex == -1) {
                    // 传输结束
                    break;
                }
                uint64_t offset = fileBlockIndex * fileBlockSize;
                auto ret = fileManager.readFileViaID(transferID, offset, fileBlockSize);
                Transfer::FileBlock fileBlock = ret.first;
                 // 计算一下哈希值
                fileManager.updateFileDigestList(fileInfo.fileid(), fileBlockIndex, ret.second, fileBlock.filesize());
                std::unique_lock<std::mutex> guard(serverWriteLock);
                if (!writer->Write(fileBlock)) {
                    // 传输失败
                    break;
                }
            }
        });
        threads[i] = ptr;
    }

    for (auto & thread : threads) {
        thread->join();
    }



    writer->WritesDone();
    Status status = writer->Finish();
    if (status.ok()) {
        return true;
    }
    return false;
}

bool StreamServiceClient::sendFileVerification(uint32_t transferID) {
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    database& db = database::getInstance();
    FileInfo fileInfo = db.queryTransfer(transferID);

    Transfer::FileDigest fileDigest = std::move(fileManager.getFileDigest(fileInfo.fileid()));
    std::cerr << fileDigest.digest() << std::endl;
    fileDigest.set_transferid(transferID);
    ClientContext context;
    RequestFileBlockList reply;
    Status status = stub_->sendFileVerification(&context, fileDigest, & reply);
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));
    if (status.ok()) {
        std::shared_ptr<SendFileList> ptr = sendFileListMap[transferID];
        std::unique_lock<std::mutex> guard(ptr->lock);
        for (int i = 0; i < reply.offset_size(); i ++) {
            uint64_t offset = reply.offset(i);
            // 返回-1表示当前文件完整
            if (offset == -1)
                break;
            uint32_t blockIndex = offset / fileBlockSize;
            ptr->FileBlockToSend.push(blockIndex);
        }
    } else {
        return false;
    }
    return true;
}

bool StreamServiceClient::isTransferEnd(std::shared_ptr<SendFileList> sendFileList) {
    if (sendFileList->FileBlockToSend.empty())
        return true;
    return false;
}

bool StreamServiceClient::endSendUploadFile(uint32_t transferID) {
    FileManager::getInstance().endReadFile(transferID);
    database::getInstance().endTransfer(transferID);


    Transfer::EndTransfer endTransfer;
    endTransfer.set_transferid(transferID);
    Placeholder placeholder;
    ClientContext context;
    Status status = stub_->endSendUploadFile(&context, endTransfer, &placeholder);
    if (status.ok()) {
        return true;
    }
    return false;
}

uint32_t StreamServiceClient::getNextBlockIndex(std::shared_ptr<SendFileList> ptr) {
    std::unique_lock<std::mutex> guard(ptr->lock);
    // 如果所有的都已经发送结束了，则返回
    if (ptr->FileBlockToSend.empty())
        return -1;
    uint32_t blockIndex = ptr->FileBlockToSend.front();
    ptr->FileBlockToSend.pop();
    return blockIndex;
}

void StreamServiceClient::connect(const string &address) {
    channel_ = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    stub_ = StreamService::NewStub(channel_);
}
