//
// Created by ghost-him on 24-4-29.
//

#ifndef SWIFTTRANSFER_CLIENTMANAGER_H
#define SWIFTTRANSFER_CLIENTMANAGER_H

#include <cstdint>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>

class ClientManager {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    ClientManager(ClientManager const&) = delete;
    ClientManager& operator=(ClientManager const&) = delete;
    // 提供一个全局唯一的接口
    static ClientManager& getInstance() {
        static ClientManager instance;
        return instance;
    }

    // 添加一个在线用户
    void addClient(uint32_t clientID);
    // 删除一个在线用户
    void deleteClient(uint32_t clientID);
    // 响应心跳检测
    void processHeartbeat(uint32_t clientID);
    // 检测当前的用户
    void testClient();



private:
    ClientManager() = default;
    // 多线程安全
    std::mutex lock;
    // 维护当前的用户正在执行的传输id
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> clientID2TransferID;
    // 维护当前的用户正在执行的
    std::unordered_map<uint32_t, uint64_t> heartbeatTime;
};


#endif //SWIFTTRANSFER_CLIENTMANAGER_H
