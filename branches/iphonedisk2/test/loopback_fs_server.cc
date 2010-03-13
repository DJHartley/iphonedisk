// Author: Allen Porter <allen@thebends.org>

#include <iostream>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/attr.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "proto/fs_service.pb.h"
#include "rpc/mach_service.h"
#include "test/loopback_fs_service.h"

using ::rpc::ExportService;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <fs service name>\n", argv[0]);
    return 1;
  }
  const std::string& service_name(argv[1]);
  proto::FsService* service = test::NewLoopbackService();
  if (!ExportService(service_name, service)) {
    std::cerr << "Failed to export " << service_name << std::endl;
    delete service;
    return 1;
  }
  delete service;


  return 0;
}
