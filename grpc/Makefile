FILES := $(shell ls *.proto)
all:
	`which protoc` --grpc_out=. --cpp_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $(FILES)