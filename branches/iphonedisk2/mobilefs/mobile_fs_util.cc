// Author: Allen Porter <allen@thebends.org>
//
// Mounts the mobilefs service.

#include <iostream>
#include "fs/fs.h"
#include "fs/fs_proxy.h"
#include "mobilefs/mobiledevice.h"
#include "mobilefs/mobile_fs_service.h"
#include "proto/fs_service.pb.h"
#include "test/loopback_fs_service.h"

struct MountArgs {
  std::string volume;
  std::string afc_service_name;
};

static bool InitializeDevice(am_device* device) {
  int ret = AMDeviceConnect(device);
  if (ret != MDERR_OK) {
    std::cerr << "AMDeviceConnect failed" << std::endl;
    return false;
  }
  if (AMDeviceIsPaired(device) == 0) {
    std::cerr << "Device was not paired" << std::endl;
    return false;
  }
  ret = AMDeviceValidatePairing(device);
  if (ret != MDERR_OK) {
    std::cerr << "AMDeviceValidatePairing failed" << std::endl;
    return false;
  }
  ret = AMDeviceStartSession(device);
  if (ret != MDERR_OK) {
    std::cerr << "AMDeviceStartSession failed" << std::endl;
    return false;
  }
  return true;
}

static bool StartSession(am_device* device, struct MountArgs* args) {
  CFStringRef service_name =
      CFStringCreateWithCString(NULL, args->afc_service_name.c_str(),
                                args->afc_service_name.size());
  int socket;
  int ret = AMDeviceStartService(device, service_name, &socket);
  if (ret != MDERR_OK) {
    std::cerr << "AMDeviceStartService failed" << std::endl;
    CFRelease(service_name);
    return false;
  }
  afc_connection* conn;
  ret = AFCConnectionOpen(socket, 0, &conn);
  if (ret != MDERR_OK) {
    std::cerr << "AFCConnectionOpen failed" << std::endl;
    CFRelease(service_name);
    return false;
  }
  proto::FsService* service = mobilefs::NewMobileFsService(conn);
  fs::Filesystem* fs = fs::NewProxyFilesystem(service, "mobile-fs",
                                              args->volume);
  if (!fs) {
    std::cerr << "fs::MountFilesystem failed" << std::endl;
  } else {
    // TODO(allen): Make this non-blocking, otherwise we do not get notified
    // when the device has disconnected.
    fs->WaitForUnmount();
    delete fs;
  }
  CFRelease(service_name);
  delete service;
  return 0;
}

static void notify_callback(am_device_notification_callback_info *info,
                            void* arg) {
  std::cout << "notify_callback invoked: msg=" << info->msg << std::endl;
  struct MountArgs* mount_args = static_cast<struct MountArgs*>(arg);
  bool exit = false;
  if (info->msg == ADNCI_MSG_DISCONNECTED) {
    std::cout << "Device disconnected" << std::endl;
    exit = true;
  } else if (info->msg == ADNCI_MSG_CONNECTED) {
    struct am_device* device = info->dev;
    if (!InitializeDevice(device)) {
      exit = true;
    } else {
      // This blocks and mounts the filesystem.  Is that a problem?
      if (!StartSession(device, mount_args)) {
        std::cerr << "Failed to start session" << std::endl;
      }
      exit = true;
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
  struct MountArgs args;
  args.volume = argv[1];
  args.afc_service_name = argv[2];
  am_device_notification* notification;
  int ret = AMDeviceNotificationSubscribe(&notify_callback, 0, 0, &args,
                                          &notification);
  if (ret != MDERR_OK) {
    std::cerr << "AMDeviceNotificationSubscribe failed" << std::endl;
    return 1;
  }
  std::cout << "Waiting for device connection" << std::endl;
  CFRunLoopRun();
  std::cout << "Run loop exited." << std::endl;
  AMDeviceNotificationUnsubscribe(notification);
}

