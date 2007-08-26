// connection.h
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
//
// Much code in this module was based on the iPhoneInterface tool by:
//   geohot, ixtli, nightwatch, warren, nail, Operator
// See http://iphone.fiveforty.net/wiki/
// 
// The default Connection implementation uses AFC to read and write files on
// the iPhone filesystem.  An AFC connection handle can be created with a
// iphonedisk::Manager object, and is passed to the GetConnection() method
// to create a new Connection object, wraping the AFC handle.

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/statvfs.h>

struct afc_connection;
namespace ythread { class Callback; }

namespace iphonedisk {

// Implementations of the Connection class must be threadsafe.
class Connection {
 public:
  virtual ~Connection() { }

  //
  // Reading properties of files/directories
  //
  virtual bool GetAttr(const std::string& path, struct stat* stbuf) = 0;
  virtual bool IsDirectory(const std::string& path) = 0;
  virtual bool IsFile(const std::string& path) = 0;
  virtual bool GetStatFs(struct statvfs* stbuf) = 0;
  virtual bool ListFiles(const std::string& dir,
                         std::vector<std::string>* files) = 0;

  //
  // Manipulating files/directories
  //
  virtual unsigned long long OpenFile(const std::string& path, int mode) = 0;
  virtual bool CloseFile(unsigned long long rAFC) = 0;
  virtual bool Mkdir(const std::string& path) = 0;
  virtual bool Unlink(const std::string& path) = 0;
  virtual bool Rename(const std::string& from, const std::string& to) = 0;
  virtual bool SetFileSize(const std::string& path, off_t offset) = 0;
  virtual bool ReadFile(unsigned long long rAFC, char* data,size_t size,
                        off_t offset) = 0;
  virtual bool WriteFile(unsigned long long rAFC, const char* data,
			 size_t size, off_t offset) = 0;

 protected:
  // cannot be instantiated directly
  Connection() { }
};

// Creates the Connection object.
Connection* NewConnection(afc_connection* handle);

}  // namespace

#endif
