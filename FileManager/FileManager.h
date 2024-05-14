//
// Created by ghost-him on 24-4-29.
//

#ifndef SWIFTTRANSFER_FILEMANAGER_H
#define SWIFTTRANSFER_FILEMANAGER_H

#include <filesystem>
#include <memory>
#include <unordered_map>
#include "fstreamPool/ifstreamPool.h"
#include "fstreamPool/ofstreamPool.h"
#include "../grpc/entity.pb.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <set>

class FileManager {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    FileManager(FileManager const&) = delete;
    FileManager& operator=(FileManager const&) = delete;
    // 提供一个全局唯一的接口
    static FileManager& getInstance() {
        static FileManager instance;
        return instance;
    }

    ~FileManager();

    // 开启一个本地的读取文件操作
    void startReadFile(const std::filesystem::path& path, uint32_t transferID);
    // 结束一个本地的读取文件操作
    void endReadFile(uint32_t transferID);
    // 对一个已经打开的文件，读取指定位置的指定字节
    std::pair<Transfer::FileBlock, std::shared_ptr<unsigned char[]>> readFileViaID(uint32_t transferID, uint64_t offset, uint32_t length);
    // 获取目标文件的大小
    uint64_t getFileSize(uint32_t transferID);
    // 获取文件的信息


    // 开启一个本地的写文件操作
    void startWriteFile(const std::filesystem::path & path, uint32_t transferID);
    // 结束一个本地的写文件操作
    void endWriteFile(uint32_t transferID);
    // 对一个已经打开的文件，在指定位置写入指定的字节
    void writeFileViaID(const Transfer::FileBlock& fileBlock);

    // 在写入文件的过程中记录文件的摘要

    // 根据文件id新建一个摘要列表
    void createFileDigestList(uint32_t transferID, uint32_t blockNumber);
    // 更新文件完整性列表
    void updateFileDigestList(uint32_t transferID, uint32_t blockIndex, std::shared_ptr<unsigned char[]> data, uint64_t dataSize);
    // 判断文件是否完整
    std::vector<uint32_t> isFileComplete(uint32_t transferID, const Transfer::FileDigest& fileDigest);
    // 删除文件
    void deleteFileDigestList(uint32_t transferID);
    // 获取文件的摘要
    Transfer::FileDigest getFileDigest(uint32_t transferID);



    /*
     * 后台处理函数
     */


    // 执行写文件函数
    void writeThread();
    // 更新维护的文件列表
    void updateFileList(const Transfer::FileList& fileList);
    // 更新文件的摘要
    void updateFileDigestThread(uint32_t transferID);
    // 判断路径的指向是否是一个普通文件
    bool isValid(const std::filesystem::path& path);
    // 结束模块
    void close();
    // fileIndex = [1, 9]
    Transfer::FileInfo getServerFileInfo(uint32_t fileIndex);

private:
    FileManager();
    std::unordered_map<uint32_t, std::shared_ptr<ifstreamPool>> ifstreamMap;
    std::unordered_map<uint32_t, std::shared_ptr<ofstreamPool>> ofstreamMap;

    std::mutex writeQLock;
    std::condition_variable clock;
    std::queue<Transfer::FileBlock> writeQueue;
    std::atomic<bool> stop;

    struct FileDigestStruct{
        std::mutex lock;
        std::shared_ptr<std::condition_variable> cv;
        std::atomic<uint64_t> targetBlockIndex;
        uint64_t totalBlockNumber;
        std::set<std::tuple<uint32_t, std::shared_ptr<unsigned char[]>, uint64_t>> blockStore;
        std::vector<std::string> partSha256;
        std::mutex computeFileLock; // 当整个文件的sha256都计算结束时，才unlock
    };

    // 文件摘要处理
    std::unordered_map<uint32_t, std::shared_ptr<FileDigestStruct>> fileDigestMap;
    // 存储服务器的文件列表
    Transfer::FileList fileList;
};


#endif //SWIFTTRANSFER_FILEMANAGER_H
