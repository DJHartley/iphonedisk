// connection.h
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
//
// Much code in this module was based on the iPhoneInterface tool by:
//   geohot, ixtli, nightwatch, warren, nail, Operator
// See http://iphone.fiveforty.net/wiki/

// TODO: Perform a regression test with mutexes

#include "ythread/callback-inl.h"

#include "connection.h"
#include <map>
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

using namespace std;

namespace iphonedisk {

static void notify_callback(am_device_notification_callback_info *info);

// This class is kind of a hack, in that it assumes that there is only ever
// one ConnectionImpl, created by the factory below.
class ConnectionImpl : public Connection {
 public:
  ConnectionImpl() : condvar_(new ythread::CondVar(&mutex_)), hAFC_(NULL),
                     connect_cb_(NULL), disconnect_cb_(NULL) { }

  //
  // Methods for the Connection Interface
  //

  // TODO: Wait with timeout?
  virtual bool WaitUntilConnected() {
    ythread::MutexLock l(&mutex_);
    while (hAFC_ == NULL) {
      condvar_->Wait();
    }
    return true;
  }

  virtual bool Unlink(const string& path) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    return (0 == AFCRemovePath(hAFC_, path.c_str()));
  }

  virtual bool Rename(const string& from, const string& to) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    return (0 == AFCRenamePath(hAFC_, from.c_str(), to.c_str()));
  }

  virtual bool Mkdir(const string& path) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    return (0 == AFCDirectoryCreate(hAFC_, path.c_str()));
  }

  virtual bool ListFiles(const string& path, vector<string>* files) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
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
      cerr << "ListFiles: " << path << ": No such file or directory" << endl;
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
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    memset(stbuf, 0, sizeof(struct stat));

    struct afc_dictionary *info;
    if (AFCFileInfoOpen(hAFC_, path.c_str(), &info) != 0) {
      return false;
    }
    map<string, string> info_map;
    CreateMap(info, &info_map);
    AFCKeyValueClose(info);

    if (info_map.find("st_size") == info_map.end()) {
      cerr << "AFCFileInfoOpen: Missing st_size";
      return false;
    }
    if (info_map.find("st_ifmt") == info_map.end()) {
      cerr << "AFCFileInfoOpen: Missing st_ifmt";
      return false;
    }
    if (info_map.find("st_blocks") == info_map.end()) {
      cerr << "AFCFileInfoOpen: Missing st_blocks";
      return false;
    }

    stbuf->st_size = atoll(info_map["st_size"].c_str());
    stbuf->st_blocks = atoll(info_map["st_blocks"].c_str());
    if (info_map["st_ifmt"] == "S_IFDIR") {
      stbuf->st_mode = S_IFDIR | 0777;
      stbuf->st_nlink = 2;
    } else if (info_map["st_ifmt"] == "S_IFREG") {
      stbuf->st_mode = S_IFREG | 0666;
      stbuf->st_nlink = 1;
    } else {
      cerr << "AFCFileInfoOpen: Unknown s_ifmt value: " << info_map["st_ifmt"];
    }
    return true;
  }

  virtual bool GetStatFs(struct statvfs* stbuf) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }

    memset(stbuf, 0, sizeof(struct statvfs));
    struct afc_dictionary *info;
    int ret = AFCDeviceInfoOpen(hAFC_, &info);
    if (ret != 0) {
      cerr << "AFCDeviceInfoOpen: " << ret << endl;
      return false;
    }
    map<string, string> info_map;
    CreateMap(info, &info_map);
    AFCKeyValueClose(info);

    if (info_map.find("FSTotalBytes") == info_map.end()) {
      cerr << "AFCDeviceInfoOpen: Missing FSTotalBytes";
      return false;
    }
    if (info_map.find("FSBlockSize") == info_map.end()) {
      cerr << "AFCDeviceInfoOpen: Missing FSBlockSize";
      return false;
    }
    if (info_map.find("Model") == info_map.end()) {
      cerr << "AFCDeviceInfoOpen: Missing Model";
      return false;
    }
    
    int blockSize = atoi(info_map["FSBlockSize"].c_str());
    unsigned long long totalBytes = atoll(info_map["FSTotalBytes"].c_str());
    unsigned long long freeBytes = atoll(info_map["FSFreeBytes"].c_str());
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
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    afc_file_ref rAFC;
    int ret = AFCFileRefOpen(hAFC_, path.c_str(), mode, &rAFC);
    if (ret != 0) {
      cerr << "AFCFileRefOpen(" << mode << "): " << ret << endl;
      return 0;
    }
    return rAFC;
  }

  virtual bool CloseFile(unsigned long long rAFC) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    return (0 == AFCFileRefClose(hAFC_, rAFC));
  }

  virtual bool ReadFile(unsigned long long rAFC, char* data,
			size_t size, off_t offset) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    int ret = AFCFileRefSeek(hAFC_, rAFC, offset, 0);
    if (ret != 0) {
      cerr << "AFCFileRefSeek: " << ret << endl;
      return false;
    }
    unsigned int afcSize = size;
    ret = AFCFileRefRead(hAFC_, rAFC, data, &afcSize);
    if (ret != 0) {
      cerr << "AFCFileRefRead: " << ret << endl;
      return false;
    }
    return true;
  }

  virtual bool WriteFile(unsigned long long rAFC, const char* data,
			 size_t size, off_t offset) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    if (size > 0) {
      int ret = AFCFileRefSeek(hAFC_, rAFC, offset, 0);
      if (ret != 0) {
        cerr << "AFCFileRefSeek: " << ret << endl;
        return false;
      }
      ret = AFCFileRefWrite(hAFC_, rAFC, data, (unsigned long)size);
      if (ret != 0) {
        cerr << "AFCFileRefWrite: " << ret << endl;
        return false;
      }
    }
    return true;
  }

  virtual bool SetFileSize(const string& path, off_t offset) {
    ythread::MutexLock l(&mutex_);
    if (!Connected()) {
      return false;
    }
    afc_file_ref rAFC;
    int ret = AFCFileRefOpen(hAFC_, path.c_str(), 3, &rAFC);
    if (ret != 0) {
      cerr << "AFCFileRefOpen: " << ret << endl;
      return false;
    }

    ret = AFCFileRefSetFileSize(hAFC_, rAFC, offset);
    AFCFileRefClose(hAFC_, rAFC);
    if (ret != 0) {
      cerr << "AFCFileRefSetFileSize: " << ret << endl;
      return false;
    }
    return true;
  }


  //
  // Methods for initialization
  //

  // Registers appropriate calbacks.  Since this class is always created
  // as a singleton by the ConnectionThread, we can call a static function
  // invokes Notify() on this object.
  bool Init() {
    am_device_notification *notif;
    int ret = AMDeviceNotificationSubscribe(notify_callback, 0, 0, 0, &notif);
    if (ret != 0) {
      cerr << "AMDeviceNotificationSubscribe: " << ret << endl;
      return false;
    }
    return true;
  }

  // Invoked by the static helper method notify_callback
  void Notify(am_device_notification_callback_info *info) {
    ythread::MutexLock l(&mutex_);
    if (info->msg == ADNCI_MSG_CONNECTED && hAFC_ == NULL) {
      hAFC_ = Connect(info->dev);
      if (connect_cb_ != NULL) {
        connect_cb_->Execute();
      }
    } else if(info->msg == ADNCI_MSG_DISCONNECTED) {
      hAFC_ = NULL;
      if (disconnect_cb_ != NULL) {
        disconnect_cb_->Execute();
      }
    }
    condvar_->SignalAll();
  }

  void SetDisconnectCallback(ythread::Callback* cb) {
    ythread::MutexLock l(&mutex_);
    disconnect_cb_ = cb;
  }

  void SetConnectCallback(ythread::Callback* cb) {
    ythread::MutexLock l(&mutex_);
    connect_cb_ = cb;
  }

 private:
  // Converts an AFC Dictionary into a map
  static void CreateMap(struct afc_dictionary* in, map<string, string>* out) {
    char *key, *val;
    while ((AFCKeyValueRead(in, &key, &val) == 0) && key && val) {
      (*out)[key] = val;
    }  
  }

  static afc_connection* Connect(am_device* device) {
    int ret = AMDeviceConnect(device);
    if (ret != 0) {
      cerr << "AMDeviceConnect: " << ret << endl;
      return NULL;
    }
    if (!AMDeviceIsPaired(device)) {
      cerr << "Device was not paired" << endl;
      return NULL;
    }
    ret = AMDeviceValidatePairing(device);
    if (ret != 0) {
      cerr << "AMDeviceValidatePairing: " << ret << endl;
      return NULL;
    }
    ret = AMDeviceStartSession(device);
    if (ret != 0) {
      cerr << "AMDeviceStartSession: " << ret << endl;
      return NULL;
    }
    // TODO: Enable a mode that connects to afc2 to get a (readonly) view of
    // the entire device?
    afc_connection *hAFC;
    ret = AMDeviceStartService(device, CFSTR("com.apple.afc"), &hAFC, NULL);
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

  // Must be called with mutex_ held
  bool Connected() {
    bool connected = (hAFC_ != NULL);
    return connected;
  }

  ythread::Mutex mutex_;        // protects hAFC_ and callbacks
  ythread::CondVar* condvar_;   // wakes WaitUntilConnected
  afc_connection *hAFC_;
  // External callbacks
  ythread::Callback* connect_cb_;
  ythread::Callback* disconnect_cb_;
};


class ConnectionThread : public ythread::Thread {
 public:
  ConnectionThread() : condvar_(new ythread::CondVar(&mutex_)),
                       connection_(NULL), started_(false) { }

  virtual ~ConnectionThread() {
    delete condvar_;
  }
 
  ConnectionImpl* GetConnection() {
    ythread::MutexLock l(&mutex_);
    while (!started_) {
      condvar_->Wait();
    }
    return connection_;
  }

 protected:
  // If initialization fails, the thread is exited immediately
  virtual void Run() {
    bool loop = true;
    mutex_.Lock();
    connection_ = new ConnectionImpl();
    if (!connection_->Init()) {
      delete connection_;
      connection_ = NULL;
      loop = false;
    }
    started_ = true;
    condvar_->SignalAll();
    mutex_.Unlock();

    if (loop) {
      CFRunLoopRun();  // forever
    }
    cerr << "ConnectionThread exiting" << endl;
  }

  ythread::Mutex mutex_;
  ythread::CondVar* condvar_;

  ConnectionImpl* connection_;
  bool started_;
};

static ConnectionThread* thread_ = NULL;

static void notify_callback(am_device_notification_callback_info *info) {
  ConnectionImpl* conn = thread_->GetConnection();
  conn->Notify(info);
}

Connection* GetConnection() {
  if (thread_ == NULL) {
    thread_ = new ConnectionThread();
    thread_->Start();
  }
  return thread_->GetConnection();
}

}  // namespace iphonedisk
