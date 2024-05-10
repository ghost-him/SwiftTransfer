//
// Created by ghost-him on 24-4-29.
//

#include "server.h"

#include "../AppConfig/AppConfig.h"
#include "../IDGenerator/IDGenerator.h"
#include "../database/database.h"
#include "../ClientManager/ClientManager.h"
#include "../FileManager/FileManager.h"
#include "../ThreadPool/threadpool.h"

Status
StreamServiceImpl::getConfiguration(ServerContext *context, const Placeholder *placeholder, Configuration *writer) {
    AppConfig& config = AppConfig::getInstance();
    IDGenerator& generator = IDGenerator::getInstance();

    uint32_t fileBlockSize = std::stoi(config.get("fileBlockSize"));
    uint32_t clientID = generator.getUniqueID();

    writer->set_clientid(clientID);
    writer->set_fileblocksize(fileBlockSize);

    ClientManager::getInstance().addClient(clientID);
    return Status::OK ;
}

Status StreamServiceImpl::getFileList(ServerContext *context, const PageIndex *pageIndex, FileList *writer) {
    uint32_t page = pageIndex->page();
    database& db = database::getInstance();

    Transfer::FileList fileList = std::move(db.queryLatestFile(page));
    writer->CopyFrom(fileList);
    return Status::OK;
}

Status StreamServiceImpl::getUniqueID(ServerContext *context, const Placeholder *placeholder, UniqueID *writer) {
    IDGenerator& generator = IDGenerator::getInstance();
    writer->set_uniqueid(generator.getUniqueID());
    return Status::OK;
}

Status StreamServiceImpl::HeartbeatSignal(ServerContext *context, const Placeholder *placeholder, Placeholder *writer) {
    std::string clientIDStr = context->client_metadata().find("clientID")->second.data();
    uint32_t clientID = std::stoi(clientIDStr);
    ClientManager::getInstance().processHeartbeat(clientID);
    return Status::OK;
}

Status StreamServiceImpl::sendUploadFileInfo(ServerContext *context, const StartTransfer *startTransfer,
                                             Placeholder *placeholder) {
    database& db = database::getInstance();
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    // 数据库初始化
    db.createTransfer(startTransfer->transferid(), startTransfer->fileinfo());
    // 文件管理器初始化
    std::string filePath = fileDirectoryPath + std::to_string(startTransfer->fileinfo().fileid());
    fileManager.startWriteFile(filePath, startTransfer->transferid());
    uint32_t blockNumber = startTransfer->fileinfo().filesize() / std::stoi(appConfig.get("fileBlockSize")) + 1;
    // 文件摘要管理初始化
    fileManager.createFileDigestList(startTransfer->fileinfo().fileid(), blockNumber);
    ThreadPool::getInstance().commit(std::bind_front(&FileManager::updateFileDigestThread, &FileManager::getInstance()), startTransfer->fileinfo().fileid());
    return Status::OK;
}

Status StreamServiceImpl::sendUploadFileBlock(ServerContext *context, ServerReader<FileBlock> *reader,
                                              Placeholder *placeholder) {
    database& db = database::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();
    Transfer::FileInfo fileInfo;
    bool read = false;
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));
    Transfer::FileBlock fileBlock;
    while(reader->Read(&fileBlock)) {
        if (!read) {
            // 如果是第一次读，则读取一下目标文件的信息
            fileInfo = std::move(db.queryTransfer(fileBlock.transferid()));
            read = true;
        }
        // 更新文件摘要信息
        std::shared_ptr<unsigned char[]> data(new unsigned char[fileBlock.filesize()]);
        memcpy(data.get(), fileBlock.fileblock().data(), fileBlock.filesize());
        fileManager.updateFileDigestList(fileInfo.fileid(), fileBlock.offset() / fileBlockSize, data, fileBlock.filesize());
        // 写入磁盘
        fileManager.writeFileViaID(fileBlock);
    }

    return Status::OK;
}

Status StreamServiceImpl::sendFileVerification(ServerContext *context, const FileDigest *fileDigest,
                                               RequestFileBlockList *requestFileBlockList) {
    database& db = database::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();

    uint32_t transferID = fileDigest->transferid();
    FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    std::vector<uint32_t> differentBlock = fileManager.isFileComplete(fileInfo.fileid(), *fileDigest);
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));
    requestFileBlockList->set_transferid(transferID);
    if (differentBlock.empty()) {
        requestFileBlockList->add_offset(-1);
    } else {
        for (auto i : differentBlock) {
            uint64_t offset = (uint64_t)i * fileBlockSize;
            requestFileBlockList->add_offset(offset);
        }
    }

    return Status::OK;
}

Status
StreamServiceImpl::endSendUploadFile(ServerContext *context, const TransferID *transferId, Placeholder *placeholder) {
    // 表示当前的文件效验完成
    database& db = database::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();

    uint32_t transferID = transferId->transferid();
    FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    FileDigest fileDigest = std::move(fileManager.getFileDigest(fileInfo.fileid()));


    fileManager.endWriteFile(transferID);
    fileManager.deleteFileDigestList(fileInfo.fileid());
    db.endTransfer(transferID);

    db.createNewFile(fileInfo.fileid(), fileInfo.filesize(), fileInfo.filename(), fileDigest.digest());


    return Status::OK;
}

void StreamServiceImpl::initFileDirectory() {
    if (std::filesystem::exists(fileDirectoryPath)) {
        return ;
    }
    std::filesystem::create_directories(fileDirectoryPath);
}

