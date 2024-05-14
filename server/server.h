//
// Created by ghost-him on 24-4-29.
//

#ifndef SWIFTTRANSFER_SERVER_H
#define SWIFTTRANSFER_SERVER_H


#include "../grpc/service.grpc.pb.h"
#include <queue>

#define Default_Send_Thread 3


using namespace grpc;
using namespace Transfer;

class StreamServiceImpl final : public StreamService::Service {
public:
    Status getConfiguration(ServerContext* context, const Placeholder* placeholder, Configuration* writer) override ;
    Status getFileList(ServerContext* context, const PageIndex* pageIndex, FileList* writer) override ;
    Status getUniqueID(ServerContext* context, const Placeholder* placeholder, UniqueID* writer) override ;
    Status HeartbeatSignal(ServerContext* context, const Placeholder* placeholder, Placeholder* writer) override ;

    Status sendUploadFileInfo(ServerContext* context, const StartTransfer* startTransfer, Placeholder* placeholder) override ;
    Status sendUploadFileBlock(ServerContext* context, ServerReader<FileBlock>* reader, Placeholder* placeholder) override ;
    Status transferUploadFileSHA(ServerContext* context, const FileDigest* fileDigest, RequestFileBlockList* requestFileBlockList) override ;
    Status endSendUploadFile(ServerContext* context, const TransferID* transferId, Placeholder* placeholder) override ;

    Status getDownloadTransferID(ServerContext* context, const FileID* fileID, TransferID* writer) override;
    Status transferDownloadFile(ServerContext* context, const TransferID* transferID, ServerWriter<FileBlock>* writer) override;
    Status transferDownloadFileSHA(ServerContext* context, const FileDigest* fileDigest, ServerWriter<FileBlock>* writer) override;
    Status endDownloadFile(ServerContext* context, const TransferID* transferID, Placeholder* writer) override;



    void initFileDirectory();

private:
    struct SendFileList {
        std::queue<uint32_t> FileBlockToSend;
        std::mutex lock;
    };
    const std::string fileDirectoryPath = "./files/";

    uint32_t getNextBlockIndex(std::shared_ptr<SendFileList> ptr);

    std::unordered_map<uint32_t, std::shared_ptr<SendFileList>> sendFileListMap;

};



#endif //SWIFTTRANSFER_SERVER_H
