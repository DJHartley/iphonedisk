// Author: Allen Porter <allen@thebends.org>

#ifndef __RPC_LOOPBACK_FS_SERVER_H__
#define __RPC_LOOPBACK_FS_SERVER_H__

namespace proto {
class FsService;
}

namespace test {

proto::FsService* NewLoopbackService();

}  // namespace test

#endif  // __RPC_LOOPBACK_FS_SERVER_H__
