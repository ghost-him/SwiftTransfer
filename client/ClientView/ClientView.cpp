//
// Created by ghost-him on 24-5-5.
//

#include "ClientView.h"
#include <iostream>
#include "../../client/client.h"
#include "../../FileManager/FileManager.h"


bool ClientView::printMainPage(uint32_t pageIndex) {
    page = pageIndex;
    FileManager& fileManager = FileManager::getInstance();
    clearWindow();
    printTitle();

    // 计算每一列的实际宽度,根据窗口大小动态调整
    size_t fileNameWidth = 20;
    size_t fileSizeWidth = 15;
    size_t sha256Width = 40;

    // 输出表格
    std::wcout << std::left << std::setw(fileNameWidth) << "File Name"
              << std::setw(fileSizeWidth) << "File Size(B)"
              << std::setw(sha256Width) << "SHA256"
              << std::endl;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
    Transfer::FileList fileList = std::move(StreamServiceClient::getInstance().getFileList(pageIndex));
    std::wcout << fileList.fileinfos().size() << std::endl;
    fileManager.updateFileList(fileList);
    auto& fileInfos = fileList.fileinfos();
    for (int i = 0; i < 9; i ++) {
        if (i < fileInfos.size()) {
            const std::string& fileName = fileInfos.at(i).filename();
            const std::wstring wfileName = converter.from_bytes(fileName);

            const uint32_t fileSize = fileInfos.at(i).filesize();
            const std::string& sha256 = fileInfos.at(i).digest();
            const std::wstring wsha256 = converter.from_bytes(sha256);
            std::wcout << std::left << std::setw(fileNameWidth) << wfileName
                       << std::setw(fileSizeWidth) << fileSize
                       << std::setw(sha256Width) << wsha256;
        }
        std::wcout << "\n";
    }
    std::wcout << L"[u]: 上传文件 [d]: 下载文件 [q]: 退出程序 [l]: 向左翻页 [r]: 向右翻页\n";
    std::wcout.flush();
    std::wstring str;
    getline(std::wcin, str);
    if (str == L"u") {
        printUploadPage();
    } else if (str == L"d") {
        printDownloadPage();
    } else if (str == L"q") {
        return false;
    } else if (str == L"l") {
        printMainPage(page + 1);
    } else if (str == L"r") {
        printMainPage((page - 1 > 0? page - 1: page));
    } else {
        std::wcout << L"无效命令" << std::endl;
    }
    return true;
}

void ClientView::printUploadPage() {
    clearWindow();
    printTitle();

    std::wstring str;
    std::wcout << L"输入[q]返回上级" << std::endl;

    while(true) {
        std::wcout << L"请输入要传输的文件的地址： " << std::endl;
        getline(std::wcin, str);
        if (str == L"q") {
            break;
        }
        std::filesystem::path path(str);

        FileManager& fileManager = FileManager::getInstance();
        if (!fileManager.isValid(path)) {
            std::wcout << L"无效地址: " << str << std::endl;
            continue;
        }

        std::wcout << L"===========================\n";
        std::wcout << L"请确认以下的文件信息：\n";
        std::wcout << L"文件路径：" << str << "\n";
        std::wcout << L"文件名字：" << path.filename().wstring() << "\n";
        std::wcout << L"文件大小：" << std::filesystem::file_size(path) << " B\n";
        std::wcout << L"如果正确，则输入y，否则输入n：";
        getline(std::wcin, str);
        if (str == L"y") {
            std::wcout << L"开始上传文件" << std::endl;
            // 上传该文件
            StreamServiceClient::getInstance().uploadFile(path);
        } else if (str == L"n"){
            std::wcout << L"取消上传" << std::endl;
        } else {
            std::wcout << L"无效输入，取消上传" << std::endl;
        }
    }
}


void ClientView::printTitle() {
    std::wcout << R"(  ____               _    __   _     _____                                 __               )" << std::endl;
    std::wcout << R"( / ___|  __      __ (_)  / _| | |_  |_   _|  _ __    __ _   _ __    ___   / _|   ___   _ __ )" << std::endl;
    std::wcout << R"( \___ \  \ \ /\ / / | | | |_  | __|   | |   | '__|  / _` | | '_ \  / __| | |_   / _ \ | '__|)" << std::endl;
    std::wcout << R"(  ___) |  \ V  V /  | | |  _| | |_    | |   | |    | (_| | | | | | \__ \ |  _| |  __/ | |   )" << std::endl;
    std::wcout << R"( |____/    \_/\_/   |_| |_|    \__|   |_|   |_|     \__,_| |_| |_| |___/ |_|    \___| |_|   )" << std::endl;
}

void ClientView::clearWindow() {
#ifdef __linux__
    system("clear");
#elif _WIN32
    system("cls");
#endif

}

void ClientView::printInitPage() {
    StreamServiceClient& client = StreamServiceClient::getInstance();
    // 设置全局locale为当前系统的locale
    std::locale::global(std::locale(""));
    // 将wcout的locale设置为全局locale
    std::wcout.imbue(std::locale());

    clearWindow();
    printTitle();

    std::wcout << L"请输入目标服务器的ip" << std::endl;
    std::wstring wip;
    getline(std::wcin, wip);
    std::string ip = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(wip);
    client.connect(ip);
    std::wcout << L"程序初始化" << std::endl;
    client.getConfiguration();
    std::wcout << L"初始化成功，跳转页面" << std::endl;
    printMainPage(1);
}

void ClientView::printDownloadPage() {

}
