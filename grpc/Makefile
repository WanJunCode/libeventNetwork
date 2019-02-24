# HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
# SYSTEM ?= $(HOST_SYSTEM)
# CXX = g++
# CPPFLAGS += `pkg-config --cflags protobuf grpc`
# CXXFLAGS += -std=c++11
# ifeq ($(SYSTEM),Darwin)
# LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
#            -lgrpc++_reflection\
#            -ldl
# else
# LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
#            -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
#            -ldl
# endif
# PROTOC = protoc
# GRPC_CPP_PLUGIN = grpc_cpp_plugin
# GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

# PROTOS_PATH = .

# vpath %.proto $(PROTOS_PATH)

# all: client server

# client: keyvaluestore.pb.o keyvaluestore.grpc.pb.o client.o
# 	$(CXX) $^ $(LDFLAGS) -o $@


# # greeter_client: helloworld.pb.o helloworld.grpc.pb.o greeter_client.o

# server: keyvaluestore.pb.o keyvaluestore.grpc.pb.o server.o
# 	$(CXX) $^ $(LDFLAGS) -o $@

# # precious 指定后 
# .PRECIOUS: %.grpc.pb.cc
# %.grpc.pb.cc: %.proto
# 	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

# .PRECIOUS: %.pb.cc
# %.pb.cc: %.proto
# 	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

# clean:
# 	rm -f *.o *.pb.cc *.pb.h client server

