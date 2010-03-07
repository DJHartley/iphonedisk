// Author: Allen Porter <allen@thebends.org>

#include "rpc/mach_channel.h"

#include <iostream>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include "rpc/proto_rpc_user.h"

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
    const std::string& method_name = method->name();
    const std::string& service_name = method->service()->name();
    std::string request_bytes;
    if (!request->SerializeToString(&request_bytes)) {
      controller->SetFailed("Request serialization error");
      done->Run();
      return;
    }
    int32_t status;
    string_t response_bytes;
    mach_msg_type_number_t response_bytes_size;
    kern_return_t kr = call_method(port_,
        (string_t) service_name.data(), (mach_msg_type_number_t) service_name.size(),
        (string_t) method_name.data(), (mach_msg_type_number_t) method_name.size(),
        (string_t) request_bytes.data(), (mach_msg_type_number_t) request_bytes.size(),
        &status, &response_bytes, &response_bytes_size);
    if (kr != KERN_SUCCESS) {
      controller->SetFailed(mach_error_string(kr));
    } else if (status == 0) {
      if (!response->ParseFromArray(response_bytes, response_bytes_size)) {
        controller->SetFailed("Response parse error");
      }
    } else {
      char buf[BUFSIZ];
      snprintf(buf, BUFSIZ, "Error from server: %d", status);
      controller->SetFailed(std::string(buf));
    }
    done->Run();
  }

 private:
  mach_port_t port_;
};

google::protobuf::RpcChannel* NewMachChannel(const std::string& service_name) {
  kern_return_t kr;
  mach_port_t port;
  if ((kr = bootstrap_look_up(bootstrap_port, (char*)service_name.c_str(),
                              &port)) != BOOTSTRAP_SUCCESS) {
    mach_error("bootstrap_look_up:", kr);
    return NULL;
  }
  return new MachChannel(port);
}

}  // namespace rpc
