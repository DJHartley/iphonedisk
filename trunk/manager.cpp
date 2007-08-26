// manager.cpp
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>

#include "ythread/callback-inl.h"

#include "manager.h"
#include <CoreFoundation/CoreFoundation.h>
#include <iostream>
#include "MobileDevice.h"
#include "ythread/thread.h"
#include "ythread/mutex.h"
#include "ythread/condvar.h"

using namespace std;

namespace iphonedisk {

static void notify_callback(am_device_notification_callback_info *info,
                            void* foo);


class ManagerImpl : public ythread::Thread, public Manager {
 public:
  ManagerImpl(ythread::Callback* connect_cb, ythread::Callback* disconnect_cb)
    : condvar_(&mutex_),
      done_(false),
      device_(NULL),
      connect_cb_(connect_cb),
      disconnect_cb_(disconnect_cb),
      loop_(NULL) { }

  virtual ~ManagerImpl() {
    // Wait until the thread is started before trying to destroy, otherwise
    // theres a race condition here
    ythread::MutexLock l(&mutex_);
    while (loop_ == NULL) {
      CFRunLoopStop(loop_);
    }
    Join();
    delete connect_cb_;
    delete disconnect_cb_;
  }

  virtual bool WaitUntilConnected() {
    ythread::MutexLock l(&mutex_);
    while (!done_ && device_ == NULL) {
      condvar_.Wait();
    }
    return !done_;
  }

  afc_connection* Open(const string& service) {
    ythread::MutexLock l(&mutex_);
    if (device_ == NULL) {
      return NULL;
    }
    afc_connection *hAFC;
    CFStringRef serv = CFStringCreateWithCString(NULL, service.c_str(),
                                                 service.size());
    int ret = AMDeviceStartService(device_, serv, &hAFC, NULL);
    if (ret != 0) {
      cerr << "AMDeviceStartService: " << ret << endl;
      return NULL;
    }
    if (hAFC == NULL) {  // sanity check
      cerr << "AMDeviceStartService did not initialize connection!";
      return NULL;
    }
    ret = AFCConnectionOpen(hAFC, 0, &hAFC);
    if (ret != 0) {
      cerr << "AFCConnectionOpen: " << ret << endl;
      return NULL;
    }
    return hAFC;
  }

  // Invoked by the static helper method notify_callback
  void Notify(am_device_notification_callback_info *info) {
    // Make a copy of the callback so that it is run without the mutex held
    ythread::Callback* cb = NULL;
    mutex_.Lock(); 
    if (info->msg == ADNCI_MSG_CONNECTED && device_ == NULL) {
      device_ = info->dev;
      if (!InitializeDevice(device_)) {
        device_ = NULL;
        CFRunLoopStop(CFRunLoopGetCurrent());
      } else if (connect_cb_ != NULL) {
        cb = connect_cb_;
      }
      condvar_.SignalAll();  // Wake WaitUntilConnected
    } else if(info->msg == ADNCI_MSG_DISCONNECTED) {
      device_ = NULL;
      if (disconnect_cb_ != NULL) {
        cb = disconnect_cb_;
      }
    }
    mutex_.Unlock();
    if (cb != NULL) {
      cb->Execute();
    }
  } 

 protected:
  virtual void Run() {
    mutex_.Lock();
    loop_ = CFRunLoopGetCurrent();
    am_device_notification *notif;
    int ret = AMDeviceNotificationSubscribe(notify_callback, 0, 0, this,
                                            &notif);
    if (ret != 0) {
      cerr << "AMDeviceNotificationSubscribe: " << ret << endl;
      return;
    }
    condvar_.SignalAll();
    mutex_.Unlock();  // Wake constructor
    CFRunLoopRun();
    done_ = true;
  } 

  static bool InitializeDevice(am_device* device) {
    int ret = AMDeviceConnect(device);
    if (ret != 0) {
      cerr << "AMDeviceConnect: " << ret << endl;
      return false;
    }
    if (!AMDeviceIsPaired(device)) {
      cerr << "Device was not paired" << endl;
      return false;
    }
    ret = AMDeviceValidatePairing(device);
    if (ret != 0) {
      cerr << "AMDeviceValidatePairing: " << ret << endl;
      return false;
    }
    ret = AMDeviceStartSession(device);
    if (ret != 0) {
      cerr << "AMDeviceStartSession: " << ret << endl;
      return false;
    }
    return true;
  }

 private:
  ythread::Mutex mutex_;
  ythread::CondVar condvar_;
  bool done_;
  am_device* device_;
  ythread::Callback* connect_cb_;
  ythread::Callback* disconnect_cb_;
  CFRunLoopRef loop_;
};

static void notify_callback(am_device_notification_callback_info *info,
                            void* arg) {
  ManagerImpl* manager = static_cast<ManagerImpl*>(arg);
  manager->Notify(info);
}

Manager* NewManager(ythread::Callback* connect_cb,
                    ythread::Callback* disconnect_cb) {
  ManagerImpl* manager = new ManagerImpl(connect_cb, disconnect_cb);
  manager->Start();
  return manager;
}

};
