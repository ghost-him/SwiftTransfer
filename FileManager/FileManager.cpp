//
// Created by ghost-him on 24-4-29.
//

#include "FileManager.h"
#include "../CipherUtils/CipherUtils.h"

FileManager::FileManager() : stop(false) {

}

FileManager::~FileManager() {
    close();
}

void FileManager::startReadFile(const std::filesystem::path& path, uint32_t transferID) {
    if (!isValid(path)) {
        return ;
    }
    std::shared_ptr<ifstreamPool> ptr = std::make_shared<ifstreamPool>();
    ptr->open(path);
    if (!ptr->is_open()) {
        throw std::runtime_error("open path failed:" + path.string());
    }
    ifstreamMap[transferID] = ptr;
}

void FileManager::endReadFile(uint32_t transferID) {
    if (ifstreamMap.contains(transferID)) {
        ifstreamMap[transferID]->close();
        ifstreamMap.erase(transferID);
    } else {
        throw std::runtime_error("transferID is not exists: " + std::to_string(transferID));
    }
    
}

std::pair<Transfer::FileBlock, std::shared_ptr<unsigned char[]>> FileManager::readFileViaID(uint32_t transferID, uint64_t offset, uint32_t length) {
    std::shared_ptr<ifstreamPool> pool = ifstreamMap[transferID];
    std::pair<std::shared_ptr<unsigned char[]>, std::streamsize> ret = pool->read(offset, length);

    Transfer::FileBlock result;
    result.set_filesize(ret.second);
    result.set_offset(offset);
    result.set_transferid(transferID);
    result.set_fileblock(reinterpret_cast<char*>(ret.first.get()), ret.second);
    return {std::move(result), ret.first};
}

void FileManager::startWriteFile(const std::filesystem::path &path, uint32_t transferID) {

    if (!std::filesystem::exists(path)) {
        std::ofstream ofs(path);
    }

    std::shared_ptr<ofstreamPool> ptr = std::make_shared<ofstreamPool>();
    ptr->open(path);

    if (!ptr->is_open()) {
        throw std::runtime_error("open path failed:" + path.string());
    }
    ofstreamMap[transferID] = ptr;
}

void FileManager::endWriteFile(uint32_t transferID) {
    if (ofstreamMap.contains(transferID)) {
        ofstreamMap[transferID]->close();
        ofstreamMap.erase(transferID);
    } else {
        throw std::runtime_error("transferID is not exists: " + std::to_string(transferID));
    }
}

void FileManager::writeFileViaID(const Transfer::FileBlock& fileBlock) {
    {
        std::unique_lock<std::mutex> guard(writeQLock);
        writeQueue.push(std::move(fileBlock));
    }
    clock.notify_one();
}

void FileManager::writeThread() {
    while (true) {
        // get data
        std::unique_lock<std::mutex> lock(writeQLock);
        clock.wait(lock, [this]() { return !writeQueue.empty() || stop; });

        if (stop && writeQueue.empty()) {
            break;
        }

        Transfer::FileBlock fileBlock = std::move(writeQueue.front());
        writeQueue.pop();
        lock.unlock();
        std::shared_ptr<ofstreamPool> ptr = ofstreamMap[fileBlock.transferid()];
        ptr->write(fileBlock.offset(), fileBlock.filesize(), (void*)fileBlock.fileblock().data());
    }
}

void FileManager::updateFileList(const Transfer::FileList &fileList) {
    this->fileList.CopyFrom(fileList);
}

uint64_t FileManager::getFileSize(uint32_t transferID) {
    std::shared_ptr<ifstreamPool> ptr = ifstreamMap[transferID];
    return ptr->getFileSize();
}

void FileManager::createFileDigestList(uint32_t transferID, uint32_t blockNumber) {
    std::shared_ptr<FileDigestStruct> ptr = std::make_shared<FileDigestStruct>();
    ptr->partSha256.resize(blockNumber);
    ptr->targetBlockIndex = 0;
    ptr->totalBlockNumber = blockNumber;
    ptr->cv = std::make_shared<std::condition_variable>();
    ptr->computeFileLock.lock();

    fileDigestMap[transferID] = ptr;
    CipherUtils& cipherUtils = CipherUtils::getInstance();
    cipherUtils.createFileSha256(transferID);

}

void FileManager::updateFileDigestList(uint32_t transferID, uint32_t blockIndex, std::shared_ptr<unsigned char[]> data, uint64_t dataSize) {
    std::shared_ptr<FileDigestStruct> ptr = fileDigestMap[transferID];
    CipherUtils& cipherUtils = CipherUtils::getInstance();
    // 更新部分的sha值
    std::string sha256 = cipherUtils.getStringSha256((void *)data.get(), dataSize);
    ptr->partSha256[blockIndex] = sha256;
    // 更新总体的sha值
    std::unique_lock<std::mutex> guard(ptr->lock);
    ptr->blockStore.insert({blockIndex, data, dataSize});
    guard.unlock();
    ptr->cv->notify_one();
}

std::vector<uint32_t> FileManager::isFileComplete(uint32_t transferID, const Transfer::FileDigest &fileDigest) {
    std::shared_ptr<FileDigestStruct> ptr = fileDigestMap[transferID];
    std::unique_lock<std::mutex> wait_for_compute(ptr->computeFileLock);
    CipherUtils& cipherUtils = CipherUtils::getInstance();
    std::vector<uint32_t> result;
    std::string fileSha256 = cipherUtils.getFileSha256(transferID);
    if (fileSha256 == fileDigest.digest()) {
        return result;
    }

    for (int i = 0; i < ptr->totalBlockNumber; i ++) {
        if (ptr->partSha256[i] != fileDigest.partdigest(i)) {
            result.push_back(i);
        }
    }

    return result;
}

void FileManager::deleteFileDigestList(uint32_t transferID) {
    CipherUtils& cipherUtils = CipherUtils::getInstance();
    std::shared_ptr<FileDigestStruct> ptr = fileDigestMap[transferID];
    std::unique_lock<std::mutex> guard(ptr->lock);
    cipherUtils.deleteFileSha256(transferID);
    fileDigestMap.erase(transferID);
}


bool FileManager::isValid(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }
    if (!std::filesystem::is_regular_file(path)) {
        return false;
    }
    return true;
}

Transfer::FileDigest FileManager::getFileDigest(uint32_t transferID) {
    CipherUtils& cipherUtils = CipherUtils::getInstance();

    std::shared_ptr<FileDigestStruct> ptr = fileDigestMap[transferID];

    Transfer::FileDigest fileDigest;
    fileDigest.set_fileid(transferID);
    std::unique_lock<std::mutex> wait_for_compute(ptr->computeFileLock);
    fileDigest.set_digest(cipherUtils.getFileSha256(transferID));

    std::unique_lock<std::mutex> guard(ptr->lock);
    for (auto& i : ptr->partSha256) {
        fileDigest.add_partdigest(i);
    }
    return std::move(fileDigest);
}

void FileManager::updateFileDigestThread(uint32_t transferID) {
    std::shared_ptr<FileDigestStruct> ptr = fileDigestMap[transferID];

    while(!stop && ptr->targetBlockIndex != ptr->totalBlockNumber) {
        // blockIdx, data, dataSize
        std::tuple<uint32_t, std::shared_ptr<unsigned char[]>, uint64_t> fileBlock;
        {
            std::unique_lock<std::mutex> cv_mt(ptr->lock);
            ptr->cv->wait(cv_mt, [this, ptr]{
                if (this->stop || !ptr->blockStore.empty()) {
                    if (!ptr->blockStore.empty() && std::get<0>(*ptr->blockStore.begin()) == ptr->targetBlockIndex) {
                        return true;
                    }
                }
                return false;
            });
            if (stop)
                return ;
            fileBlock = *ptr->blockStore.begin();
            ptr->blockStore.erase(ptr->blockStore.begin());
        }
        CipherUtils& cipherUtils = CipherUtils::getInstance();
        cipherUtils.updateFileSha256(transferID, (void *)std::get<1>(fileBlock).get(), std::get<2>(fileBlock));
        ptr->targetBlockIndex ++;
    }
    ptr->computeFileLock.unlock();
}

void FileManager::close() {
    stop = true;
    clock.notify_all();
}

Transfer::FileInfo FileManager::getServerFileInfo(uint32_t fileIndex) {
    Transfer::FileInfo fileInfo;
    fileInfo.CopyFrom(fileList.fileinfos(fileIndex));
    return std::move(fileInfo);
}


