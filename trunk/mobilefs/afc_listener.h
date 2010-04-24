// Author: Allen Porter <allen@thebends.org>
//
// A wrapper around the MobileDevice library calls for listening for device
// connect and disconnect events.  The AfcListener expects the caller to
// to start and stop a CFRunLoop.

#ifndef __MOBILEFS_AFC_LISTENER_H__
#define __MOBILEFS_AFC_LISTENER_H__

#include <string>
#include "mobilefs/mobiledevice.h"

namespace mobilefs {

// Information about the AFC connection, passed to the NotifyCallback.  The
// connection is non-NULL when the device is connected and NULL otherwise.
struct NotifyStatus {
  afc_connection* connection;
};

typedef void (*NotifyCallback)(NotifyStatus* status, void* user_data);

class AfcListener {
 public:
  // The name of the AFC service on the device.  Typically this is
  // "com.apple.afc", but might be different if the device is jailbroken running
  // a custom or additional AFC service.
  AfcListener(const std::string& afc_service_name);
  ~AfcListener();

  // Registers a device listener
  bool SetNotifyCallback(NotifyCallback callback, void* user_data);

  // Used internall
  void DeviceCallback(am_device_notification_callback_info* info);

 private:
  static bool InitializeDevice(am_device* device);
  bool OpenConnection(am_device* device);

  CFStringRef afc_service_name_;
  am_device_notification* notification_;
  afc_connection* connection_;
  NotifyCallback user_callback_;
  void* user_data_;
};

}  // namespace mobilefs

#endif
