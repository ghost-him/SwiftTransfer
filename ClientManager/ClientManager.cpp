//
// Created by ghost-him on 24-4-29.
//

#include "ClientManager.h"
#include <stdexcept>
#include <chrono>
#include <vector>

void ClientManager::addClient(uint32_t clientID) {
    std::unique_lock<std::mutex> lock_guard(lock);
    if (clientID2TransferID.contains(clientID)) {
        throw std::runtime_error((std::string)"duplicate client id:" + std::to_string(clientID));
    }
    clientID2TransferID[clientID] = {};
    int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (heartbeatTime.contains(clientID)) {
        throw std::runtime_error((std::string)"duplicate client id:" + std::to_string(clientID));
    }
    heartbeatTime[clientID] = time;
}

void ClientManager::deleteClient(uint32_t clientID) {
    std::unique_lock<std::mutex> lock_guard(lock);
    if (clientID2TransferID.contains(clientID)) {
        clientID2TransferID.erase(clientID);
    }
    if (heartbeatTime.contains(clientID)) {
        heartbeatTime.erase(clientID);
    }
}

void ClientManager::processHeartbeat(uint32_t clientID) {
    std::unique_lock<std::mutex> lock_guard(lock);
    if (heartbeatTime.contains(clientID)) {
        int64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        heartbeatTime[clientID] = time;
    }
}

void ClientManager::testClient() {
    int64_t nowTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    std::unique_lock<std::mutex> lock_guard(lock);
    std::vector<int32_t> clientIDRemoved;
    // 如果10当前的时间距上次时间超过10s,则说明已经断开了连接
    for (auto& item : heartbeatTime) {
        const int32_t& clientID = item.first;
        uint64_t& lastTime = item.second;
        if ((nowTime - lastTime) / 1000 > 10) {
            clientIDRemoved.push_back(clientID);
        }
    }
    // 将已经断连的id删除
    for (auto& id : clientIDRemoved) {
        deleteClient(id);
    }
}
