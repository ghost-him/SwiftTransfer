#pragma once

#include <filesystem>
#include <atomic>
#include <semaphore>
#include <cstddef>
#include <set>
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <array>

#define fstreamPool_Default_Count 5

class fstreamPool {

protected:

struct fstreamNode {
    uint32_t fstreamID;
    std::shared_ptr<std::fstream> fst;
    std::shared_ptr<std::mutex> lock;
    const bool operator<(const fstreamNode& other) const {
        return fstreamID < other.fstreamID;
    }
};

public:
    fstreamPool(std::ptrdiff_t count = fstreamPool_Default_Count);
    fstreamPool(const std::filesystem::path& path, ptrdiff_t count = fstreamPool_Default_Count);

    virtual ~fstreamPool();

    bool open(const std::filesystem::path& path);
    bool is_open();
    void close();

protected:
    // 初始化信号量
    void initSemaphore(std::ptrdiff_t count);
    // 打开文件流
    virtual bool openPath(const std::filesystem::path& path) = 0;

    // 返回一个可以使用的文件流ID
    uint32_t getAvailableFstream();
    // 结束使用目标文件流
    void endUseFile(uint32_t fstreamID);
    // 根据文件流ID获得文件流
    std::shared_ptr<std::fstream> getFstream(uint32_t fstreamID);


    std::atomic<bool> _stop = true;
    std::unique_ptr<std::counting_semaphore<fstreamPool_Default_Count>> _sem;
    // 操作以下的数据结构时，需要将其锁起来
    std::mutex _poolGuard;
    std::set<std::shared_ptr<fstreamNode>> _usedNodes;
    std::set<std::shared_ptr<fstreamNode>> _freeNodes;
    std::unordered_map<uint32_t, std::set<std::shared_ptr<fstreamNode>>::iterator> _fstreamIDMap;

    // 文件路径
    std::filesystem::path _path;
};