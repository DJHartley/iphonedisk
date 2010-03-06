// Author: Allen Porter <allen@thebends.org>

#include <proto/fs.pb.h>
#include <proto/fs_service.pb.h>
#include <fuse.h>

namespace fs {

void Initialize(proto::FsService* service,
                struct fuse_operations* fuse_op) {

}

}  // namespace fs
