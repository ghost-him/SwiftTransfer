//
// Created by ghost-him on 24-4-29.
//

#include "AppConfig.h"
#include <stdexcept>

void AppConfig::set(const std::string &key, const std::string &value) {
    if (store.contains(key)) {
        throw std::runtime_error("duplicate config key:" + key);
    }
    store[key] = value;
}

const std::string &AppConfig::get(const std::string &key) {
    if (!store.contains(key)) {
        throw std::runtime_error("target key no exist:" + key);
    }
    return store[key];
}
