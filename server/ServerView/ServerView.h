//
// Created by ghost-him on 24-5-7.
//

#ifndef SWIFTTRANSFER_SERVERVIEW_H
#define SWIFTTRANSFER_SERVERVIEW_H


class ServerView {
public:
    // 删除拷贝构造函数和拷贝赋值操作符
    ServerView(ServerView const&) = delete;
    ServerView& operator=(ServerView const&) = delete;
    // 提供一个全局唯一的接口
    static ServerView& getInstance() {
        static ServerView instance;
        return instance;
    }

    void printTitle();

    void printInitPage();

private:
    ServerView() = default;
};


#endif //SWIFTTRANSFER_SERVERVIEW_H
