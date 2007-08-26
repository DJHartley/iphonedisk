// connection.h
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
//
// Much code in this module was based on the iPhoneInterface tool by:
//   geohot, ixtli, nightwatch, warren, nail, Operator
// See http://iphone.fiveforty.net/wiki/

#include "connection.h"
#include <map>
#include <iostream>
#include <string>
#include "MobileDevice.h"

using namespace std;

namespace iphonedisk {

// This class is kind of a hack, in that it assumes that there is only ever
// one ConnectionImpl, created by the factory below.
class ConnectionImpl : public Connection {
 public:
  ConnectionImpl(afc_connection* handle)
    : hAFC_(handle) { }

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
    } else if (info_map["st_ifmt"] == "S_IFSOCK") {
      stbuf->st_mode = S_IFSOCK | 0400;
    } else if (info_map["st_ifchr"] == "S_IFCHR") {
      stbuf->st_mode = S_IFCHR | 0400;
   } else {
      cerr << "AFCFileInfoOpen: Unknown s_ifmt value: " << info_map["st_ifmt"]
           << endl;
    }
    return true;
  }

  virtual bool GetStatFs(struct statvfs* stbuf) {
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
    afc_file_ref rAFC;
    int ret = AFCFileRefOpen(hAFC_, path.c_str(), mode, &rAFC);
    if (ret != 0) {
      cerr << "AFCFileRefOpen(" << mode << "): " << ret << endl;
      return 0;
    }
    return rAFC;
  }

  virtual bool CloseFile(unsigned long long rAFC) {
    return (0 == AFCFileRefClose(hAFC_, rAFC));
  }

  virtual bool ReadFile(unsigned long long rAFC, char* data,
                        size_t size, off_t offset) {
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

 private:
  // Converts an AFC Dictionary into a map
  static void CreateMap(struct afc_dictionary* in, map<string, string>* out) {
    char *key, *val;
    while ((AFCKeyValueRead(in, &key, &val) == 0) && key && val) {
      (*out)[key] = val;
    }  
  }

  afc_connection *hAFC_;
};

Connection* NewConnection(afc_connection* handle) {
  return new ConnectionImpl(handle);
}

}  // namespace iphonedisk
