//
// Created by ghost-him on 24-5-7.
//

#include "ServerView.h"
#include <iostream>
#include "../../AppConfig/AppConfig.h"

void ServerView::printTitle() {
    std::wcout << R"(  ____               _    __   _     _____                                 __               )" << std::endl;
    std::wcout << R"( / ___|  __      __ (_)  / _| | |_  |_   _|  _ __    __ _   _ __    ___   / _|   ___   _ __ )" << std::endl;
    std::wcout << R"( \___ \  \ \ /\ / / | | | |_  | __|   | |   | '__|  / _` | | '_ \  / __| | |_   / _ \ | '__|)" << std::endl;
    std::wcout << R"(  ___) |  \ V  V /  | | |  _| | |_    | |   | |    | (_| | | | | | \__ \ |  _| |  __/ | |   )" << std::endl;
    std::wcout << R"( |____/    \_/\_/   |_| |_|    \__|   |_|   |_|     \__,_| |_| |_| |___/ |_|    \___| |_|   )" << std::endl;
}

void ServerView::printInitPage() {
    // 设置全局locale为当前系统的locale
    std::locale::global(std::locale(""));
    // 将wcout的locale设置为全局locale
    std::wcout.imbue(std::locale());

    AppConfig& appConfig = AppConfig::getInstance();

    printTitle();
    std::wcout << L"请输入传输的文件块大小，默认:2097152(B)";
    std::wstring str;
    getline(std::wcin, str);
    if (str.empty())
        str = L"2097152";
    uint32_t fileBlockSize = std::stoi(str);
    appConfig.set("fileBlockSize", std::to_string(fileBlockSize));
    std::wcout << L"设置完成" << std::endl;
}