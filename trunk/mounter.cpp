// mounter.cpp
// Authors: Allen Porter <allen@thebends.org>

#include "ythread/callback-inl.h"
#include <string>
#include <iostream>
#include "mounter.h"
#include "connection.h"
#include "iphonedisk.h"
#include "ythread/thread.h"
#include "ythread/mutex.h"
#include "ythread/condvar.h"

using namespace std;

namespace iphonedisk {

class MounterImpl : public ythread::Thread, public Mounter {
 public:
  MounterImpl() : fuse_(NULL), afc_(NULL), cb_(NULL), mutex_(),
                  condvar_(&mutex_), loop_(false) {
    ythread::Thread::Start();
  }

  virtual ~MounterImpl() {
#ifdef DEBUG
    cerr << "Mounter::Dtor" << endl;
#endif
    Shutdown(true);
    Join();
  }

  virtual void Start(afc_connection* handle,
                     const string& volume,
                     const string& volicon,
                     ythread::Callback* cb) {
#ifdef DEBUG
    cerr << "Mounter::Start (" << volume << ")" << endl;
#endif
    assert(!volume.empty());
    ythread::MutexLock l(&mutex_);
    afc_ = handle;
    volume_ = volume;
    volicon_ = volicon;
    cb_ = cb;
    condvar_.SignalAll();  // Wake Run()
  }

  virtual void Stop() {
#ifdef DEBUG
    cerr << "Mounter::Stop" << endl;
#endif
    Shutdown(false);
  }

 protected: 
  void Shutdown(bool stop_loop) {
    ythread::MutexLock l(&mutex_);
    afc_ = NULL;
    if (fuse_ != NULL) {
      fuse_exit(fuse_);
    }
    loop_ = !stop_loop;
    condvar_.SignalAll();
  }

  virtual void Run() {
#ifdef DEBUG
    cerr << "Mounter::Run" << endl;
#endif
    mutex_.Lock();
    loop_ = true;
    while (loop_) {
      while (loop_ && afc_ == NULL) {
        condvar_.Wait();  // Wait for Connect or Disconnect callback
      }
      if (!loop_) {
        break;
      }
  
      iphonedisk::Connection* conn = iphonedisk::NewConnection(afc_);

      args_ = (struct fuse_args)FUSE_ARGS_INIT(0, NULL);
      fuse_opt_add_arg(&args_, "-d");
#ifdef DEBUG
      fuse_opt_add_arg(&args_, "-odebug");
#endif
      fuse_opt_add_arg(&args_, "-odefer_auth");

      string volname_arg("-ovolname=");
      volname_arg.append(volume_);
      fuse_opt_add_arg(&args_, volname_arg.c_str());

      string volicon_arg("-ovolicon=");
      volicon_arg.append(volicon_);  
      if (!volicon_.empty()) {
        fuse_opt_add_arg(&args_, volicon_arg.c_str());
      }
 
      iphonedisk::InitFuseConfig(conn, &operations_);

      string mount_path("/Volumes/");
      mount_path.append(volume_);
      // Ignore errors
      rmdir(mount_path.c_str());
      mkdir(mount_path.c_str(), S_IFDIR|0755);

      struct fuse_chan* chan = fuse_mount(mount_path.c_str(), &args_);
      if (chan == NULL) {
        cerr << "fuse_mount() failed" << endl;
        exit(1);
      }
      fuse_ = fuse_new(chan, &args_, &operations_, sizeof(operations_), NULL);
      if (fuse_ == NULL) {
        fuse_unmount(mount_path.c_str(), chan);
        cerr << "fuse_new() failed" << endl;
        exit(1);
      }
      mutex_.Unlock();
      fuse_loop(fuse_);

      ythread::Callback* cb = NULL;

      mutex_.Lock();
      afc_ = NULL;
      cb = cb_;
      cb_ = NULL;
      mutex_.Unlock();
      if (cb != NULL) {
        cb->Execute();
        delete cb;
      }
      mutex_.Lock();
      // TODO: This call can take a long time when the iphone has already been
      // disconnected.  Investigate this.
      fuse_unmount(mount_path.c_str(), chan);
      fuse_destroy(fuse_);
      fuse_ = NULL;
    }
    mutex_.Unlock();
#ifdef DEBUG
    cerr << "Mounter::Run done" << endl;
#endif
  }

  struct fuse_operations operations_;
  struct fuse_args args_;
  struct fuse* fuse_;
  afc_connection* afc_;
  string volume_;
  string volicon_;
  ythread::Callback* cb_;
  ythread::Mutex mutex_;
  ythread::CondVar condvar_;
  bool loop_;
};

Mounter* NewMounter() {
  return new MounterImpl();
}

}  // namespace iphonedisk
