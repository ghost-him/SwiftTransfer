#include "ifstreamPool.h"

ifstreamPool::ifstreamPool(std::ptrdiff_t count) : fstreamPool(count) {
}

ifstreamPool::~ifstreamPool() {
    close();
}

std::pair<std::shared_ptr<unsigned char[]>, std::streamsize> ifstreamPool::read(std::ptrdiff_t offset, std::ptrdiff_t length) {
    if (!isValid())
        return {};
    std::shared_ptr<unsigned char[]> buffer(new unsigned char[length]);

    uint32_t fstreamID = getAvailableFstream();
    std::shared_ptr<std::fstream> ptr = getFstream(fstreamID);
    ptr->seekg(offset, std::ios::beg);
    std::ptrdiff_t getCount = ptr->readsome(reinterpret_cast<char *>(buffer.get()), length);
    endUseFile(fstreamID);
    return std::make_pair(buffer, getCount);
}

void ifstreamPool::read(std::ptrdiff_t offset, std::ptrdiff_t& length, void* target) {
    if (!isValid())
        return;
    uint32_t fstreamID = getAvailableFstream();
    std::shared_ptr<std::fstream> ptr = getFstream(fstreamID);
    ptr->seekg(offset);
    std::ptrdiff_t getCount = ptr->readsome(reinterpret_cast<char *>(target), length);
    endUseFile(fstreamID);
    length = getCount;
}


bool ifstreamPool::openPath(const std::filesystem::path& path) {
    // 测试是否可以正常打开
    std::ifstream tempIfs(path);
    if (!tempIfs.is_open()) {
        return false;
    }
    tempIfs.close();

    for (auto& i : _freeNodes) {
        i->fst->open(path, std::ios::in | std::ios::binary);
    }
    _stop.store(false);

    return true;
}

bool ifstreamPool::isValid() {
    // 当文件池没有被关闭时，则可以访问
    return is_open();
}

uint64_t ifstreamPool::getFileSize() {
    return std::filesystem::file_size(_path);
}
