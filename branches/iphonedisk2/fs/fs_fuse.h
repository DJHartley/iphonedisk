// Author: Allen Porter <allen@thebends.org>
//
// The fuse filesystem implementation.  This filesystem forwards all calls to
// an FsService.

#ifndef __FS_FS_FUSE_H__
#define __FS_FS_FUSE_H__

#include <string>
#include <fuse/fuse.h>

struct fuse_operations;
namespace proto {
class FsService;
}

namespace fs {

// Context information about filesystem available to every filesystem call.
//
// The caller is responsible for initializing a Context pointer that is
// specified in the private userdata call to fuse_new.  Every filesystem call
// will use the Context to obtain the FsService object that actually processes
// the filesystem command.
//
// Note that the fuse filesystem can only be initialized in multithreaded mode
// if the FsService is also multi-threaded -- however most implementations in
// this project are not currently multi-threaded.
struct Context {
  Context() : service(NULL) { }

  proto::FsService* service;
  std::string fs_id;
};

// Initialize the fuse_op datastructure for use with an FsService.
void InitFuseOps(struct fuse_operations* fuse_op);

// Initialize the FuseArgs datastructure for mounting the specified volume
void InitFuseArgs(struct fuse_args* args, const std::string& volname,
                  const std::string& volicon);

}  // namespace fs

#endif  // __FS_FS_FUSE_H__
