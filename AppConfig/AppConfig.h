//
// Created by ghost-him on 24-4-29.
//

#ifndef SWIFTTRANSFER_APPCONFIG_H
#define SWIFTTRANSFER_APPCONFIG_H

#include <unordered_map>
#include <string>

class AppConfig {
public :
    // 删除拷贝构造函数和拷贝赋值操作符
    AppConfig(AppConfig const&) = delete;
    AppConfig& operator=(AppConfig const&) = delete;
    // 提供一个全局唯一的接口
    static AppConfig& getInstance() {
        static AppConfig instance;
        return instance;
    }
    // 注册一个
    void set(const std::string& key, const std::string& value);
    const std::string& get(const std::string& key);
private:
    AppConfig() = default;
    std::unordered_map<std::string, std::string> store;
};


#endif //SWIFTTRANSFER_APPCONFIG_H
