// Author: Allen Porter <allen@thebends.org>

#include "rpc/mach_service.h"

#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <syslog.h>
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
    syslog(LOG_ERR, "Uknown service name: %s", service_name.c_str());
    return NULL;
  }
  const google::protobuf::MethodDescriptor* method =
      g_service_desc->FindMethodByName(method_name);
  if (method == NULL) {
    syslog(LOG_ERR, "Uknown method name: %s", method_name.c_str());
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
  const std::string& service = std::string(service_name, service_nameCnt);
  const std::string& method = std::string(method_name, method_nameCnt);
  const google::protobuf::MethodDescriptor* method_desc =
      FindMethod(service, method);
  if (method_desc == NULL) {
    // Method or service name was incorrect
    return KERN_INVALID_ARGUMENT;
  }
  google::protobuf::Message* request;
  google::protobuf::Message* response;
  AllocateProtocolMessages(method_desc, &request, &response);

  rpc::Rpc rpc;
  kern_return_t kr;
  if (!request->ParseFromArray(request_bytes, request_bytesCnt)) {
    syslog(LOG_ERR, "%s.%s: Unable to parse request message",
           service.c_str(), method.c_str());
    kr = KERN_INVALID_ARGUMENT;
  } else {
    g_service->CallMethod(method_desc, &rpc, request, response,
                          google::protobuf::NewCallback(
                              &google::protobuf::DoNothing));
    if (rpc.Failed()) {
      kr = KERN_SUCCESS;
      *status = -1;
      const std::string& error_text = rpc.ErrorText();
      syslog(LOG_DEBUG, "%s.%s: RPC Error: %s",
             service.c_str(), method.c_str(), error_text.c_str());
      *response_bytes = (char*)error_text.c_str();
      *response_bytesCnt = error_text.size();
    } else {
      if (!response->SerializeToString(raw_response)) {
        syslog(LOG_ERR, "%s.%s: Unable to serialize response message",
               service.c_str(), method.c_str());
        kr = KERN_INVALID_ARGUMENT;
      } else {
        kr = KERN_SUCCESS;
        syslog(LOG_DEBUG, "REQ: %s; RESP: %s",
               request->ShortDebugString().c_str(),
               response->ShortDebugString().c_str());
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
    syslog(LOG_ERR, "bootstrap_check_in: %s", mach_error_string(kr));
    return false;
  }
  syslog(LOG_INFO, "Starting mach service: %s", service_name.c_str());
  g_service = service;
  g_service_desc = service->GetDescriptor();
  kr = mach_msg_server(proto_rpc_server,
      sizeof(__Request__call_method_t), server_port, MACH_MSG_TIMEOUT_NONE);
  if (kr != KERN_SUCCESS) {
    syslog(LOG_ERR, "mach_msg_server: %s", mach_error_string(kr));
  }

  // TODO(allen): This doesn't seem quite correct.  mach_port_destroy?  This
  // code probably never gets run anyway since the server never exits, but
  // clean shutdown should be possible.
  mach_port_deallocate(mach_task_self(), server_port);
  g_service = NULL;
  return (kr == KERN_SUCCESS);
}

}  // namespace rpc
