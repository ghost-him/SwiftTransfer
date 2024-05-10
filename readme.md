# 简述

> 目前还在开发，已完成：上传文件
> 
> 还需完成：1.下载文件 2.客户端离线时清除维护信息

支持文件的上传与下载。支持断点传输，文件效验，多线程传输。

## 功能

* 文件上传
* 文件下载
* 查看文件列表



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





