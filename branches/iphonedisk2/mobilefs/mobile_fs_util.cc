// Author: Allen Porter <allen@thebends.org>
//
// Mounts the mobilefs service.

#include <iostream>
#include "fs/fs.h"
#include "fs/fs_proxy.h"
#include "mobilefs/afc_listener.h"
#include "mobilefs/mobile_fs_service.h"
#include "proto/fs_service.pb.h"
#include "test/loopback_fs_service.h"

struct MountArgs {
  std::string volume;
};

static fs::Filesystem* filesystem = NULL;
static proto::FsService* service = NULL;

static void Shutdown() {
  filesystem->Unmount();
  delete filesystem;
  filesystem = NULL;
  delete service;
  service = NULL;
}

static void sig_handler(int signal) {
  if (filesystem != NULL) {
    Shutdown();
  }
  CFRunLoopStop(CFRunLoopGetCurrent());
}

static void notify_callback(mobilefs::NotifyStatus* status,
                            void* arg) {
  struct MountArgs* mount_args = static_cast<struct MountArgs*>(arg);
  bool exit = false;
  if (status->connection == NULL) {
    std::cout << "Device disconnected" << std::endl;
    if (filesystem != NULL) {
      // Unmount a previously mounted device
      Shutdown();
    }
  } else {
    if (filesystem != NULL) {
      std::cout << "Device already mounted? Waiting for disconnect";
      return;
    }
    service = mobilefs::NewMobileFsService(status->connection);
    filesystem = fs::NewProxyFilesystem(service, "mobile-fs",
                                        mount_args->volume);
    if (!filesystem->Mount()) {
      std::cerr << "fs::MountFilesystem failed" << std::endl;
      Shutdown();
      exit = true;
    } else {
      std::cerr << "Device mounted as: " << mount_args->volume << std::endl;
    }
  }
  if (exit) {
    CFRunLoopStop(CFRunLoopGetCurrent());
  }
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <volume> <afc service>\n", argv[0]);
    return 1;
  }
  signal(SIGINT, sig_handler);

  struct MountArgs args;
  args.volume = argv[1];
  mobilefs::AfcListener listener(argv[2]);
  if (!listener.SetNotifyCallback(&notify_callback, &args)) {
    std::cerr << "Failed to initialize listener" << std::endl;
    return 1;
  }
  std::cout << "Waiting for device connection" << std::endl;
  CFRunLoopRun();
  std::cout << "Run loop exited." << std::endl;
}

