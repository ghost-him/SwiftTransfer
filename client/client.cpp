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
    PageIndex pageIndex;
    FileList reply;
    ClientContext context;

    pageIndex.set_page(page);

    Status status = stub_->getFileList(&context, pageIndex, &reply);
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

    fileManager.createFileDigestList(transferID, totalFileBlockSize);
    ThreadPool::getInstance().commit(std::bind_front(&FileManager::updateFileDigestThread, &FileManager::getInstance()), transferID);

    for (int i = 0; i < totalFileBlockSize; i ++) {
        sendFileList->FileBlockToSend.push(i);
    }

    while(true) {
        if (!sendUploadFileBlock(transferID)) break;
        if (!transferUploadFileSHA(transferID)) break;

        // 检测当前是否还有没发送完的
        if (isTransferEnd(sendFileList)) {
            break;
        }
    }

    endSendUploadFile(transferID);

    fileManager.deleteFileDigestList(transferID);
    sendFileListMap.erase(transferID);
}

void StreamServiceClient::downloadFile(uint32_t fileID) {

    uint32_t transferID = startDownloadServerFile(fileID);

    if (transferID == -1) {
        return;
    }

    if (!transferDownloadFile(transferID)) {
        return;
    }

    bool end = false;
    while(!end) {
        int ret = transferDownloadFileSHA(transferID);
        switch(ret) {
            case -1:
                return;
            case 0:
                end = true;
                break;
            default:
                end = false;
                break;
        }
    }
    endDownloadFile(transferID);
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

    std::mutex clientWriteLock;
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
                fileManager.updateFileDigestList(transferID, fileBlockIndex, ret.second, fileBlock.filesize());
                std::unique_lock<std::mutex> guard(clientWriteLock);
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

bool StreamServiceClient::transferUploadFileSHA(uint32_t transferID) {
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    database& db = database::getInstance();
    FileInfo fileInfo = db.queryTransfer(transferID);

    Transfer::FileDigest fileDigest = std::move(fileManager.getFileDigest(transferID));
    std::cerr << fileDigest.digest() << std::endl;
    fileDigest.set_transferid(transferID);
    ClientContext context;
    RequestFileBlockList reply;
    Status status = stub_->transferUploadFileSHA(&context, fileDigest, & reply);
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


    Transfer::TransferID transferId;
    transferId.set_transferid(transferID);
    Placeholder placeholder;
    ClientContext context;
    Status status = stub_->endSendUploadFile(&context, transferId, &placeholder);
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

uint32_t StreamServiceClient::startDownloadServerFile(uint32_t fileIndex) {
    FileManager& fileManager = FileManager::getInstance();
    database& db = database::getInstance();
    FileInfo fileInfo = std::move(fileManager.getServerFileInfo(fileIndex));
    AppConfig& appConfig = AppConfig::getInstance();
    ThreadPool& pool = ThreadPool::getInstance();
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));

    // 获得传输id
    FileID fileID;
    fileID.set_fileid(fileInfo.fileid());
    TransferID transferID;
    ClientContext context;

    Status status = stub_->getDownloadTransferID(&context, fileID, &transferID);

    if (!status.ok()) {
        return -1;
    }
    // 初始化数据库
    db.createTransfer(transferID.transferid(), fileInfo);
    // 初始化文件管理器
    std::string filePath = fileDirectoryPath + fileInfo.filename();
    std::wstring wfilePath = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(filePath);
    fileManager.startWriteFile(wfilePath, transferID.transferid());
    // 文件摘要管理
    uint32_t fileBlockNumber = fileInfo.filesize() / fileBlockSize + 1;
    fileManager.createFileDigestList(transferID.transferid(), fileBlockNumber);
    pool.commit(std::bind_front(&FileManager::updateFileDigestThread, &FileManager::getInstance()), transferID.transferid());

    return transferID.transferid();
}

bool StreamServiceClient::transferDownloadFile(uint32_t transferID) {
    FileManager& fileManager = FileManager::getInstance();
    database& db = database::getInstance();
    FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    AppConfig& appConfig = AppConfig::getInstance();
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));

    ClientContext context;
    TransferID id;
    id.set_transferid(transferID);
    std::unique_ptr<ClientReader<FileBlock>> reader(stub_->transferDownloadFile(&context, id));

    FileBlock fileBlock;
    while(reader->Read(& fileBlock)) {
        // 更新文件摘要信息
        std::shared_ptr<unsigned char[]> data(new unsigned char[fileBlock.filesize()]);
        memcpy(data.get(), fileBlock.fileblock().data(), fileBlock.filesize());
        fileManager.updateFileDigestList(transferID, fileBlock.offset() / fileBlockSize, data, fileBlock.filesize());
        // 写入磁盘
        fileManager.writeFileViaID(fileBlock);
    }

    Status status = reader->Finish();
    if (status.ok()) {
        return true;
    } else {
        return false;
    }
}

void StreamServiceClient::endDownloadFile(uint32_t transferID) {
    // 表示当前的文件效验完成
    database& db = database::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();

    FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    FileDigest fileDigest = std::move(fileManager.getFileDigest(transferID));


    fileManager.endWriteFile(transferID);
    fileManager.deleteFileDigestList(transferID);
    db.endTransfer(transferID);

}

void StreamServiceClient::initFileDirectory() {
    if (std::filesystem::exists(fileDirectoryPath)) {
        return ;
    }
    std::filesystem::create_directories(fileDirectoryPath);
}

int StreamServiceClient::transferDownloadFileSHA(uint32_t transferID) {
    // 如果都已经成功传送了，则返回0，否则返回1，有异常返回-1;
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    database& db = database::getInstance();
    FileInfo fileInfo = db.queryTransfer(transferID);
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));

    Transfer::FileDigest fileDigest = std::move(fileManager.getFileDigest(transferID));
    std::cerr << fileDigest.digest() << std::endl;
    fileDigest.set_transferid(transferID);
    ClientContext context;
    std::unique_ptr<ClientReader<FileBlock>> reader(stub_->transferDownloadFileSHA(&context, fileDigest));
    Transfer::FileBlock fileBlock;

    bool end = true;
    while(reader->Read(&fileBlock)) {
        end = false;
        // 更新文件摘要信息
        std::shared_ptr<unsigned char[]> data(new unsigned char[fileBlock.filesize()]);
        memcpy(data.get(), fileBlock.fileblock().data(), fileBlock.filesize());
        fileManager.updateFileDigestList(transferID, fileBlock.offset() / fileBlockSize, data, fileBlock.filesize());
        // 写入磁盘
        fileManager.writeFileViaID(fileBlock);
    }

    Status status = reader->Finish();
    if (status.ok()) {
        if (end) {
            return 0;
        } else {
            return 1;
        }
    } else {
        return -1;
    }
}
