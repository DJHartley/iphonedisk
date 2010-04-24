// Author: Allen Porter <allen@thebends.org>
//
// The ProxyFilesystem starts a background thread that invokes the main fuse
// loop.

#include "fs/fs_proxy.h"

#include <string>
#include <fuse/fuse.h>
#include <fuse/fuse_lowlevel.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <syslog.h>
#include "proto/fs_service.pb.h"
#include "fs/fs.h"
#include "fs/fs_fuse.h"

namespace fs {

// A wrapper around a fuse_chan object.  This object mainly exists to enforce
// propper shutdown of the fuse channel.
class MountPoint {
 public:
  MountPoint(const std::string& volname,
             const std::string& volicon) : channel_(NULL) {
    mount_path_ = "/Volumes/";
    mount_path_.append(volname);
    InitFuseArgs(&args_, volname, volicon);
  }

  ~MountPoint() {
    if (channel_ != NULL) {
      fuse_unmount(mount_path_.c_str(), channel_);
    }
    // Ignore errors
    rmdir(mount_path_.c_str());
  }
 
  bool Create() {
    // Ignore errors
    rmdir(mount_path_.c_str());
    mkdir(mount_path_.c_str(), S_IFDIR|0755);

    channel_ = fuse_mount(mount_path_.c_str(), &args_);
    if (channel_ == NULL) {
      syslog(LOG_ERR, "fuse_mount() failed");
      return false;
    }
    return true;
  }

  struct fuse_chan* channel() { return channel_; }
  struct fuse_args* args() { return &args_; }
  const std::string& mount_path() { return mount_path_; }

 private:
  struct fuse_chan* channel_;
  struct fuse_args args_;
  std::string mount_path_;
};

// A wrapper around the fuse object.  This mostly exists to enforce that
// shutdown is performed in the correct order.  This class is not thread safe,
// however it relies on MakeLoopExit() being called from a background thread
// while Loop is running.
class Session {
 public:
  // Session takes ownership of MountPoint
  Session(MountPoint* mount_point) : mount_point_(mount_point),
                                     fuse_(NULL) {
    InitFuseOps(&fuse_ops_);
  }

  // The caller is responsible for making sure that this object is not deleted
  // while the Loop is running.
  ~Session() {
    delete mount_point_;
    if (fuse_ != NULL) {
      fuse_destroy(fuse_);
    }
  }

  bool Create(Context* context) {
    fuse_ = fuse_new(mount_point_->channel(), mount_point_->args(), &fuse_ops_,
                     sizeof(fuse_ops_), context);
    if (fuse_ == NULL) {
      syslog(LOG_INFO, "fuse_new() failed");
      return false;
    }
    return true;
  }

  // Invokes the fuse event loop and blocks until either the filesystem is
  // unmounted by a third party or MakeLoopExit is called.
  void Loop() {
    // TODO(allen): The signal handling code for fuse does not support more than
    // one session at a time, which is a bit worrysome in this context. Fix!
    fuse_set_signal_handlers(fuse_get_session(fuse_));
    // Blocks until the session has exited
    fuse_loop(fuse_);
    // The session has exited, either because the filesystem was unmounted by
    // a third party or because this filesystem object is in the destructor.
    fuse_remove_signal_handlers(fuse_get_session(fuse_));
  }

  // Causes the main fuse loop to exit.  This is typically invoked by another
  // thread to make a thread blocked in Loop exit.  This is a no-op if the
  // loop has already exited.
  void MakeLoopExit() {
    unmount(mount_point_->mount_path().c_str(), MNT_FORCE);
  }

 private:
  MountPoint* mount_point_;
  struct fuse* fuse_;
  struct fuse_operations fuse_ops_;
};

static void* StartMountThread(void* data);

class ProxyFilesystem : public Filesystem {
 public:
  ProxyFilesystem(proto::FsService* service,
                  const std::string& fs_id,
                  const std::string& volname,
                  const std::string& volicon)
      : volname_(volname),
        volicon_(volicon),
        session_(NULL) {
    pthread_mutex_init(&mutex_, NULL);
    pthread_cond_init(&cond_, NULL);
    context_.service = service;
    context_.fs_id = fs_id;
  }

  virtual ~ProxyFilesystem() {
    // Unmount the filesystem if it has not been unmounted already
    Unmount();
    WaitForUnmount();

    pthread_mutex_destroy(&mutex_);
    pthread_cond_destroy(&cond_);
  }

  virtual bool Mount() {
    int rc = pthread_create(&thread_, NULL, &StartMountThread, this);
    if (rc) {
      syslog(LOG_ERR, "pthread_create() failed: %m");
      return false;
    }
    // Block until RunInMountThread is actually running
    pthread_mutex_lock(&mutex_);
    pthread_cond_wait(&cond_, &mutex_);
    bool success = (session_ != NULL);
    pthread_mutex_unlock(&mutex_);
    return success;
  }

  void Unmount() {
    pthread_mutex_lock(&mutex_);
    if (session_ != NULL) {
      session_->MakeLoopExit();
      syslog(LOG_INFO, "Filesystem unmounted");
    }
    pthread_mutex_unlock(&mutex_);
  }

  // Block until the background thread has exited
  virtual void WaitForUnmount() {
    pthread_join(thread_, NULL);
  }

  void RunInMountThread() {
    pthread_mutex_lock(&mutex_);

    MountPoint* mount_point = new MountPoint(volname_, volicon_);
    if (!mount_point->Create()) {
      delete mount_point;
      // Wake Mount()
      pthread_cond_signal(&cond_);
      pthread_mutex_unlock(&mutex_);
      return;
    }

    // session takes ownership of the mount_point
    Session* session = new Session(mount_point);
    if (!session->Create(&context_)) {
      delete session;
      // Wake Mount()
      pthread_cond_signal(&cond_);
      pthread_mutex_unlock(&mutex_);
      return;
    }

    session_ = session;
    // Wake Mount()
    pthread_cond_signal(&cond_);
    pthread_mutex_unlock(&mutex_);

    // Block until a third party unmounts the filesystem or until MakeLoopExit
    // is invoked by the destructor
    session_->Loop();

    pthread_mutex_lock(&mutex_);
    delete session;
    session_ = NULL;
    pthread_mutex_unlock(&mutex_);
  }

 private:
  struct Context context_;
  std::string volname_;
  std::string volicon_;

  // Background thread that actually runs the filesystem loop
  pthread_t thread_;

  pthread_mutex_t mutex_;  // protects session_
  pthread_cond_t cond_;
  Session* session_;
};

static void* StartMountThread(void* data) {
  ProxyFilesystem* fs = static_cast<ProxyFilesystem*>(data);
  fs->RunInMountThread();
  return NULL;
}


Filesystem* NewProxyFilesystem(proto::FsService* service,
                               const std::string& fs_id,
                               const std::string& volname,
                               const std::string& volicon) {
  return new ProxyFilesystem(service, fs_id, volname, volicon);
}

}  // namespace fs
