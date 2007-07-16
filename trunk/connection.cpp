// connection.h
// Author: Allen Porter <allen@thebends.org> (pin)
// Much code in this module was based on the iPhoneInterface tool by:
//   geohot, ixtli, nightwatch, warren, nail, Operator
// See http://iphone.fiveforty.net/wiki/
#include "connection.h"
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stddef.h>
#include <sys/types.h>
#include <dirent.h>
#include <ythread/condvar.h>
#include <ythread/mutex.h>
#include <ythread/thread.h>
#include "MobileDevice.h"  // from iPhoneInterface

#if !defined(WIN32)
#define __cdecl
#endif
typedef int (__cdecl * cmdsend)(am_recovery_device *,void *);

using namespace std;

namespace iphonedisk {

static ythread::Mutex mutex_;
static ythread::CondVar* condvar_;
static bool connected_;
static am_device *iPhone_;
static cmdsend wsendCommandToDevice_;
static afc_connection *hAFC_;

// This class is kind of a hack, in that it assumes that there is only ever
// one ConnectionImpl, created by the factory below.
class ConnectionImpl : public Connection {
 public:
  ConnectionImpl() {
    condvar_ = new ythread::CondVar(&mutex_);
    connected_ = false;
    iPhone_ = NULL;
    wsendCommandToDevice_ = NULL;
  }

  virtual void WaitUntilConnected() {
    mutex_.Lock();
    while (!connected_) {
      condvar_->Wait();
    }
    mutex_.Unlock();
  }

  virtual bool IsDirectory(const string& path) {
    afc_directory *hAFCDir;
    if (0 == AFCDirectoryOpen(hAFC_, path.c_str(), &hAFCDir)) {
      AFCDirectoryClose(hAFC_, hAFCDir);
      return true;
    }
    return false;
  }

  virtual bool Unlink(const string& path) {
    return (0 == AFCRemovePath(hAFC_, path.c_str()));
  }

  virtual bool Mkdir(const string& path) {
    return (0 == AFCDirectoryCreate(hAFC_, path.c_str()));
  }

  virtual bool ListFiles(const string& path, vector<string>* files) {
    afc_directory *hAFCDir;
    if (0 == AFCDirectoryOpen(hAFC_, path.c_str(), &hAFCDir)){
      char *buffer = NULL;
      do {
        AFCDirectoryRead(hAFC_, hAFCDir, &buffer);
        if (buffer != NULL) {
          files->push_back(string(buffer));
        }
      } while (buffer != NULL);
      AFCDirectoryClose(hAFC_, hAFCDir);
      return true;
    } else {
      cout << path << ": No such file or directory" << endl;
      return false;
    }
  }

  // TODO: See if the returned dictionary can be used to "stat" he file,
  // instead of reading it all
  virtual bool IsFile(const string& path) {
    afc_dictionary* info;
    return (0 == AFCFileInfoOpen(hAFC_, path.c_str(), &info));
  }

  virtual int GetFileSize(const string& path) {
    struct afc_dictionary *info;
    if (AFCFileInfoOpen(hAFC_, path.c_str(), &info)) {
      return -1;
    }

    unsigned int size;
    char *key, *val;
    while (1) {
        AFCKeyValueRead(info, &key, &val);
        if (!key || !val)
            break;

        if (!strcmp(key, "st_size")) {
            sscanf(val, "%u", &size);
            AFCKeyValueClose(info);
            return size;
        }
    }
    AFCKeyValueClose(info);
    return -1;
  }

  static const unsigned int kReadBlockSize = 5 * 1024;

  virtual bool ReadFileToString(const string& path, string* data) {
    afc_file_ref rAFC;
    int ret = AFCFileRefOpen(hAFC_, path.c_str(), 2, &rAFC);
    if (ret != 0) {
      cout << "Problem with AFCFileRefOpen(2): " << ret << endl;
      return false;
    }
    data->clear();
    unsigned int size = GetFileSize(path);
    char* buf = (char*)malloc(size);
    ret = AFCFileRefRead(hAFC_, rAFC, buf, &size);
    if (ret != 0) {
      AFCFileRefClose(hAFC_, rAFC);
      free(buf);
      cout << "Problem with AFCFileRefWrite: " << ret << endl;
      return false;
    }
    data->append(buf, size);
    free(buf);
    AFCFileRefClose(hAFC_, rAFC);
    return true;
  }

  virtual bool WriteStringToFile(const string& path, const string& data) {
    afc_file_ref rAFC;
    int ret = AFCFileRefOpen(hAFC_, path.c_str(), 3, &rAFC);
    if (ret != 0) {
      cout << "Problem with AFCFileRefOpen(3): " << ret << endl;
      return false;
    }
    if (data.size() > 0) {
      ret = AFCFileRefWrite(hAFC_, rAFC, data.data(), data.size());
      if (ret != 0) {
        AFCFileRefClose(hAFC_, rAFC);
        cout << "Problem with AFCFileRefWrite: " << ret << endl;
        return false;
      }
    }
    AFCFileRefClose(hAFC_, rAFC);
    return true;
  }

  void Start() {
    am_device_notification *notif;
    int ret;
    void* handle = dlopen(
      "/System/Library/PrivateFrameworks/MobileDevice.framework/MobileDevice",
      RTLD_LOCAL);
    if (handle == 0) {
      cout << "Error during dlopen: " << dlerror() << endl;
      exit(EXIT_FAILURE);
    }
    void* knownSymbol = dlsym(handle, "_AMSGetErrorReasonForErrorCode");
    if(knownSymbol == 0) {
      cout << "Error looking up global symbolduring dlsym: " << dlerror()
           << endl;
      exit(EXIT_FAILURE);
    } else {
      wsendCommandToDevice_ = (cmdsend)((char*)knownSymbol + 0xC0E2);
    }
    ret = AMDeviceNotificationSubscribe(device_notification_callback, 0, 0, 0,
                                         &notif);
    if (ret != 0) {
      cout << "Problem registering main callback: " << ret << endl;
      exit(EXIT_FAILURE);
    }
    cout << "Waiting for phone... " << flush;
    CFRunLoopRun();
  }

 private:
  static bool Connect(am_device* iPhone) {
    if (AMDeviceConnect(iPhone_)) {
      int connid;
      cout << "can't connect, trying in recovery mode..." << endl;
      connid = AMDeviceGetConnectionID(iPhone_);
      cout << "Connection ID = " << connid << endl;
      return false;
    }
    if (!AMDeviceIsPaired(iPhone_)) {
      cout << "failed, Pairing Issue" << endl;
      return false;
    }
    if (AMDeviceValidatePairing(iPhone_) != 0) {
      cout << "failed, Pairing NOT Valid" << endl;
      return false;
    }
    if (AMDeviceStartSession(iPhone_))  {
      cout << "failed, Session NOT Started" << endl;
      return false;
    }
    cout << "established." << endl;

    // initial display of iphone status on app run
    int ret = AMDeviceStartService(iPhone_, CFSTR("com.apple.afc"), &hAFC_,
                                   NULL);
    if (ret != 0) {
      cout << "Problem starting AFC: " << ret << endl;
      return false;
    }
    ret = AFCConnectionOpen(hAFC_, 0, &hAFC_);
    if (ret != 0) {
      cout << "Problem with AFCConnectionOpen: " << ret << endl; 
      return false;
    }
    return true;
  }

  static void device_notification_callback(
      am_device_notification_callback_info *info) {
    iPhone_ = info->dev;
    if (info->msg == ADNCI_MSG_CONNECTED && !connected_) {
      mutex_.Lock();
      connected_ = Connect(iPhone_);
      if (connected_) {
        condvar_->SignalAll();
      }
      mutex_.Unlock();
    } else if(info->msg==ADNCI_MSG_DISCONNECTED) {
      cout << "Disonnected!" << endl;
      exit(0);
    }
  }
};

class Factory : public ythread::Thread {
 public:
  Factory() : condvar_(new ythread::CondVar(&mutex_)), connection_(NULL) { }

  virtual ~Factory() {
    delete condvar_;
  }
 
  void Wait() {
    mutex_.Lock();
    while (connection_ == NULL) {
      condvar_->Wait();
    }
    mutex_.Unlock();
  }

  Connection* GetConnection() {
    return connection_;
  }

 protected:
  virtual void Run() {
    mutex_.Lock();
    connection_ = new ConnectionImpl();
    condvar_->SignalAll();
    mutex_.Unlock();
    connection_->Start();
  }

  ythread::Mutex mutex_;
  ythread::CondVar* condvar_;
  ConnectionImpl* connection_;
};

static Factory* factory_ = NULL;

Connection* GetConnection() {
  if (factory_ == NULL) {
    factory_ = new Factory();
    factory_->Start();
    factory_->Wait();
  }
  return factory_->GetConnection();
}

}  // namespace iphonedisk
