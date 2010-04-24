// Author: Allen Porter <allen@thebends.org>
//
// Mounts the mobilefs service.

#include <string>
#include <syslog.h>
#include "mobilefs/afc_listener.h"
#include "mobilefs/mobile_fs_service.h"
#include "mount/mount_service.h"
#include "proto/mount_service.pb.h"
#include "rpc/rpc.h"
#include "test/loopback_fs_service.h"

using namespace google::protobuf;

struct MountArgs {
  std::string volume;
  std::string volicon;
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
    syslog(LOG_INFO, "Device disconnected");
    delete mounter;
    mounter = NULL;
  } else {
    if (mounter != NULL) {
      syslog(LOG_DEBUG, "Device already mounted");
      return;
    }
    syslog(LOG_INFO, "Device connected");
    mounter = mount::NewMountService(
        mobilefs::NewMobileFsService(status->connection),
        mount_args->volicon);
    rpc::Rpc rpc;
    proto::MountRequest request;
    request.set_fs_id("mobile-fs");
    request.set_volume(mount_args->volume);
    proto::MountResponse response;
    mounter->Mount(&rpc, &request, &response, NewCallback(DoNothing));
    if (rpc.Failed()) {
      syslog(LOG_ERR, "Failed to mount filesystem");
      delete mounter;
      mounter = NULL;
      CFRunLoopStop(CFRunLoopGetCurrent());
    }
  }
}

int main(int argc, char* argv[]) {
  openlog("mobile_fs_util", LOG_PERROR, LOG_USER);
#ifdef DEBUG
  setlogmask(LOG_UPTO(LOG_DEBUG));
#else
  setlogmask(LOG_UPTO(LOG_INFO));
#endif
  if (argc != 4) {
    syslog(LOG_ERR, "Usage: %s <volume> <volicon> <afc service>", argv[0]);
    return 1;
  }
  signal(SIGINT, sig_handler);

  struct MountArgs args;
  args.volume = argv[1];
  args.volicon = argv[2];
  mobilefs::AfcListener listener(argv[3]);
  if (!listener.SetNotifyCallback(&notify_callback, &args)) {
    syslog(LOG_ERR, "Failed to initialize device listener");
    closelog();
    return 1;
  }
  syslog(LOG_INFO, "Waiting for device connection");
  CFRunLoopRun();
  syslog(LOG_INFO, "Exiting");
  closelog();
  return 0;
}

