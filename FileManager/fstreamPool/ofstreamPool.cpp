//
// Created by ghost-him on 24-5-5.
//

#include "ofstreamPool.h"
#include <iostream>

ofstreamPool::ofstreamPool(std::ptrdiff_t count) : fstreamPool(count) {

}

ofstreamPool::~ofstreamPool() {
    close();
}


bool ofstreamPool::openPath(const std::filesystem::path &path) {
    // 测试是否可以正常打开
    std::ifstream tempIfs(path);
    if (!tempIfs.is_open()) {
        return false;
    }
    tempIfs.close();

    for (auto& i : _freeNodes) {
        i->fst->open(path, std::ios::out | std::ios::binary);
    }

    return true;
}

bool ofstreamPool::isValid() {
    // 当文件池没有被关闭时，则可以访问
    return is_open();
}

void ofstreamPool::write(std::ptrdiff_t offset, std::ptrdiff_t length, void *source) {
    if (!isValid())
        return ;

    uint32_t fstreamID = getAvailableFstream();
    std::shared_ptr<std::fstream> ptr = getFstream(fstreamID);
    std::cout << "offset:" << offset << "length:" << length << std::endl;
    ptr->seekp(offset, std::ios::beg);
    ptr->write(reinterpret_cast<char*>(source), length);
    ptr->flush();
    endUseFile(fstreamID);
}





