// Author: Allen Porter <allen@thebends.org>

#include "fs/mount_service.h"

#include <iostream>
#include <string>
#include <errno.h>
#include <fuse.h>
#include <google/protobuf/service.h>
#include <sys/stat.h>
#include "rpc/mach_channel.h"
#include "proto/fs_service.pb.h"
#include "proto/mount_service.pb.h"
#include "fs/fs.h"

namespace {

static proto::FsService* NewFsService(const std::string& fs_service_name) {
  google::protobuf::RpcChannel* channel = rpc::NewMachChannel(fs_service_name);
  if (channel == NULL) {
    return NULL;
  }
  return new proto::FsService::Stub(channel);
}

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

static void MountFilesystem(const std::string& fs_service_name,
                            const std::string& fs_id,
                            const std::string& volume) {
  proto::FsService* service = NewFsService(fs_service_name);
  if (service == NULL) {
    std::cerr << fs_id << ": FsService creation failed" << std::endl;
    return;
  }

  struct fuse_operations fuse_ops;
  fs::Initialize(service, fs_id, &fuse_ops);

  struct fuse_args args;  
  InitFuseArgs(&args, volume);

  std::string mount_path = "/Volumes/";
  mount_path.append(volume);
  // Ignore errors
  rmdir(mount_path.c_str());
  mkdir(mount_path.c_str(), S_IFDIR|0755);

  struct fuse_chan* chan = fuse_mount(mount_path.c_str(), &args);
  if (chan == NULL) {
    std::cerr << fs_id << ": fuse_mount() failed" << std::endl;
    delete service;
    return;
  }

  struct fuse* f = fuse_new(chan, &args, &fuse_ops, sizeof(fuse_ops), NULL);
  if (f == NULL) {
    std::cerr << fs_id << ": fuse_new() failed" << std::endl;
    // TODO(aporter): Fix?
    //fuse_unmount(mount_path.c_str(), chan);
    delete service;
    return;
  }
  std::cout << fs_id << ": Fuse loop started.";
  fuse_loop(f);
  std::cout << fs_id << ": Fuse loop exited.";
  // TODO(aporter): Is it even possible to make this loop exist?
  fuse_destroy(f);
  fuse_unmount(mount_path.c_str(), chan);
  delete service;
}

class Mounter : public proto::MountService {
 public:
  Mounter(const std::string& fs_service_name)
      : fs_service_name_(fs_service_name) { }
  virtual ~Mounter() { }

  void Mount(google::protobuf::RpcController* rpc,
             const proto::MountRequest* request,
             proto::MountResponse* response,
             google::protobuf::Closure* done) {
    std::string fs_id = request->fs_id();
    std::string volume = request->volume();
    pid_t p = fork();
    if (p == -1) {
      perror("fork");
      exit(errno);
    } else if (p == 0) {
      // child process
      MountFilesystem(fs_service_name_, fs_id, volume);
      return;
    } else {
      // parent process
      // TODO(aporter): Keep track of the child process id and perhaps report
      // status about it via another API
      done->Run();
    }
  }

 private:
  std::string fs_service_name_;
};

}  // namespace

namespace fs {

proto::MountService* NewMountService(const std::string& fs_service_name) {
  return new Mounter(fs_service_name);
}

}  // namespace fs
