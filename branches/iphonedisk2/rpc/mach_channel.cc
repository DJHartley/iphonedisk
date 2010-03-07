// Author: Allen Porter <allen@thebends.org>

#include "rpc/mach_channel.h"
#include "rpc/proto_rpc_types.h"
#include "rpc/proto_rpc.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

namespace rpc {

class MachChannel : public google::protobuf::RpcChannel {
 public:
  MachChannel(mach_port_t port) : port_(port) { }

  virtual ~MachChannel() {
    mach_port_deallocate(mach_task_self(), port_);
  }

  virtual void CallMethod(
      const google::protobuf::MethodDescriptor* method,
      google::protobuf::RpcController* controller,
      const google::protobuf::Message* request,
      google::protobuf::Message* response,
      google::protobuf::Closure* done) {
    const std::string& service_name = method->service()->full_name();
    const std::string& method_name = method->full_name();
printf("Method name: %s", method_name.c_str());
    std::string request_bytes;
    if (!request->SerializeToString(&request_bytes)) {
      controller->SetFailed("Request parse error");
      done->Run();
      return;
    }
    int status;
    char* response_bytes;
    mach_msg_type_number_t response_bytes_size;
    kern_return_t kr = call_method(port_,
        (char*) service_name.data(), service_name.size(),
        (char*) method_name.data(), method_name.size(),
        (char*) request_bytes.data(), request_bytes.size(),
        &status, &response_bytes, &response_bytes_size);
    if (kr != KERN_SUCCESS) {
      mach_error("call_method:", kr);
      controller->SetFailed("RPC error");
    } else if (status == 0) {
      if (!response->ParseFromArray(response_bytes, response_bytes_size)) {
        controller->SetFailed("Response parse error");
      }
    } else {
      controller->SetFailed("Error from server");
    }
    done->Run();
  }

 private:
  mach_port_t port_;
};

google::protobuf::RpcChannel* NewMachChannel(const std::string& service_name) {
  kern_return_t kr;
  mach_port_t port;
  char* name = (char*)malloc(service_name.size());
  strncpy(name, service_name.data(), service_name.size());
  if ((kr = bootstrap_look_up(bootstrap_port, name,
                              &port)) != BOOTSTRAP_SUCCESS) {
    free(name);
    mach_error("bootstrap_look_up:", kr);
    return NULL;
  }
  free(name);
  return new MachChannel(port);
}

}  // namespace rpc
