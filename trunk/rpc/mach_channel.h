// Author: Allen Porter <allen@thebends.org>
//
// An RpcChannel implementation on top of mach.

#include <string>

#ifndef __RPC_MACH_CHANNEL_H__
#define __RPC_MACH_CHANNEL_H__

namespace google {
namespace protobuf {
class RpcChannel;
}
}

namespace rpc {

// Creates a new RpcChannel for the specified service name, or returns NULL
// if the service is not found.  This channel only supports blocking operations.
google::protobuf::RpcChannel* NewMachChannel(const std::string& service_name);

}  // namespace rpc

#endif // __RPC_MACH_CHANNEL_H__
