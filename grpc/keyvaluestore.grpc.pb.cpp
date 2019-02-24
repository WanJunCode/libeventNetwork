// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: keyvaluestore.proto

#include "keyvaluestore.pb.h"
#include "keyvaluestore.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace keyvaluestore {

static const char* KeyValueStore_method_names[] = {
  "/keyvaluestore.KeyValueStore/GetValues",
};

std::unique_ptr< KeyValueStore::Stub> KeyValueStore::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< KeyValueStore::Stub> stub(new KeyValueStore::Stub(channel));
  return stub;
}

KeyValueStore::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_GetValues_(KeyValueStore_method_names[0], ::grpc::internal::RpcMethod::BIDI_STREAMING, channel)
  {}

::grpc::ClientReaderWriter< ::keyvaluestore::Request, ::keyvaluestore::Response>* KeyValueStore::Stub::GetValuesRaw(::grpc::ClientContext* context) {
  return ::grpc::internal::ClientReaderWriterFactory< ::keyvaluestore::Request, ::keyvaluestore::Response>::Create(channel_.get(), rpcmethod_GetValues_, context);
}

void KeyValueStore::Stub::experimental_async::GetValues(::grpc::ClientContext* context, ::grpc::experimental::ClientBidiReactor< ::keyvaluestore::Request,::keyvaluestore::Response>* reactor) {
  ::grpc::internal::ClientCallbackReaderWriterFactory< ::keyvaluestore::Request,::keyvaluestore::Response>::Create(stub_->channel_.get(), stub_->rpcmethod_GetValues_, context, reactor);
}

::grpc::ClientAsyncReaderWriter< ::keyvaluestore::Request, ::keyvaluestore::Response>* KeyValueStore::Stub::AsyncGetValuesRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq, void* tag) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::keyvaluestore::Request, ::keyvaluestore::Response>::Create(channel_.get(), cq, rpcmethod_GetValues_, context, true, tag);
}

::grpc::ClientAsyncReaderWriter< ::keyvaluestore::Request, ::keyvaluestore::Response>* KeyValueStore::Stub::PrepareAsyncGetValuesRaw(::grpc::ClientContext* context, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncReaderWriterFactory< ::keyvaluestore::Request, ::keyvaluestore::Response>::Create(channel_.get(), cq, rpcmethod_GetValues_, context, false, nullptr);
}

KeyValueStore::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      KeyValueStore_method_names[0],
      ::grpc::internal::RpcMethod::BIDI_STREAMING,
      new ::grpc::internal::BidiStreamingHandler< KeyValueStore::Service, ::keyvaluestore::Request, ::keyvaluestore::Response>(
          std::mem_fn(&KeyValueStore::Service::GetValues), this)));
}

KeyValueStore::Service::~Service() {
}

::grpc::Status KeyValueStore::Service::GetValues(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::keyvaluestore::Response, ::keyvaluestore::Request>* stream) {
  (void) context;
  (void) stream;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace keyvaluestore

