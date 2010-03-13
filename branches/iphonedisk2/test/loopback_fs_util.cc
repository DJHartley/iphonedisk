// Author: Allen Porter <allen@thebends.org>

#include <iostream>
#include <fuse.h>
#include "fs/fs.h"
#include "proto/fs_service.pb.h"
#include "test/loopback_fs_service.h"

static void InitFuseArgs(struct fuse_args* args, const std::string& volname) {
  *args = (struct fuse_args)FUSE_ARGS_INIT(0, NULL);
  fuse_opt_add_arg(args, "-d");
#ifdef DEBUG
  fuse_opt_add_arg(args, "-odebug");
#endif
  fuse_opt_add_arg(args, "-odefer_permissions");
  std::string volname_arg("-ovolname=");
  volname_arg.append(volname);
  fuse_opt_add_arg(args, volname_arg.c_str());
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <volume>\n", argv[0]);
    return 1;
  }
  const std::string& volume(argv[1]);
  proto::FsService* service = test::NewLoopbackService();

  struct fuse_operations fuse_ops;
  fs::Initialize(service, "dummy-fs-id", &fuse_ops);

  struct fuse_args args;  
  InitFuseArgs(&args, volume);

  std::string mount_path = "/Volumes/";
  mount_path.append(volume);
  // Ignore errors
  rmdir(mount_path.c_str());
  mkdir(mount_path.c_str(), S_IFDIR|0755);

  struct fuse_chan* chan = fuse_mount(mount_path.c_str(), &args);
  if (chan == NULL) {
    std::cerr << "fuse_mount() failed" << std::endl;
    delete service;
    return 1;
  }

  struct fuse* f = fuse_new(chan, &args, &fuse_ops, sizeof(fuse_ops), NULL);
  if (f == NULL) {
    std::cerr << "fuse_new() failed" << std::endl;
    delete service;
    return 1;
  }
  std::cout << "Fuse loop started.";
  fuse_loop(f);
  std::cout << "Fuse loop exited.";
  // TODO(aporter): Is it even possible to make this loop exist?
  fuse_destroy(f);
  delete service;
  return 0;
}
