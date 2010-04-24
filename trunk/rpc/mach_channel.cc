// Author: Allen Porter <allen@thebends.org>

#include "rpc/mach_channel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <syslog.h>
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
      syslog(LOG_ERR, "%s.%s serialization error",
             service_name.c_str(), method_name.c_str());
      controller->SetFailed("Request serialization error");
      done->Run();
      return;
    }
    int status = -1;
    string_t response_bytes = NULL;
    unsigned int response_bytes_size = 0;
    kern_return_t kr = call_method(port_,
        (string_t) service_name.data(), service_name.size(),
        (string_t) method_name.data(), method_name.size(),
        (string_t) request_bytes.data(), request_bytes.size(),
        &status, &response_bytes, &response_bytes_size);
    if (kr != KERN_SUCCESS) {
      syslog(LOG_ERR, "Unexpected RPC failure: %s", mach_error_string(kr));
      controller->SetFailed(mach_error_string(kr));
    } else if (status == 0) {
      if (!response->ParseFromArray(response_bytes, response_bytes_size)) {
        syslog(LOG_ERR, "%s.%s unable to parse response of size: %d",
               service_name.c_str(), method_name.c_str(), response_bytes_size);
        controller->SetFailed("Response parse error");
      }
    } else {
      std::string detail;
      detail.assign(response_bytes, response_bytes_size);
      syslog(LOG_DEBUG, "%s.%s failed: %s", service_name.c_str(),
             method_name.c_str(), detail.c_str());
      controller->SetFailed(detail);
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
    syslog(LOG_ERR, "bootstrap_look_up failed: %s", mach_error_string(kr));
    return NULL;
  }
  return new MachChannel(port);
}

}  // namespace rpc
