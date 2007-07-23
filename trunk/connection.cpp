// connection.h
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
//
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
#include "ythread/condvar.h"
#include "ythread/mutex.h"
#include "ythread/thread.h"
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
static afc_connection *hAFC_;

// This class is kind of a hack, in that it assumes that there is only ever
// one ConnectionImpl, created by the factory below.
class ConnectionImpl : public Connection {
 public:
  ConnectionImpl() {
    condvar_ = new ythread::CondVar(&mutex_);
    connected_ = false;
    iPhone_ = NULL;
  }

  virtual void WaitUntilConnected() {
    mutex_.Lock();
    while (!connected_) {
      condvar_->Wait();
    }
    mutex_.Unlock();
  }

  virtual bool Unlink(const string& path) {
    return (0 == AFCRemovePath(hAFC_, path.c_str()));
  }

  virtual bool Rename(const string& from, const string& to) {
    return (0 == AFCRenamePath(hAFC_, from.c_str(), to.c_str()));
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

  virtual bool IsDirectory(const string& path) {
    struct stat stbuf;
    return GetAttr(path, &stbuf) && ((stbuf.st_mode & S_IFDIR) != 0);
  }

  virtual bool IsFile(const string& path) {
    struct stat stbuf;
    return GetAttr(path, &stbuf) && ((stbuf.st_mode & S_IFREG) != 0);
  }

  virtual bool GetAttr(const string& path, struct stat* stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    struct afc_dictionary *info;
    if (AFCFileInfoOpen(hAFC_, path.c_str(), &info)) {
      return false;
    }

    char *key, *val;
    while (1) {
      AFCKeyValueRead(info, &key, &val);
      if (!key || !val)
        break;

      if (!strcmp(key, "st_size")) {
        stbuf->st_size = atoll(val);
      } else if (!strcmp(key, "st_blocks")) {
        stbuf->st_blocks = atoll(val);
      } else if (!strcmp(key, "st_ifmt")) {
        if (!strcmp(val, "S_IFDIR")) {
          stbuf->st_mode = S_IFDIR | 0777;
          stbuf->st_nlink = 2;
        } else if (!strcmp(val, "S_IFREG")) {
          stbuf->st_mode = S_IFREG | 0666;
          stbuf->st_nlink = 1;
        } else {
          cout << "Unhandled file type:" << val;
        }
      } else {
        cout << "Unhandled stat value:" << key << "|" << val;
      }
    }
    AFCKeyValueClose(info);
    return true;
  }

  virtual bool GetStatFs(struct statvfs* stbuf) {
    memset(stbuf, 0, sizeof(struct statvfs));
    struct afc_dictionary *info;
    if (AFCDeviceInfoOpen(hAFC_, &info) != 0) {
      cout << "AFCDeviceInfo failed" << endl;
      return false;
    }

    unsigned long long totalBytes = 0;
    unsigned long long freeBytes = 0;
    int blockSize = 4096;

    // TODO: Write a function for converting an afc_dictionary into a map
    char *key, *val;
    while (1) {
      AFCKeyValueRead(info, &key, &val);
      if (!key || !val)
        break;
      cout << "Key=" << key << ",val=" << val << endl;
      if (!strcmp(key, "FSTotalBytes")) {
        totalBytes = atoll(val);
      } else if (!strcmp(key, "FSFreeBytes")) {
        freeBytes = atoll(val);
      } else if (!strcmp(key, "FSBlockSize")) {
        blockSize = atoll(val);
      } else if (!strcmp(key, "Model")) {
        // ignore
      } else {
        cout << "Unhandled statfs value: " << key << "|" << val;
      }
    }

    stbuf->f_namemax = 255;
    stbuf->f_bsize = blockSize;
    stbuf->f_frsize = stbuf->f_bsize;
    stbuf->f_frsize = stbuf->f_bsize;
    stbuf->f_blocks = totalBytes / blockSize;
    stbuf->f_bfree = stbuf->f_bavail = freeBytes / blockSize;
    stbuf->f_files = 110000;
    stbuf->f_ffree = stbuf->f_files - 10000;
    return true;
  }

  virtual unsigned long long OpenFile(const string& path, int mode) {
    afc_file_ref rAFC;
    int ret = AFCFileRefOpen(hAFC_, path.c_str(), mode, &rAFC);
    if (ret != 0) {
      cout << "Problem with AFCFileRefOpen(2): " << ret << endl;
      return 0;
    }

    return rAFC;
  }

  virtual bool CloseFile(unsigned long long rAFC) {
    return (0 == AFCFileRefClose(hAFC_, rAFC));
  }

  virtual bool ReadFile(unsigned long long rAFC, char* data,
			size_t size, off_t offset) {
    AFCFileRefSeek(hAFC_, rAFC, offset, 0);

    unsigned int afcSize = size;
    int ret = AFCFileRefRead(hAFC_, rAFC, data, &afcSize);

    if (ret != 0) {
      cout << "Problem with AFCFileRefRead: " << ret << endl;
      return false;
    }

    return true;
  }

  virtual bool WriteFile(unsigned long long rAFC, const char* data,
			 size_t size, off_t offset) {
    if (size > 0) {
      AFCFileRefSeek(hAFC_, rAFC, offset, 0);

      int ret = AFCFileRefWrite(hAFC_, rAFC, data, (unsigned long)size);

      if (ret != 0) {
        cout << "Problem with AFCFileRefWrite: " << ret << endl;
        return false;
      }
    }
    return true;
  }

  virtual bool SetFileSize(const string& path, off_t offset) {
    afc_file_ref rAFC;
    int ret = AFCFileRefOpen(hAFC_, path.c_str(), 3, &rAFC);
    if (ret != 0) {
      cout << "Problem with AFCFileRefOpen(3): " << ret << endl;
      return false;
    }

    ret = AFCFileRefSetFileSize(hAFC_, rAFC, offset);

    AFCFileRefClose(hAFC_, rAFC);

    if (ret != 0) {
      cout << "Problem with AFCFileRefSetFileSize: " << ret << endl;
      return false;
    }

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
