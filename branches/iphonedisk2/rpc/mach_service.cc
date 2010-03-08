// Author: Allen Porter <allen@thebends.org>

#include "rpc/mach_service.h"

#include <iostream>
#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <sys/syslog.h>
#include "rpc/proto_rpc_server.h"
#include "rpc/rpc.h"

extern boolean_t proto_rpc_server(
    mach_msg_header_t *InHeadP,
    mach_msg_header_t *OutHeadP);

namespace {

google::protobuf::Service* g_service = NULL;
const google::protobuf::ServiceDescriptor* g_service_desc = NULL;

static const google::protobuf::MethodDescriptor* FindMethod(
      const std::string& service_name,
      const std::string& method_name) {
  if (g_service_desc->name() != service_name) {
    std::cerr << "Unknown service name" << service_name << std::endl;
    return NULL;
  }
  const google::protobuf::MethodDescriptor* method =
      g_service_desc->FindMethodByName(method_name);
  if (method == NULL) {
    std::cerr << "Unknown method name: " << method_name << std::endl;
    return NULL;
  }
  return method;
}

static void AllocateProtocolMessages(
      const google::protobuf::MethodDescriptor* method_desc, 
      google::protobuf::Message** request,
      google::protobuf::Message** response) {
  const google::protobuf::Message& request_prototype =
      g_service->GetRequestPrototype(method_desc);
  const google::protobuf::Message& response_prototype =
      g_service->GetResponsePrototype(method_desc);
  *request = request_prototype.New();
  *response = response_prototype.New();
}

}  // namespace

kern_return_t call_method(
    mach_port_t server_port,
    string_t service_name,
    mach_msg_type_number_t service_nameCnt,
    string_t method_name,
    mach_msg_type_number_t method_nameCnt,
    string_t request_bytes,
    mach_msg_type_number_t request_bytesCnt,
    int32_t* status,
    string_t* response_bytes,
    mach_msg_type_number_t* response_bytesCnt) {
  // Static storage for the bytes in the return value.  This response protocol
  // buffer is serialized to this string which needs to stick around so that
  // mach can read it.
  static std::string* raw_response = NULL;
  if (raw_response == NULL) {
    raw_response = new std::string;
  }

  assert(g_service != NULL);
  const google::protobuf::MethodDescriptor* method =
      FindMethod(std::string(service_name, service_nameCnt),
                 std::string(method_name, method_nameCnt));
  if (method == NULL) {
    // Method or service name was incorrect
    return KERN_INVALID_ARGUMENT;
  }
  google::protobuf::Message* request;
  google::protobuf::Message* response;
  AllocateProtocolMessages(method, &request, &response);

  rpc::Rpc rpc;
  kern_return_t kr;
  if (!request->ParseFromArray(request_bytes, request_bytesCnt)) {
    std::cerr << "Unable to parse request message" << std::endl;
    kr = KERN_INVALID_ARGUMENT;
  } else {
    g_service->CallMethod(method, &rpc, request, response,
                          google::protobuf::NewCallback(
                              &google::protobuf::DoNothing));
    if (rpc.Failed()) {
      // TODO(aporter): The rpc failure reason could be serialized into the
      // response, perhaps.
      kr = KERN_SUCCESS;
      *status = -1;
    } else {
      if (!response->AppendToString(raw_response)) {
        std::cerr << "Unable to serialize response message" << std::endl;
        kr = KERN_INVALID_ARGUMENT;
      } else {
        kr = KERN_SUCCESS;
        *status = 0;
        *response_bytes = (char*)raw_response->data();
        *response_bytesCnt = raw_response->size();
      }
    }
  }
  delete request;
  delete response;
  return kr;
}


namespace rpc {

bool ExportService(const std::string& service_name,
                   google::protobuf::Service* service) {
  kern_return_t kr;
  mach_port_t server_port;
  kr = bootstrap_check_in(bootstrap_port, (char*)service_name.c_str(),
                          &server_port);
  if (kr != KERN_SUCCESS) {
    mach_error("bootstrap_check_in:", kr);
    return false;
  }
  std::cout << "mach_msg_server: starting " << service_name << std::endl;
  g_service = service;
  g_service_desc = service->GetDescriptor();
  kr = mach_msg_server(proto_rpc_server,
      sizeof(__Request__call_method_t), server_port, MACH_MSG_TIMEOUT_NONE);
  if (kr != KERN_SUCCESS) {
    mach_error("mach_msg_server:", kr);
  }

  // TODO(allen): This doesn't seem quite correct.  mach_port_destroy?  This
  // code probably never gets run anyway since the server never exits, but
  // clean shutdown should be possible.
  mach_port_deallocate(mach_task_self(), server_port);
  g_service = NULL;
  return true;
}

}  // namespace rpc
