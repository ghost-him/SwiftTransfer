# 简述

> 目前还在开发，已完成：上传文件，下载文件
> 
> 还需完成：客户端离线时清除维护信息，更多的传输提示信息（传输速率，剩余时间等）

该软件由c++与grpc共同开发完成。支持文件的上传与下载。支持断点传输，文件效验，多线程传输。

## 功能

* 文件上传
* 文件下载
* 查看文件列表

## 安装方法

### 运行库安装

要按照以下的顺序安装

* sqlite3开发库
* [boost](https://www.boost.org/users/download/)
* [absl](https://github.com/abseil/abseil-cpp)
* [soci](https://github.com/SOCI/soci)
* [cryptopp](https://github.com/weidai11/cryptopp/)
* [protobuf](https://github.com/protocolbuffers/protobuf#protobuf-compiler-installation)
* [grpc](https://grpc.io/docs/languages/cpp/quickstart/)

## 使用方法

### 服务器

运行可执行文件后，输入默认的文件块的大小（默认为2MB），之后服务器会监听`127.0.0.1:12345`端口。

### 客户端

运行可执行文件后，要手动输入服务端所监听的地址与端口，之后即可完成初始化。

# ===========以下过期，待更新========


## 数据库设计

文件在服务端存储时，将文件id作为本地的名字；客户端在本地存储时，才存放原本的名字。

（客户端管理实现）该表用于管理当前的在线用户

```
int32 clientID	// 客户端id
int64 updateTime	// 上次心跳检测的时间
```

（客户端管理实现）该表用于管理一个用户正在进行的传输

```sqlite
int32 clientID
int32 transferID
```

该表用于管理当前正在传输的文件

```sqlite
int32 transferID
int32 fileID
int64 fileSize
string fileName
string digest
int64 startTransferTime
```

该表用于管理已经存储下来的文件（服务端用）

```sqlite3
int32 fileID
int64 fileSize
string fileName
string digest
int64 uploadTime
```





