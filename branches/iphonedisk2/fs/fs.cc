// Author: Allen Porter <allen@thebends.org>
//
// A fuse filesystem that proxies the filesystem requests through a protocol
// buffer RPC service (FsService).

#include "fs.h"

#include <iostream>
#include <fuse.h>
#include <errno.h>
#include <sys/stat.h>
#include "proto/fs.pb.h"
#include "proto/fs_service.pb.h"
#include "rpc/rpc.h"

namespace fs {

static proto::FsService* g_service = NULL;
static google::protobuf::Closure* g_null_callback = NULL;

static int fs_getattr(const char* path, struct stat* stbuf) {
  rpc::Rpc rpc;
  proto::GetAttrRequest request;
  proto::GetAttrResponse response;
  request.set_path(path);
  g_service->GetAttr(&rpc, &request, &response, g_null_callback);
  if (rpc.Failed()) {
    return -ENOENT;
  }
  if (!response.status().success()) {
    return -ENOENT;
  }
  stbuf->st_size = response.stat().size();
  stbuf->st_blocks = response.stat().blocks();
  stbuf->st_mode = response.stat().mode();
  stbuf->st_nlink = response.stat().nlink();
  return 0; 
}

static int fs_chown(const char* path, uid_t uid, gid_t) {
  return 0;
}

static int fs_chmod(const char* path, mode_t) {
  return 0;
}

static int fs_utimens(const char* path, const struct timespec tv[2]) {
  return 0;
}

void Initialize(proto::FsService* service,
                struct fuse_operations* fuse_op) {
  assert(g_service == NULL);
  assert(g_null_callback == NULL);
  g_service = service;
  g_null_callback = google::protobuf::NewCallback(&google::protobuf::DoNothing);
  fuse_op->getattr  = fs_getattr;
/*
  fuse_op->readdir  = fs_readdir;
  fuse_op->open     = fs_open;
  fuse_op->create   = fs_create;
  fuse_op->release  = fs_release;
  fuse_op->read     = fs_read;
  fuse_op->write    = fs_write;
  fuse_op->truncate = fs_truncate;
  fuse_op->unlink   = fs_unlink;
  fuse_op->rename   = fs_rename;
  fuse_op->mkdir    = fs_mkdir;
  fuse_op->rmdir    = fs_unlink;
  fuse_op->statfs   = fs_statfs;
*/
  fuse_op->chown    = fs_chown;
  fuse_op->chmod    = fs_chmod;
  fuse_op->utimens  = fs_utimens;
}

}  // namespace fs
