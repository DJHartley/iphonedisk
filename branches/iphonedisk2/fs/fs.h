// Author: Allen Porter <allen@thebends.org>

struct fuse_operations;
namespace proto {
class FsService;
}

namespace fs {

void Initialize(proto::FsService* service, struct fuse_operations* fuse_op);

}
