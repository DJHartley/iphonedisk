// Author: Allen Porter <allen@thebends.org>
//
// Mounts the loopback fs service.

#include <syslog.h>
#include "fs/fs.h"
#include "fs/fs_proxy.h"
#include "proto/fs_service.pb.h"
#include "test/loopback_fs_service.h"

int main(int argc, char* argv[]) {
  openlog("loopback_fs_util", LOG_PERROR, LOG_USER);
#ifdef DEBUG
  setlogmask(LOG_UPTO(LOG_DEBUG));
#else
  setlogmask(LOG_UPTO(LOG_INFO));
#endif
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <volume> <volicon>\n", argv[0]);
    return 1;
  }
  const std::string& volume(argv[1]);
  const std::string& volicon(argv[2]);
  proto::FsService* service = test::NewLoopbackService();
  fs::Filesystem* fs = fs::NewProxyFilesystem(service, "dummy-fs-id", volume,
                                              volicon);
  if (!fs->Mount()) {
    syslog(LOG_ERR, "Failed to mount filesystem");
  } else {
    fs->WaitForUnmount();
  }
  delete fs;
  delete service;
  closelog();
  return 0;
}
