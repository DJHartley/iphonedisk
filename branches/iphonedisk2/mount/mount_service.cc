// Author: Allen Porter <allen@thebends.org>

#include "mount/mount_service.h"

#include <string>
#include <errno.h>
#include <google/protobuf/service.h>
#include <sys/stat.h>
#include <syslog.h>
#include "rpc/rpc.h"
#include "proto/fs_service.pb.h"
#include "proto/mount_service.pb.h"
#include "fs/fs.h"
#include "fs/fs_proxy.h"

namespace {

class Mounter : public proto::MountService {
 public:
  Mounter(proto::FsService* service)
      : service_(service), 
        proxy_fs_(NULL) { }

  virtual ~Mounter() {
    if (proxy_fs_ != NULL) {
      // A filesystem was mounted!
      syslog(LOG_INFO, "Abandoning proxy filesystem");
      proxy_fs_->Unmount();
      delete proxy_fs_;
    }
    delete service_;
  }

   virtual void Mount(google::protobuf::RpcController* rpc,
                      const proto::MountRequest* request,
                      proto::MountResponse* response,
                      google::protobuf::Closure* done) {
    std::string fs_id = request->fs_id();
    std::string volume = request->volume();
    if (proxy_fs_ != NULL) {
      rpc->SetFailed("Filesystem already mounted");
    } else {
      proxy_fs_ = fs::NewProxyFilesystem(service_, fs_id, volume);
      if (!proxy_fs_->Mount()) {
        rpc->SetFailed("Failed to mount proxy filesystem");
        proxy_fs_ = NULL;
        delete proxy_fs_;
      } else {
        syslog(LOG_INFO, "Mounted %s as %s", fs_id.c_str(), volume.c_str());
      }
    }
    done->Run();
  }

  virtual void Unmount(google::protobuf::RpcController* rpc,
                       const proto::UnmountRequest* request,
                       proto::UnmountResponse* response,
                       google::protobuf::Closure* done) {
    if (proxy_fs_ == NULL) {
      rpc->SetFailed("Filesystem not mounted");
    } else {
      proxy_fs_->Unmount();
      syslog(LOG_INFO, "Unmounted filesystem");
      delete proxy_fs_;
      proxy_fs_ = NULL;
    }
    done->Run();
  }

 private:
  proto::FsService* service_;
  // TODO(allen): Can this be a map of filesystems? I think the fuse code needs
  // to be static-free before this is possible.
  fs::Filesystem* proxy_fs_;
};

}  // namespace

namespace mount {

proto::MountService* NewMountService(proto::FsService* fs_service) {
  return new Mounter(fs_service);
}

}  // namespace fs
