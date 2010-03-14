// Author: Allen Porter <allen@thebends.org>

#include "mount/mount_service.h"

#include <iostream>
#include <string>
#include <errno.h>
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
      proto::FsService* service = NewFsService(fs_service_name_);
      if (service == NULL) {
        std::cerr << fs_id << ": FsService creation failed" << std::endl;
        return;
      }
      fs::MountFilesystem(service, fs_id, volume);
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
