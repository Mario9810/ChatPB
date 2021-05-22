all: server client

server: server.cpp payload.pb.cc
	g++ -o server server.cpp payload.pb.cc -lpthread -lprotobuf

client: cliente.cpp payload.pb.cc
	g++ -o cliente cliente.cpp payload.pb.cc -lpthread -lprotobuf

payload.pb.cc: payload.proto
	protoc -I=. --cpp_out=. payload.proto
