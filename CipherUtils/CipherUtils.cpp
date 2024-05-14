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

void CipherUtils::createFileSha256(uint32_t id) {
    if (sha256Map.contains(id)) {
        throw std::runtime_error("duplicate id: " + std::to_string(id));
    }
    sha256Map[id] = {};
}

void CipherUtils::updateFileSha256(uint32_t id, void *data, uint64_t dataSize) {
    CryptoPP::SHA256& sha256 = sha256Map[id];
    sha256.Update((const CryptoPP::byte*)data, dataSize);
}

std::string CipherUtils::getFileSha256(uint32_t id) {
    if (fileSha256Store.contains(id))
        return fileSha256Store[id];
    unsigned char buffer[CryptoPP::SHA256::DIGESTSIZE];
    CryptoPP::SHA256& sha256 = sha256Map[id];
    sha256.Final(buffer);
    std::string result;
    CryptoPP::StringSource(buffer, sha256.DigestSize(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(result)));
    fileSha256Store[id] = result;
    return result;
}

void CipherUtils::deleteFileSha256(uint32_t id) {
    if (sha256Map.contains(id)) {
        sha256Map.erase(id);
    } else {
        throw std::runtime_error("invalid id: " + std::to_string(id));
    }
    if (fileSha256Store.contains(id)) {
        fileSha256Store.erase(id);
    }
}

