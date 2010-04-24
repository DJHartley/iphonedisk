// Author: Allen Porter

#include <string>

#ifndef __MOUNT_MOUNT_SERVICE_H__
#define __MOUNT_MOUNT_SERVICE_H__

namespace proto {
class FsService;
class MountService;
}

namespace mount {

// Takes ownership of fs_service
proto::MountService* NewMountService(proto::FsService* fs_service,
                                     const std::string& volicon);

}  // namespace

#endif  // __MOUNT_MOUNT_SERVICE_H__
