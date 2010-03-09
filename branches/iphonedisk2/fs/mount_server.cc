// Author: Allen Porter <allen@thebends.org>

#include <iostream>
#include <fuse.h>
#include <google/protobuf/service.h>
#include <sys/stat.h>
#include "fs/fs.h"
#include "proto/fs_service.pb.h"
#include "rpc/mach_channel.h"

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
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <mach service name> <volume name>\n", argv[0]);
    return 1;
  }
  const std::string service_name(argv[1]);
  const std::string volume(argv[22]);
  google::protobuf::RpcChannel* channel = rpc::NewMachChannel(service_name);
  if (channel == NULL) {
    std::cerr << "Unable to create channel for service: " << service_name
              << std::endl; 
    return 1;
  }
  proto::FsService* service = new proto::FsService::Stub(channel);
  struct fuse_operations fuse_ops;
  fs::Initialize(service, &fuse_ops);

  struct fuse_args args;  
  InitFuseArgs(&args, volume);

  std::string mount_path = "/Volumes/";
  mount_path.append(volume);
  // Ignore errors
  rmdir(mount_path.c_str());
  mkdir(mount_path.c_str(), S_IFDIR|0755);

  int err = 0;
  struct fuse_chan* chan = fuse_mount(mount_path.c_str(), &args);
  if (chan == NULL) {
    std::cerr << "fuse_mount() failed" << std::endl;
    err = 1;
  } else {
    struct fuse* f = fuse_new(chan, &args, &fuse_ops, sizeof(fuse_ops), NULL);
    if (f == NULL) {
      std::cerr << "fuse_new() failed" << std::endl;
      err = 1;
    } else {
      fuse_loop(f);
      std::cout << "Fuse loop existed.";
      fuse_destroy(f);
    }
    fuse_unmount(mount_path.c_str(), chan);
  }
  delete channel;
  delete service;
  return err;
}
