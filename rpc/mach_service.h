// Author: Allen Porter <allen@thebends.org>

#ifndef __RPC_MACH_SERVICE_H__
#define __RPC_MACH_SERVICE_H__

#include <string>
#include <mach/mach.h>

namespace google {
namespace protobuf {
class Service;
}
}

namespace rpc {

// A blocking call.  Registeres a new service with the specified name and starts
// the network loop.  Returns false if starting the server failed.  This method
// can only be called by a single thread.  This function keeps a reference
// to the service until it returns.
bool ExportService(const std::string& service_name,
                   google::protobuf::Service* service);

}  // namespace rpc

#endif  // __RPC_MACH_SERVICE_H__
