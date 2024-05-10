#pragma once

#include "fstreamPool.h"
#include <filesystem>
#include <atomic>
#include <semaphore>
#include <cstddef>

class ifstreamPool : public fstreamPool{
public:
    explicit ifstreamPool(std::ptrdiff_t count = fstreamPool_Default_Count);
    explicit ifstreamPool(const std::filesystem::path& path, ptrdiff_t count = fstreamPool_Default_Count);

    ~ifstreamPool();

    // 读取指定位置的指定长度的数据
    std::pair<std::shared_ptr<unsigned char[]>, std::streamsize>  read(std::ptrdiff_t offset, std::ptrdiff_t length);
    // 读取指定位置的指定长度的数据并存放到指定的数组中（要用户保证数组长度有效）
    void read(int64_t offset, std::ptrdiff_t& length, void* target);
    // 获取目标文件的大小
    uint64_t getFileSize();

private:
    // 判断当前是否可以执行操作
    bool isValid();
    // 打开文件流
    bool openPath(const std::filesystem::path& path) override;
};