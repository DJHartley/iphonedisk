// iphonedisk.c
// Author: Allen Porter <allen@thebends.org> 
// Contributions: Scott Turner
//
// A MacFUSE filesystem implementation for the iPhone.
// WARNING: Use at your own risk.
//
// http://www.thebends.org/~allen/code/iPhoneDisk
//
// TODO: Address bug that makes coping files in the finder not work correctly

#include <map>
#include <iostream>
#include <fuse.h>
#include <errno.h>
#include <string.h>
#include "connection.h"

using namespace std;

static iphonedisk::Connection* conn;

static int iphone_getattr(const char* path, struct stat *stbuf) {
  cout << "getattr: " << path << endl;
  return conn->GetAttr(path, stbuf) ? 0 : -ENOENT;
}

static int iphone_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    cout << "readdir: " << path << endl;
    (void) offset;
    (void) fi;

    if (!conn->IsDirectory(path))
        return -ENOENT;

    vector<string> files;
    conn->ListFiles(path, &files);
    for (vector<string>::const_iterator it = files.begin(); it != files.end();
         ++it) {
      filler(buf, it->c_str(), NULL, 0);
    }
    return 0;
}

static int iphone_open(const char *path, struct fuse_file_info *fi) {
    cout << "open:" << path << ", flags=" << fi->flags << endl;
    return 0;
}

static int iphone_unlink(const char* path) {
  cout << "unlink: " << path << endl;
  return conn->Unlink(path) ? 0 : -ENOENT;
} 

static int iphone_mkdir(const char* path, mode_t mode) {
  cout << "mkdir: " << path << endl;
  (void)mode;
  return conn->Mkdir(path) ? 0 : -ENOENT;
}

static int iphone_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
  cout << "read: " << path << endl;
  (void) fi;
  return conn->ReadFile(path, buf, size, offset) ? size : -ENOENT;
}

static int iphone_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
  cout << "write: " << path << endl;
  (void) fi;
  return conn->WriteFile(path, buf, size, offset) ? size : -ENOENT;
}

static int iphone_truncate(const char* path, off_t offset) {
  cout << "truncate: " << path << endl;
  string data;
  if (!conn->ReadFileToString(path, &data)) {
    return -ENOENT;
  }
  if (offset < data.size() &&
      !conn->WriteStringToFile(path, data.substr(0, offset))) {
    cout << "Error truncating file" << endl;
    return -ENOENT;
  } 
  return 0;
}

static int iphone_create(const char *path, mode_t mode,
                         struct fuse_file_info *fi) {
  cout << "create " << path << "; mode=" << mode << endl;
  (void)mode;
  (void)fi;
  if (conn->IsDirectory(path) || conn->IsFile(path)) {
    return -ENOENT;
  }
  if (!conn->WriteStringToFile(path, "")) {
    return -ENOENT;
  }
  return 0;
}

// TODO: Get more accurate file system information
static int iphone_statfs(const char* path, struct statvfs* vfs) {
  cout << "statfs: " << path << endl;
  memset(vfs, 0, sizeof(struct statvfs));

  vfs->f_namemax = 255;
  vfs->f_bsize = 1024;
  vfs->f_frsize = vfs->f_bsize;
  vfs->f_frsize = vfs->f_bsize;
  vfs->f_blocks = vfs->f_bfree = vfs->f_bavail =
          1000ULL * 1024 * 1024 * 1024 / vfs->f_frsize;
  vfs->f_files = vfs->f_ffree = 1000000000;
  return 0;
}

static struct fuse_operations iphone_oper;

int main(int argc, char* argv[]) {
  conn = iphonedisk::GetConnection();
  conn->WaitUntilConnected();

  iphone_oper.getattr = iphone_getattr;
  iphone_oper.readdir = iphone_readdir;
  iphone_oper.open    = iphone_open;
  iphone_oper.read    = iphone_read;
  iphone_oper.write   = iphone_write;
  iphone_oper.unlink  = iphone_unlink;
  iphone_oper.rmdir   = iphone_unlink;
  iphone_oper.truncate = iphone_truncate;
  iphone_oper.mkdir   = iphone_mkdir;
  iphone_oper.create  = iphone_create;
  iphone_oper.statfs  = iphone_statfs;

  cout << "Mounting iPhone Volume..." << endl;
  return fuse_main(argc, argv, &iphone_oper, NULL);
}
