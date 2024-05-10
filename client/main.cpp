#include "ClientView/ClientView.h"
#include "../database/database.h"
#include "../AppConfig/AppConfig.h"
#include "../ThreadPool/threadpool.h"
#include "../FileManager/FileManager.h"


int main() {
    ThreadPool& threadPool = ThreadPool::getInstance();
    AppConfig& config = AppConfig::getInstance();
    FileManager& fileManager = FileManager::getInstance();
    /////////////
    database::getInstance().init();

    threadPool.commit([&](){
        fileManager.writeThread();
    });
    //////////////
    ClientView& view = ClientView::getInstance();
    view.printInitPage();

    while(true) {
        if (!view.printMainPage(1))
            break;
    }

    return 0;

}

