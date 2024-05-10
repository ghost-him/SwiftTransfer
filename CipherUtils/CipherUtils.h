//
// Created by ghost-him on 24-5-6.
//

#ifndef SWIFTTRANSFER_CIPHERUTILS_H
#define SWIFTTRANSFER_CIPHERUTILS_H

#include <string>
#include <unordered_map>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

class CipherUtils {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    CipherUtils(CipherUtils const&) = delete;
    CipherUtils& operator=(CipherUtils const&) = delete;
    // 提供一个全局唯一的接口
    static CipherUtils& getInstance() {
        static CipherUtils instance;
        return instance;
    }

    std::string getStringSha256(void * data, uint64_t dataSize);

    void createFileSha256(uint32_t fileID);

    void updateFileSha256(uint32_t fileID, void * data, uint64_t dataSize);

    std::string getFileSha256(uint32_t fileID);

    void deleteFileSha256(uint32_t fileID);


private:
    CipherUtils() = default;
    std::unordered_map<uint32_t, CryptoPP::SHA256> sha256Map;
    std::unordered_map<uint32_t, std::string> fileSha256Store;

};


#endif //SWIFTTRANSFER_CIPHERUTILS_H
