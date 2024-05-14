//
// Created by ghost-him on 24-4-29.
//

#include "database.h"

#include <filesystem>
#include <stdexcept>
#include <fstream>
#include "sqlite3.h"

void database::init() {
    // 创建数据库文件
    std::filesystem::path path(dbPath);
    if (!std::filesystem::exists(path)) {
        std::ofstream ofs(path, std::ios::out);
        if (!ofs.is_open()) {
            throw std::runtime_error("create database failed");
        }
        ofs.close();
    }
    dbPool = std::make_shared<soci::connection_pool>(DB_POOL_SIZE);
    // 连接数据库
    // 初始化连接池
    for (std::ptrdiff_t i = 0; i < DB_POOL_SIZE; i++) {
        soci::session &db = dbPool->at(i);
        db.open(soci::sqlite3, dbPath);
    }

    // 初始化连接
    if (std::filesystem::is_empty(dbPath)) {
        // 如果是空的，则要初始化数据库
        createTable();
    }

}

void database::createTable() {
    std::string sqls[2];
    sqls[0] = R"(CREATE TABLE FileInfo (
                    fileID INTEGER PRIMARY KEY,
                    fileSize BIGINT,
                    fileName TEXT,
                    digest TEXT
                );)";

    sqls[1] = R"(CREATE TABLE TransferInfo (
                    transferID INTEGER PRIMARY KEY,
                    fileID INTEGER,
                    fileSize BIGINT,
                    fileName TEXT,
                    digest TEXT
                );)";
    soci::session db(*dbPool);
    for (int i = 0 ; i < 2; i ++) {
        try {
            db << sqls[i];
        } catch (const soci::soci_error& e) {
            std::cerr << "SOCI error: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

void
database::createNewFile(uint32_t fileID, uint64_t fileSize, const std::string &fileName, const std::string &digest) {
    soci::session db(*dbPool);
    try {
        db << "INSERT INTO FileInfo (fileID, fileSize, fileName, digest) VALUES (:fileID, :fileSize, :fileName, :digest);",
        soci::use(fileID),
        soci::use(fileSize),
        soci::use(fileName),
        soci::use(digest);
    } catch (const soci::soci_error& e) {
        std::cerr << "SOCI error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

Transfer::FileInfo database::queryFile(uint32_t fileID) {
    soci::session db(*dbPool);
    Transfer::FileInfo result;
    std::string sql = "SELECT * FROM FileInfo WHERE fileID = :fileID;";
    soci::row ans;
    try {
        db << sql.c_str(), soci::into(ans), soci::use(fileID);
        // 检查是否有结果
        if (!db.got_data()) {
            std::cerr << "No data found." << std::endl;
            return result;
        }

        result.set_fileid(ans.get<int32_t>("fileID"));
        result.set_filesize(ans.get<sqlite_api::sqlite3_int64>("fileSize"));
        result.set_filename(ans.get<std::string>("fileName"));
        result.set_digest(ans.get<std::string>("digest"));
    } catch (const soci::soci_error& e) {
        std::cerr << "SOCI error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return std::move(result);
}

void database::createTransfer(uint32_t transferID, const Transfer::FileInfo &fileInfo) {
    uint32_t fileID = fileInfo.fileid();
    uint64_t fileSize = fileInfo.filesize();
    std::string fileName = fileInfo.filename();
    std::string digest = fileInfo.digest();
    soci::session db(*dbPool);
    try {
        db << "INSERT INTO TransferInfo (transferID, fileID, fileSize, fileName, digest) VALUES (:transferID, :fileID, :fileSize, :fileName, :digest);",
                soci::use(transferID),
                soci::use(fileID),
                soci::use(fileSize),
                soci::use(fileName),
                soci::use(digest);
    } catch (const soci::soci_error& e) {
        std::cerr << "SOCI error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void database::endTransfer(uint32_t transferID) {
    soci::session db(*dbPool);
    try {
        db << "DELETE FROM TransferInfo WHERE transferID = :transferID;",
                soci::use(transferID);
    } catch (const soci::soci_error& e) {
        std::cerr << "SOCI error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

Transfer::FileInfo database::queryTransfer(uint32_t transferID) {
    soci::session db(*dbPool);
    Transfer::FileInfo result;
    std::string sql = "SELECT * FROM TransferInfo WHERE transferID = :transferID;";
    soci::row ans;
    try {
        db << sql.c_str(), soci::into(ans), soci::use(transferID);
        // 检查是否有结果
        if (!db.got_data()) {
            std::cerr << "No data found." << std::endl;
            return result;
        }

        result.set_fileid(ans.get<int32_t>("fileID"));
        result.set_filesize(ans.get<sqlite_api::sqlite3_int64>("fileSize"));
        result.set_filename(ans.get<std::string>("fileName"));
        result.set_digest( ans.get<std::string>("digest"));

    } catch (const soci::soci_error& e) {
        std::cerr << "SOCI error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return std::move(result);
}

Transfer::FileList database::queryLatestFile(uint32_t page) {
    soci::session db(*dbPool);
    Transfer::FileList result;
    std::string sql = "SELECT fileID, fileSize, fileName, digest FROM FileInfo ORDER BY fileID DESC LIMIT 9 OFFSET (:page - 1) * 9;";
    // 计算查询范围
    try {
        // 查询数据库,获取总记录数
        int count;
        db << "SELECT COUNT(*) FROM FileInfo", soci::into(count);
        int totalPage = count / 9 + 1;
        // 获得指定的个数
        soci::rowset<soci::row> rs = (db.prepare << sql, soci::use(page));

        result.set_page(page);
        result.set_totalpage(totalPage);
        for (int i = 0; i < 9; i ++) {
            result.add_fileinfos();
        }

        int index = 0;
        for (auto it = rs.begin(); it != rs.end(); it ++) {
            const soci::row& row = *it;
            Transfer::FileInfo* info = result.mutable_fileinfos(index ++);

            info->set_fileid(row.get<int32_t>("fileID"));
            info->set_filesize(row.get<sqlite_api::sqlite3_int64>("fileSize"));
            info->set_filename(row.get<std::string>("fileName"));
            info->set_digest(row.get<std::string>("digest"));
        }
    } catch (const soci::soci_error& e) {
        std::cerr << "SOCI error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return std::move(result);
}
