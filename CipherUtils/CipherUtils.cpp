//
// Created by ghost-him on 24-5-6.
//

#include "CipherUtils.h"

std::string CipherUtils::getStringSha256(void *data, uint64_t dataSize) {
    CryptoPP::SHA256 sha256;
    CryptoPP::HexEncoder hexEncoder;

    std::string hashValue;
    hashValue.resize(CryptoPP::SHA256::DIGESTSIZE * 2 + 1);

    CryptoPP::ArraySink sink((CryptoPP::byte*)hashValue.data(), hashValue.size());
    CryptoPP::ArraySource((const CryptoPP::byte*) data, dataSize, true,  new CryptoPP::HashFilter(sha256, new CryptoPP::Redirector(sink)));
    // 去除末尾的空字符
    hashValue.resize(CryptoPP::SHA256::DIGESTSIZE * 2);
    return hashValue;
}

void CipherUtils::createFileSha256(uint32_t fileID) {
    if (sha256Map.contains(fileID)) {
        throw std::runtime_error("duplicate fileID: " + std::to_string(fileID));
    }
    sha256Map[fileID] = {};
}

void CipherUtils::updateFileSha256(uint32_t fileID, void *data, uint64_t dataSize) {
    CryptoPP::SHA256& sha256 = sha256Map[fileID];
    sha256.Update((const CryptoPP::byte*)data, dataSize);
}

std::string CipherUtils::getFileSha256(uint32_t fileID) {
    if (fileSha256Store.contains(fileID))
        return fileSha256Store[fileID];
    unsigned char buffer[CryptoPP::SHA256::DIGESTSIZE];
    CryptoPP::SHA256& sha256 = sha256Map[fileID];
    sha256.Final(buffer);
    std::string result;
    CryptoPP::StringSource(buffer, sha256.DigestSize(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(result)));
    fileSha256Store[fileID] = result;
    return result;
}

void CipherUtils::deleteFileSha256(uint32_t fileID) {
    if (sha256Map.contains(fileID)) {
        sha256Map.erase(fileID);
    } else {
        throw std::runtime_error("invalid fileID: " + std::to_string(fileID));
    }
    if (fileSha256Store.contains(fileID)) {
        fileSha256Store.erase(fileID);
    }
}

