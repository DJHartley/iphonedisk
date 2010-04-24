// Author: Allen Porter <allen@thebends.org>

#include <string>

struct fuse_operations;
namespace proto {
class FsService;
}

namespace fs {

class Filesystem {
 public:
  Filesystem();

  // Destroy the filesystem, and unmount it if it was previously mounted
  virtual ~Filesystem();

  // Mount the filesystem with the specified volume name and return immediately.
  virtual bool Mount() = 0;

  // Unmount the filesyste, after it has previously been mounted.
  virtual void Unmount() = 0;

  // Block until the filesystem is unmounted by a third party
  virtual void WaitForUnmount() = 0;
};

}  // namespace fs
