// Author: Allen Porter <allen@thebends.org>

#include <iostream>
#include <google/protobuf/service.h>
#include <sys/stat.h>
#include "mount/mount_service.h"
#include "proto/fs_service.pb.h"
#include "proto/mount_service.pb.h"
#include "rpc/rpc.h"
#include "rpc/mach_channel.h"
#include "rpc/mach_service.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <mount service name> <fs service name>\n",
            argv[0]);
    return 1;
  }
  const std::string mount_service_name(argv[1]);
  const std::string fs_service_name(argv[2]);
  google::protobuf::RpcChannel* channel = rpc::NewMachChannel(fs_service_name);
  if (channel == NULL) {
    std::cerr << "Failed to create service: " << fs_service_name << std::endl;
    return 1;
  }
  proto::FsService* fs_service = new proto::FsService::Stub(channel);
  proto::MountService* service = mount::NewMountService(fs_service);
  if (!rpc::ExportService(mount_service_name, service)) {
    std::cerr << "Failed to export " << mount_service_name << std::endl;
    delete service;
    return 1;
  }
  delete service;
  return 0;
}
