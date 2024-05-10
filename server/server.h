//
// Created by ghost-him on 24-4-29.
//

#ifndef SWIFTTRANSFER_SERVER_H
#define SWIFTTRANSFER_SERVER_H


#include "../grpc/service.grpc.pb.h"

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
    Status sendFileVerification(ServerContext* context, const FileDigest* fileDigest, RequestFileBlockList* requestFileBlockList) override ;
    Status endSendUploadFile(ServerContext* context, const TransferID* transferId, Placeholder* placeholder) override ;

    void initFileDirectory();

private:
    const std::string fileDirectoryPath = "./files/";

};



#endif //SWIFTTRANSFER_SERVER_H
