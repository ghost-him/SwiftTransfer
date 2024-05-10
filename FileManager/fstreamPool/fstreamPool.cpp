#include "fstreamPool.h"

fstreamPool::fstreamPool(std::ptrdiff_t count) {
    initSemaphore(count);
}

fstreamPool::fstreamPool(const std::filesystem::path& path, ptrdiff_t count) : _path(path) {
    initSemaphore(count);
}

fstreamPool::~fstreamPool() {
    close();
}

bool fstreamPool::open(const std::filesystem::path& path) {
    _path = path;
    bool ret = openPath(path);
    if (ret) {
        _stop.store(false);
    }
    return ret;
}

bool fstreamPool::is_open() {
    return !_stop;
}

void fstreamPool::close() {
    _stop.store(true);
    // 等待所有的都结束
    for (const std::shared_ptr<fstreamNode>& i : _freeNodes) {
        std::unique_lock<std::mutex> wait_for_end(*(i->lock));
    }
    // 关闭所有的内容
    for (const std::shared_ptr<fstreamNode>& i : _freeNodes) {
        i->fst->close();
    }
}

void fstreamPool::initSemaphore(std::ptrdiff_t count) {
    _sem = std::make_unique<std::counting_semaphore<fstreamPool_Default_Count>>(fstreamPool_Default_Count);
    for (int i = 1; i <= count; i ++) {
        std::shared_ptr<fstreamNode> ptr = std::make_shared<fstreamNode>();
        ptr->fstreamID = i;
        ptr->fst = std::make_shared<std::fstream>();
        ptr->lock = std::make_shared<std::mutex>();
        auto ret = _freeNodes.insert(ptr);
        _fstreamIDMap[i] = ret.first;
    }
}

uint32_t fstreamPool::getAvailableFstream() {
    _sem->acquire();
    std::unique_lock<std::mutex> lock(_poolGuard);

    std::shared_ptr<fstreamNode> target = *_freeNodes.begin();
    _freeNodes.erase(target);

    target->lock->lock();
    uint32_t fstreamID = target->fstreamID;
    _usedNodes.insert(target);

    _fstreamIDMap[fstreamID] = _usedNodes.find(target);
    return fstreamID;
}

void fstreamPool::endUseFile(uint32_t fstreamID) {
    std::unique_lock<std::mutex> lock(_poolGuard);

    std::shared_ptr<fstreamNode> target = *_fstreamIDMap[fstreamID];
    _usedNodes.erase(target);

    target->lock->unlock();
    _freeNodes.insert(target);

    _fstreamIDMap[fstreamID] = _freeNodes.find(target);

    lock.unlock();
    _sem->release();
}

std::shared_ptr<std::fstream> fstreamPool::getFstream(uint32_t fstreamID) {
    std::unique_lock<std::mutex> lock(_poolGuard);
    return (*_fstreamIDMap[fstreamID])->fst;
}