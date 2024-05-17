# 简述

该软件由c++与grpc开发完成。支持文件的上传与下载。支持文件效验，多线程传输。

自己测试下来，传输文件的上限为4GB，我也不知道怎么设置成更大的。

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

### 下载与编译

```
git clone https://github.com/ghost-him/SwiftTransfer.git
```

```
cd SwiftTransfer
```

```
cmake -B build
```

```
cmake --build build -j 4
```

客户端在`./build/client`文件夹下，服务端在`./build/server`文件夹下

## 使用方法

### 服务器

运行可执行文件后，输入默认的文件块的大小（默认为2MB），之后服务器会监听`127.0.0.1:12345`端口。

### 客户端

运行可执行文件后，要手动输入服务端所监听的地址与端口，之后即可完成初始化。

客户端会读取服务端中存在的文件，然后显示出来。

