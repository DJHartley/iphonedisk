// Author: Allen Porter <allen@thebends.org>
//
// Mounts the loopback fs service.

#include <iostream>
#include "fs/fs.h"
#include "fs/fs_proxy.h"
#include "proto/fs_service.pb.h"
#include "test/loopback_fs_service.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <volume>\n", argv[0]);
    return 1;
  }
  const std::string& volume(argv[1]);
  proto::FsService* service = test::NewLoopbackService();
  fs::Filesystem* fs = fs::NewProxyFilesystem(service, "dummy-fs-id", volume);
  if (!fs->Mount()) {
    std::cerr << "fs::MountFilesystem failed";
  } else {
    fs->WaitForUnmount();
  }
  delete fs;
  delete service;
  return 0;
}
