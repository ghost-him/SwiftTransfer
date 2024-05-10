//
// Created by ghost-him on 24-5-5.
//

#ifndef SWIFTTRANSFER_SERVERVIEW_H
#define SWIFTTRANSFER_VIEW_H

#include "../../grpc/entity.pb.h"
#include <unordered_map>
#include <functional>

const int maxUI = 3;


class ClientView {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    ClientView(ClientView const&) = delete;
    ClientView& operator=(ClientView const&) = delete;
    // 提供一个全局唯一的接口
    static ClientView& getInstance() {
        static ClientView instance;
        return instance;
    }

    void printInitPage();

    void clearWindow();

    void printTitle();

    bool printMainPage(uint32_t pageIndex);

    void printUploadPage();

    void printDownloadPage();

private:
    int page;
    ClientView() = default;
};


#endif //SWIFTTRANSFER_SERVERVIEW_H
