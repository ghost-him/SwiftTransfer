//
// Created by ghost-him on 24-4-29.
//

#include "server.h"

#include "../AppConfig/AppConfig.h"
#include "../IDGenerator/IDGenerator.h"
#include "../database/database.h"
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
    fileManager.createFileDigestList(startTransfer->transferid(), blockNumber);
    ThreadPool::getInstance().commit(std::bind_front(&FileManager::updateFileDigestThread, &FileManager::getInstance()), startTransfer->transferid());
    // 初始化客户端管理器


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
        fileManager.updateFileDigestList(fileBlock.transferid(), fileBlock.offset() / fileBlockSize, data, fileBlock.filesize());
        // 写入磁盘
        fileManager.writeFileViaID(fileBlock);
    }

    return Status::OK;
}

Status StreamServiceImpl::transferUploadFileSHA(ServerContext *context, const FileDigest *fileDigest,
                                               RequestFileBlockList *requestFileBlockList) {
    database& db = database::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();

    uint32_t transferID = fileDigest->transferid();
    FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    std::vector<uint32_t> differentBlock = fileManager.isFileComplete(transferID, *fileDigest);
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
    FileDigest fileDigest = std::move(fileManager.getFileDigest(transferID));


    fileManager.endWriteFile(transferID);
    fileManager.deleteFileDigestList(transferID);
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

Status StreamServiceImpl::getDownloadTransferID(ServerContext *context, const FileID *fileID, TransferID *writer) {
    database& db = database::getInstance();
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileInfo fileInfo = std::move(db.queryFile(fileID->fileid()));
    // 初始化文件传输
    uint32_t transferID = IDGenerator::getInstance().getUniqueID();
    writer->set_transferid(transferID);
    // 初始化数据库
    db.createTransfer(transferID, fileInfo);

    // 文件管理器初始化
    std::string filePath = fileDirectoryPath + std::to_string(fileID->fileid());
    fileManager.startReadFile(filePath, transferID);
    uint32_t totalBlockNumber = fileInfo.filesize() / std::stoi(appConfig.get("fileBlockSize")) + 1;
    // 文件摘要管理初始化
    fileManager.createFileDigestList(transferID, totalBlockNumber);
    ThreadPool::getInstance().commit(std::bind_front(&FileManager::updateFileDigestThread, &FileManager::getInstance()), transferID);
    // 初始化文件发送器
    std::shared_ptr<SendFileList> sendFileList = std::make_shared<SendFileList>();
    sendFileListMap[transferID] = sendFileList;

    for (int i = 0; i < totalBlockNumber; i ++) {
        sendFileList->FileBlockToSend.push(i);
    }

    return Status::OK;

}

Status StreamServiceImpl::transferDownloadFile(ServerContext *context, const TransferID *transferID,
                                               ServerWriter<FileBlock> *writer) {
    FileManager& fileManager = FileManager::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    database& db = database::getInstance();

    FileInfo fileInfo = std::move(db.queryTransfer(transferID->transferid()));
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));
    std::shared_ptr<std::thread> threads[Default_Send_Thread];
    std::mutex serverWriterLock;
    for (int i = 0; i < Default_Send_Thread; i ++) {
        std::shared_ptr<std::thread> ptr = std::make_shared<std::thread>([&](){
            while(true) {
                uint32_t fileBlockIndex = getNextBlockIndex(sendFileListMap[transferID->transferid()]);
                if (fileBlockIndex == -1) {
                    // 传输结束
                    break;
                }
                uint64_t offset = fileBlockIndex * fileBlockSize;
                auto ret = fileManager.readFileViaID(transferID->transferid(), offset, fileBlockSize);
                Transfer::FileBlock fileBlock = ret.first;
                // 计算一下哈希值
                fileManager.updateFileDigestList(transferID->transferid(), fileBlockIndex, ret.second, fileBlock.filesize());
                std::unique_lock<std::mutex> guard(serverWriterLock);
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
    return Status::OK;
}

Status StreamServiceImpl::transferDownloadFileSHA(ServerContext *context, const FileDigest *fileDigest,
                                                  ServerWriter<FileBlock> *writer) {
    database& db = database::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();

    uint32_t transferID = fileDigest->transferid();
    FileInfo fileInfo = std::move(db.queryTransfer(transferID));
    std::vector<uint32_t> differentBlock = fileManager.isFileComplete(transferID, *fileDigest);
    uint32_t fileBlockSize = std::stoi(appConfig.get("fileBlockSize"));


    if (!differentBlock.empty()) {
        for (auto i : differentBlock) {
            uint64_t offset = (uint64_t)i * fileBlockSize;
            auto ret = fileManager.readFileViaID(fileDigest->transferid(), offset, fileBlockSize);
            Transfer::FileBlock fileBlock = ret.first;
            if (!writer->Write(fileBlock)) {
                // 传输失败
                break;
            }
        }
    }

    return Status::OK;
}

Status StreamServiceImpl::endDownloadFile(ServerContext *context, const TransferID *transferID, Placeholder *writer) {
    // 表示当前的文件效验完成
    database& db = database::getInstance();
    AppConfig& appConfig = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();

    FileInfo fileInfo = std::move(db.queryTransfer(transferID->transferid()));
    FileDigest fileDigest = std::move(fileManager.getFileDigest(transferID->transferid()));

    fileManager.endWriteFile(transferID->transferid());
    fileManager.deleteFileDigestList(transferID->transferid());
    db.endTransfer(transferID->transferid());
    return Status::OK;
}

uint32_t StreamServiceImpl::getNextBlockIndex(std::shared_ptr<SendFileList> ptr) {
    std::unique_lock<std::mutex> guard(ptr->lock);
    // 如果所有的都已经发送结束了，则返回
    if (ptr->FileBlockToSend.empty())
        return -1;
    uint32_t blockIndex = ptr->FileBlockToSend.front();
    ptr->FileBlockToSend.pop();
    return blockIndex;
}

