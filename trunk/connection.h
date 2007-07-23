// connection.h
// Authors: Allen Porter <allen@thebends.org> (pin)
//          Scott Turner <scottturner007@gmail.com>
//
// Much code in this module was based on the iPhoneInterface tool by:
//   geohot, ixtli, nightwatch, warren, nail, Operator
// See http://iphone.fiveforty.net/wiki/

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/statvfs.h>

namespace iphonedisk {

// Implementations of the Connection class must be threadsafe.
class Connection {
 public:
  virtual ~Connection() { }

  // Should be invoked before using the connection
  virtual void WaitUntilConnected() = 0;

  virtual unsigned long long OpenFile(const std::string& path, int mode) = 0;

  virtual bool CloseFile(unsigned long long rAFC) = 0;

  virtual bool GetAttr(const std::string& path, struct stat* stbuf) = 0;

  virtual bool IsDirectory(const std::string& path) = 0;

  virtual bool IsFile(const std::string& path) = 0;

  virtual bool GetStatFs(struct statvfs* stbuf) = 0;

  virtual bool Rename(const std::string& from, const std::string& to) = 0;

  virtual bool Unlink(const std::string& path) = 0;

  virtual bool Mkdir(const std::string& path) = 0;

  virtual bool ListFiles(const std::string& dir,
                         std::vector<std::string>* files) = 0;

  virtual bool SetFileSize(const std::string& path, off_t offset) = 0;

  virtual bool ReadFile(unsigned long long rAFC, char* data,size_t size, off_t offset) = 0;

  virtual bool WriteFile(unsigned long long rAFC, const char* data,
			 size_t size, off_t offset) = 0;

 protected:
  // cannot be instantiated directly
  Connection() { }
};

// Not thread safe
Connection* GetConnection();

}  // namespace

#endif
