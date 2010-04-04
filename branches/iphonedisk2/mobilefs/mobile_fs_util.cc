// Author: Allen Porter <allen@thebends.org>
//
// Mounts the mobilefs service.

#include <iostream>
#include "fs/fs.h"
#include "fs/fs_proxy.h"
#include "mobilefs/afc_listener.h"
#include "mobilefs/mobile_fs_service.h"
#include "mount/mount_service.h"
#include "proto/fs_service.pb.h"
#include "proto/mount_service.pb.h"
#include "rpc/rpc.h"
#include "test/loopback_fs_service.h"

using namespace google::protobuf;

struct MountArgs {
  std::string volume;
};

static proto::MountService* mounter = NULL;

static void sig_handler(int signal) {
  delete mounter;
  mounter = NULL;
  CFRunLoopStop(CFRunLoopGetCurrent());
}

static void notify_callback(mobilefs::NotifyStatus* status,
                            void* arg) {
  struct MountArgs* mount_args = static_cast<struct MountArgs*>(arg);
  if (status->connection == NULL) {
    std::cout << "Device disconnected" << std::endl;
    delete mounter;
    mounter = NULL;
  } else {
    if (mounter != NULL) {
      std::cout << "Device already mounted? Waiting for disconnect";
      return;
    }
    mounter = mount::NewMountService(
        mobilefs::NewMobileFsService(status->connection));
    rpc::Rpc rpc;
    proto::MountRequest request;
    request.set_fs_id("mobile-fs");
    request.set_volume(mount_args->volume);
    proto::MountResponse response;
    mounter->Mount(&rpc, &request, &response, NewCallback(DoNothing));
    if (rpc.Failed()) {
      std::cerr << "fs::MountFilesystem failed" << std::endl;
      delete mounter;
      mounter = NULL;
      CFRunLoopStop(CFRunLoopGetCurrent());
    } else {
      std::cerr << "Device mounted as: " << mount_args->volume << std::endl;
    }
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

