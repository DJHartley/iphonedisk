// iphonedisk.c
// Authors: Allen Porter <allen@thebends.org> 
//          Scott Turner <scottturner007@gmail.com>
// http://iphonedisk.googlecode.com/
//
// Implements a MacFUSE filesystem.  See /usr/local/include/fuse/fuse.h for
// details of the member functions of the fuse_operations struct which is
// implemented here.

#include "iphonedisk.h"
#include <iostream>
#include <errno.h>
#include <string.h>
#include "connection.h"

using namespace std;

namespace iphonedisk {

static iphonedisk::Connection* conn;

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
static int iphone_getattr(const char* path, struct stat *stbuf) {
#ifdef DEBUG
  cout << "getattr: " << path << endl;
#endif
  return conn->GetAttr(path, stbuf) ? 0 : -ENOENT;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
static int iphone_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
#ifdef DEBUG
    cout << "readdir: " << path << endl;
#endif
    (void) offset;
    (void) fi;

    if (!conn->IsDirectory(path)) {
        return -ENOENT;
    }

    vector<string> files;
    conn->ListFiles(path, &files);
    for (vector<string>::const_iterator it = files.begin(); it != files.end();
         ++it) {
      filler(buf, it->c_str(), NULL, 0);
    }
    return 0;
}

/** Remove a file
 *
*/
static int iphone_unlink(const char* path) {
#ifdef DEBUG
  cout << "unlink: " << path << endl;
#endif
  return conn->Unlink(path) ? 0 : -ENOENT;
} 

/** Create a directory
 *
 */
static int iphone_mkdir(const char* path, mode_t mode) {
#ifdef DEBUG
  cout << "mkdir: " << path << endl;
#endif
  (void)mode;
  return conn->Mkdir(path) ? 0 : -ENOENT;
}

/** Rename a file
 *
 */
static int iphone_rename(const char* from, const char* to) {
#ifdef DEBUG
  cout << "rename: " << from << " to " << to << endl;
#endif
  return conn->Rename(from, to) ? 0 : -ENOENT;
} 

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
static int iphone_open(const char *path, struct fuse_file_info *fi) {
#ifdef DEBUG
  cout << "open:" << path << ", fi= " << fi << ", flags=" << fi->flags << endl;
#endif

  fi->fh = conn->OpenFile(path, ((fi->flags & 1) == 1) ? 3 : 2);
  cout << "file handle: " << fi->fh << endl;

  if (fi->fh == 0) {
    return -ENOENT;
  }
  return 0;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
static int iphone_create(const char *path, mode_t mode,
                         struct fuse_file_info *fi) {
#ifdef DEBUG
  cout << "create " << path << "; mode=" << mode << endl;
#endif
  (void)mode;

  // TODO: Just stat the file instead
  if (conn->IsDirectory(path) || conn->IsFile(path)) {
    return -ENOENT;
  }

  fi->fh = conn->OpenFile(path, 3);
  cout << "file handle: " << fi->fh << endl;

  if (fi->fh == 0) {
    return -ENOENT;
  }
  return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
static int iphone_release(const char *path, struct fuse_file_info *fi) {
#ifdef DEBUG
  cout << "release: " << path << " fi " << fi << " fh " << fi->fh << endl;
#endif
  return conn->CloseFile(fi->fh) ? 0 : -ENOENT;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
static int iphone_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
#ifdef DEBUG
  cout << "read: " << path << " fi " << fi << " fh " << fi->fh << endl;
#endif

  if (!conn->ReadFile(fi->fh, buf, size, offset)) {
    return -ENOENT;
  }

#ifdef DEBUG
  cout << "[*] read " << size << " bytes";
#endif
  return size;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
static int iphone_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
#ifdef DEBUG
  cout << "write: " << path << endl;
#endif

  if (!conn->WriteFile(fi->fh, buf, size, offset)) {
    return -ENOENT;
  }

  cout << "[*] wrote " << size << " bytes\n";
  return size;
}

/**
 * Change the size of a file
 *
 */
static int iphone_truncate(const char* path, off_t offset) {
#ifdef DEBUG
  cout << "truncate: " << path << endl;
#endif

  if (!conn->SetFileSize(path, offset)) {
    return -ENOENT;
  }

  return 0;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
#if 0
struct statvfs {
	unsigned long	f_bsize;	/* File system block size */
	unsigned long	f_frsize;	/* IGNORED Fundamental file system block size */
	fsblkcnt_t	f_blocks;	/* Blocks on FS in units of f_frsize */
	fsblkcnt_t	f_bfree;	/* Free blocks */
	fsblkcnt_t	f_bavail;	/* Blocks available to non-root */
	fsfilcnt_t	f_files;	/* Total inodes */
	fsfilcnt_t	f_ffree;	/* Free inodes */
	fsfilcnt_t	f_favail;	/* IGNORED Free inodes for non-root */
	unsigned long	f_fsid;		/* IGNORED Filesystem ID */
	unsigned long	f_flag;		/* IGNORED Bit mask of values */
	unsigned long	f_namemax;	/* Max file name length */
};
#endif
static int iphone_statfs(const char* path, struct statvfs* vfs) {
#ifdef DEBUG
  cout << "statfs: " << path << endl;
#endif
  return conn->GetStatFs(vfs) ? 0 : -ENOENT;
}

static int iphone_chown(const char* path, uid_t uid, gid_t) {
  return 0;
}

static int iphone_chmod(const char* path, mode_t) {
  return 0;
}

static int iphone_utimens(const char* path, const struct timespec tv[2]) {
  return 0;
}

void InitFuseConfig(Connection* connection,
                    struct fuse_operations* iphone_oper)  {
  conn = connection;
  iphone_oper->getattr  = iphone_getattr;
  iphone_oper->readdir  = iphone_readdir;
  iphone_oper->open     = iphone_open;
  iphone_oper->create   = iphone_create;
  iphone_oper->release  = iphone_release;
  iphone_oper->read     = iphone_read;
  iphone_oper->write    = iphone_write;
  iphone_oper->truncate = iphone_truncate;
  iphone_oper->unlink   = iphone_unlink;
  iphone_oper->rename   = iphone_rename;
  iphone_oper->mkdir    = iphone_mkdir;
  iphone_oper->rmdir    = iphone_unlink;
  iphone_oper->statfs   = iphone_statfs;
  iphone_oper->chown    = iphone_chown;
  iphone_oper->chmod    = iphone_chmod;
  iphone_oper->utimens  = iphone_utimens;
}

}  // namespace
