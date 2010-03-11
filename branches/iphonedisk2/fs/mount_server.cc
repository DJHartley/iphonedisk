// Author: Allen Porter <allen@thebends.org>

#include <iostream>
#include <fuse.h>
#include <google/protobuf/service.h>
#include <sys/stat.h>
#include "fs/mount_service.h"
#include "proto/mount_service.pb.h"
#include "rpc/mach_service.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <mount service name> <fs service name>\n",
            argv[0]);
    return 1;
  }
  const std::string mount_service_name(argv[1]);
  const std::string fs_service_name(argv[1]);
  proto::MountService* service = fs::NewMountService(fs_service_name);
  if (!rpc::ExportService(mount_service_name, service)) {
    std::cerr << "Failed to export " << mount_service_name << std::endl;
    delete service;
    return 1;
  }
  delete service;
  return 0;
}
