#pragma once

#include <string>
#include <memory>

#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>
#include "../grpc/entity.pb.h"

#define DB_POOL_SIZE 5

class database {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    database(database const&) = delete;
    database& operator=(database const&) = delete;
    // 提供一个全局唯一的接口
    static database& getInstance() {
        static database instance;
        return instance;
    }


    void init();

    const std::string dbPath = "./data.db";

    // 新建一个文件
    void createNewFile(uint32_t fileID, uint64_t fileSize, const std::string& fileName, const std::string& digest);
    // 查询一个文件
    Transfer::FileInfo queryFile(uint32_t fileID);
    // 查询最新添加的文件
    Transfer::FileList queryLatestFile(uint32_t page);


    // 维护一个新的传输
    void createTransfer(uint32_t transferID, const Transfer::FileInfo& fileInfo);
    // 结束一个传输
    void endTransfer(uint32_t transferID);
    // 查询一个传输的文件信息
    Transfer::FileInfo queryTransfer(uint32_t transferID);


private:
    database() = default;

    void createTable();

    std::shared_ptr<soci::connection_pool> dbPool;

};

