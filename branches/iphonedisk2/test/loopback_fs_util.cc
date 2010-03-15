// Author: Allen Porter <allen@thebends.org>
//
// Mounts the loopback fs service.

#include <iostream>
#include "fs/fs.h"
#include "proto/fs_service.pb.h"
#include "test/loopback_fs_service.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <volume>\n", argv[0]);
    return 1;
  }
  const std::string& volume(argv[1]);
  proto::FsService* service = test::NewLoopbackService();
  bool res = fs::MountFilesystem(service, "dummy-fs-id", volume);
  if (!res) {
    std::cerr << "fs::MountFilesystem failed";
  }
  delete service;
  return res ? 0 : 1;
}
