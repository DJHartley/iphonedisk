// Author: Allen Porter <allen@thebends.org>

#include <string>

struct fuse_operations;
namespace proto {
class FsService;
}

namespace fs {

bool MountFilesystem(proto::FsService* service,
                     const std::string& fs_id,
                     const std::string& volname);

}
