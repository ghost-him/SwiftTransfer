#pragma once

#include "fstreamPool.h"

class ofstreamPool : public fstreamPool {
public:
    explicit ofstreamPool(std::ptrdiff_t count = fstreamPool_Default_Count);
    explicit ofstreamPool(const std::filesystem::path& path, ptrdiff_t count = fstreamPool_Default_Count);

    ~ofstreamPool();

    // 在指定的位置写指定的内容
    void write(std::ptrdiff_t offset, std::ptrdiff_t length, void* source);
private:
    // 判断当前是否可以执行操作
    bool isValid();

    // 打开文件流
    bool openPath(const std::filesystem::path& path) override;
};
