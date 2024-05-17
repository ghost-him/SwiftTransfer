cmake_minimum_required(VERSION 3.21)
project(SwiftTransfer)

set(CMAKE_CXX_STANDARD 20)

# 找到 Protobuf 包
find_package(protobuf REQUIRED CONFIG)
# 找到 gRPC 包
find_package(gRPC REQUIRED)
find_package(absl REQUIRED)
find_package(Boost COMPONENTS date_time REQUIRED)
find_package(SOCI REQUIRED COMPONENTS sqlite3)

aux_source_directory(IDGenerator SRC)
aux_source_directory(grpc SRC)
aux_source_directory(FileManager SRC)
aux_source_directory(FileManager/fstreamPool SRC)
aux_source_directory(AppConfig SRC)
aux_source_directory(database SRC)
aux_source_directory(ThreadPool SRC)
aux_source_directory(CipherUtils SRC)

aux_source_directory(server server_SRC)
aux_source_directory(server/ServerView server_SRC)

aux_source_directory(client client_SRC)
aux_source_directory(client/ClientView client_SRC)

# 设置输出的目标文件夹

add_executable(server ${SRC} ${server_SRC})
set_target_properties(server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/server")

add_executable(client ${SRC} ${client_SRC})
set_target_properties(client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/client")


target_link_libraries(server PUBLIC gRPC::grpc++ gRPC::grpc++_reflection gRPC::gpr gRPC::grpc gRPC::grpc++ protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite absl::strings absl::base)
target_link_libraries(client PUBLIC gRPC::grpc++ gRPC::grpc++_reflection gRPC::gpr gRPC::grpc gRPC::grpc++ protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite absl::strings absl::base)
target_link_libraries(server PUBLIC $<IF:$<TARGET_EXISTS:SOCI::soci_sqlite3>,SOCI::soci_sqlite3,SOCI::soci_sqlite3_static>)
target_link_libraries(client PUBLIC $<IF:$<TARGET_EXISTS:SOCI::soci_sqlite3>,SOCI::soci_sqlite3,SOCI::soci_sqlite3_static>)
target_link_libraries(server PUBLIC cryptopp)
target_link_libraries(client PUBLIC cryptopp)
