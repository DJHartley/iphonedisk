// Author: Allen Porter <allen@thebends.org>
//
// A Filesystem implementation that proxies all calls to the specified
// FsService and mounts the filesystem using FUSE.

#ifndef __FS_FS_PROXY_H__
#define __FS_FS_PROXY_H__

#include <string>

namespace proto {
  class FsService;
}

namespace fs {

class Filesystem;

Filesystem* NewProxyFilesystem(proto::FsService* service,
                               const std::string& fs_id,
                               const std::string& volname,
                               const std::string& volicon);

}  // namespace fs

#endif  // __FS_FS_PROXY_H__
