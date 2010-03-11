// Author: Allen Porter <allen@thebends.org>

#include <string>

struct fuse_operations;
namespace proto {
class FsService;
}

namespace fs {

void Initialize(proto::FsService* service, const std::string& fs_id, 
                struct fuse_operations* fuse_op);

}
