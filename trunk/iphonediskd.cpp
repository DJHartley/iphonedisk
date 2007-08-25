// iphonediskd.cpp
// Author: Allen Porter <allen@thebends.org>
//
// A standalone server that mounts the iphone when it is connected.
//
// TODO: Catch signals and shutdown fuse fonnection if started


#include <CoreFoundation/CoreFoundation.h>
#include "iphonedisk.h"
#include "connection.cpp"
#include "ythread/callback.h"
#include "ythread/mutex.h"
#include "ythread/condvar.h"

// TODO: Make this a command line or config parameter
#define MOUNT_POINT "/Volumes/iPhone"
#define SERVICE "com.service.afc"

class FuseThread : public ythread::Thread {
 public:
  FuseThread(iphonedisk::Connection* conn)
      : fuse_(NULL),
        conn_(conn),
        connected_(false),
        mutex_(),
        condvar_(&mutex_) { }

 protected:
  void Disconnect() {
    cout << "iPhone disconnected" << endl;
    ythread::MutexLock l(&mutex_);
    if (fuse_ != NULL) {
      fuse_exit(fuse_);
    }
  }

  void Connect() {
#ifdef DEBUG
    cout << "iPhone connected" << endl;
#endif
    ythread::MutexLock l(&mutex_);
    condvar_.Signal();  // Wake Run()
  }

  virtual void Run() {
    mutex_.Lock();
    conn_->SetConnectCallback(NewCallback(this, &FuseThread::Connect));
    conn_->SetDisconnectCallback(NewCallback(this, &FuseThread::Disconnect));
    while (1) {
      cout << "Waiting for connection..." << endl;
      condvar_.Wait();  // Wait for Connect or Disconnect callback

      cout << "Initializing" << endl;
      args_ = (struct fuse_args)FUSE_ARGS_INIT(0, NULL);
      fuse_opt_add_arg(&args_, "-d");
      fuse_opt_add_arg(&args_, "-odebug");
      fuse_opt_add_arg(&args_, "-odefer_auth");
      fuse_opt_add_arg(&args_, "-ovolname=iPhone");
      fuse_opt_add_arg(&args_, "-ovolicon=./iPhoneDisk.icns");
      iphonedisk::InitFuseConfig(conn_, &operations_);

      // Ignore errors
      rmdir(MOUNT_POINT);
      mkdir(MOUNT_POINT, S_IFDIR|0755);

      struct fuse_chan* chan = fuse_mount(MOUNT_POINT, &args_);
      if (chan == NULL) {
        cerr << "fuse_mount() failed" << endl;
        exit(1);
      }
      cout << "Mounting filesystem" << endl;
      fuse_ = fuse_new(chan, &args_, &operations_, sizeof(operations_), NULL);
      if (fuse_ == NULL) {
        fuse_unmount(MOUNT_POINT, chan);
        cerr << "fuse_new() failed" << endl;
        exit(1);
      }
      mutex_.Unlock();
      cout << "Starting fuse" << endl;
      fuse_loop(fuse_);
      cout << "Unmounting filesystem" << endl;
      mutex_.Lock();
      // TODO: This call can take a long time when the iphone has already been
      // disconnected.  Investigate this.
      fuse_unmount(MOUNT_POINT, chan);
      fuse_destroy(fuse_);
      fuse_ = NULL;
    }
  } 

  struct fuse_operations operations_;
  struct fuse_args args_;
  struct fuse* fuse_;
  iphonedisk::Connection* conn_;
  bool connected_;
  ythread::Mutex mutex_;
  ythread::CondVar condvar_;
};

int main(int argc, char* argv[]) {
  cout << "Initializaing." << endl;
  iphonedisk::Connection* conn = iphonedisk::GetConnection(SERVICE);
  if (conn == NULL) {
    cerr << "Unable to initialize" << endl;
    return 1;
  } 
  ythread::Thread* t = new FuseThread(conn);
  t->Start();
  t->Join();
  delete conn;
  cout << "Program exited.";
}
