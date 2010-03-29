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

static fs::Filesystem* filesystem = NULL;
static proto::FsService* service = NULL;

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
  CFRelease(service_name);
  afc_connection* conn;
  ret = AFCConnectionOpen(socket, 0, &conn);
  if (ret != MDERR_OK) {
    std::cerr << "AFCConnectionOpen failed" << std::endl;
    return false;
  }
  service = mobilefs::NewMobileFsService(conn);
  filesystem = fs::NewProxyFilesystem(service, "mobile-fs", args->volume);
  if (!filesystem->Mount()) {
    std::cerr << "fs::MountFilesystem failed" << std::endl;
    delete filesystem;
    filesystem = NULL;
    delete service;
    service = NULL;
    return false;
  }
  std::cout << "Mounted volume as: " << args->volume << std::endl;
  return true;
}

static void notify_callback(am_device_notification_callback_info *info,
                            void* arg) {
  std::cout << "notify_callback invoked: msg=" << info->msg << std::endl;
  struct MountArgs* mount_args = static_cast<struct MountArgs*>(arg);
  bool exit = false;
  if (info->msg == ADNCI_MSG_DISCONNECTED) {
    std::cout << "Device disconnected" << std::endl;
    // TODO(allen): Should this wait for a re-connect?
    exit = true;
    if (filesystem != NULL) {
      // Unmount a previously mounted device
      filesystem->Unmount();
      delete filesystem;
      filesystem = NULL;
      delete service;
      service = NULL;
    }
  } else if (info->msg == ADNCI_MSG_CONNECTED) {
    if (filesystem != NULL) {
      std::cout << "Device already mounted? Waiting for disconnect";
      return;
    }
    struct am_device* device = info->dev;
    if (!InitializeDevice(device)) {
      exit = true;
    } else {
      // This blocks and mounts the filesystem.  Is that a problem?
      if (!StartSession(device, mount_args)) {
        std::cerr << "Failed to start session" << std::endl;
      }
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

