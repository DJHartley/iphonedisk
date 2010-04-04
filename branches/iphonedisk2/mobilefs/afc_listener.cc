// Author: Allen Porter <allen@thebends.org>
//
// TODO(aporter): AMDeviceStopSession/AMDeviceDisconnect as well? I have
// a feeling this might leak connections.

#include "mobilefs/afc_listener.h"

#include <iostream>

namespace mobilefs {

static void notify_callback(am_device_notification_callback_info *info,
                            void* arg) {
  AfcListener* listener = static_cast<AfcListener*>(arg);
  listener->DeviceCallback(info);
}

AfcListener::AfcListener(const std::string& afc_service_name)
    : notification_(NULL),
      connection_(NULL),
      user_callback_(NULL),
      user_data_(NULL) {
  afc_service_name_ = CFStringCreateWithCString(NULL, afc_service_name.c_str(),
                                                afc_service_name.size());
}

AfcListener::~AfcListener() {
  if (notification_ != NULL) {
    AMDeviceNotificationUnsubscribe(notification_);
    if (connection_ != NULL) {
      AFCConnectionClose(connection_);
    }
  }
  CFRelease(afc_service_name_);
}

bool AfcListener::SetNotifyCallback(NotifyCallback callback, void* user_data) {
  int ret = AMDeviceNotificationSubscribe(&notify_callback, 0, 0, this,
                                          &notification_);
  if (ret != MDERR_OK) {
    std::cerr << "AMDeviceNotificationSubscribe failed" << std::endl;
    return false;
  }
  user_callback_ = callback;
  user_data_ = user_data;
  return true;
}

void AfcListener::DeviceCallback(am_device_notification_callback_info* info) {
  if (info->msg != ADNCI_MSG_DISCONNECTED &&
      info->msg != ADNCI_MSG_CONNECTED) {
    // Ignore any non-connection events
    return;
  }
  if (info->msg == ADNCI_MSG_CONNECTED) {
    struct am_device* device = info->dev;
    if (!InitializeDevice(device) || !OpenConnection(device)) {
      connection_ = NULL;
    }
  } else {
    if (connection_ != NULL) {
      int ret = AFCConnectionClose(connection_);
      if (ret != MDERR_OK) {
        std::cerr << "AFCConnectionClose failed" << std::endl;
      }
      connection_ = NULL;
    }
  }
  struct NotifyStatus status;
  status.connection = connection_;
  (*user_callback_)(&status, user_data_);
}

bool AfcListener::InitializeDevice(am_device* device) {
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

bool AfcListener::OpenConnection(am_device* device) {
  int socket;
  int ret = AMDeviceStartService(device, afc_service_name_, &socket);
  if (ret != MDERR_OK) {
    std::cerr << "AMDeviceStartService failed" << std::endl;
    return false;
  }
  ret = AFCConnectionOpen(socket, 0, &connection_);
  if (ret != MDERR_OK) {
    std::cerr << "AFCConnectionOpen failed" << std::endl;
    return false;
  }
  std::cout << "AFCConnection established" << std::endl;
  return true;
}


}  // namespace mobilefs
