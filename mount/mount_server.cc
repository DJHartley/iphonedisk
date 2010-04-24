// Author: Allen Porter <allen@thebends.org>

#include <google/protobuf/service.h>
#include <sys/stat.h>
#include <syslog.h>
#include "mount/mount_service.h"
#include "proto/fs_service.pb.h"
#include "proto/mount_service.pb.h"
#include "rpc/rpc.h"
#include "rpc/mach_channel.h"
#include "rpc/mach_service.h"

int main(int argc, char* argv[]) {
  openlog("mount_server", LOG_PERROR | LOG_PID, LOG_DAEMON);
  if (argc != 4) {
    syslog(LOG_ERR, "Usage: %s <mount service name> <fs service name> "
           "<volicon>", argv[0]);
    return 1;
  }
  const std::string mount_service_name(argv[1]);
  const std::string fs_service_name(argv[2]);
  google::protobuf::RpcChannel* channel = rpc::NewMachChannel(fs_service_name);
  if (channel == NULL) {
    syslog(LOG_ERR, "Failed to create service: %s", fs_service_name.c_str());
    return 1;
  }
  const std::string volicon(argv[3]);
  proto::FsService* fs_service = new proto::FsService::Stub(channel);
  proto::MountService* service = mount::NewMountService(fs_service, volicon);
  if (!rpc::ExportService(mount_service_name, service)) {
    syslog(LOG_ERR, "Failed to export: %s", mount_service_name.c_str());
    delete service;
    return 1;
  }
  delete service;
  return 0;
}
