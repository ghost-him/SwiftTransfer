//
// Created by ghost-him on 24-4-28.
//

#ifndef SWIFTTRANSFER_IDGENERATOR_H
#define SWIFTTRANSFER_IDGENERATOR_H

#include <atomic>
#include <stdint.h>
#include <chrono>

class IDGenerator {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    IDGenerator(IDGenerator const&) = delete;
    IDGenerator& operator=(IDGenerator const&) = delete;
    // 提供一个全局唯一的接口
    static IDGenerator& getInstance() {
        static IDGenerator instance;
        return instance;
    }

    uint32_t getUniqueID();

private:
    IDGenerator();
    std::atomic<uint32_t> _id;
};


#endif //SWIFTTRANSFER_IDGENERATOR_H
