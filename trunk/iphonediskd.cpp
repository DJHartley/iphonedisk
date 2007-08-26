// iphonediskd.cpp
// Author: Allen Porter <allen@thebends.org>
//
// A standalone server that mounts the iphone when it is connected.
//
// TODO: Catch signals and shutdown fuse fonnection if started

#include <iostream>
#include "ythread/callback-inl.h"
#include <CoreFoundation/CoreFoundation.h>
#include "mounter.h"
#include "manager.h"

// TODO: Make this a command line or config parameter
#define VOLNAME "iPhone"
#define SERVICE "com.apple.afc"

using namespace std;

class Controller {
 public:
  Controller() {
    manager_ = iphonedisk::NewManager(
        ythread::NewCallback(this, &Controller::Connect),
        ythread::NewCallback(this, &Controller::Disconnect));
    mounter_ = iphonedisk::NewMounter();
  }

  ~Controller() {
    delete manager_;
    delete mounter_;
  }

 protected:
  void Disconnect() {
    cout << "iPhone disconnected" << endl;
    mounter_->Stop();
  }

  void Connect() {
    cout << "iPhone connected" << endl;
    afc_connection* afc = manager_->Open(SERVICE);
    if (afc == NULL) {
      cerr << "Opening " << SERVICE << " failed" << endl;
      exit(1);
    }
    mounter_->Start(afc, VOLNAME);
  }

  iphonedisk::Manager* manager_;
  iphonedisk::Mounter* mounter_;
};

int main(int argc, char* argv[]) {
  cout << "Initializaing." << endl;
  Controller controller;
  CFRunLoopRun();
  cout << "Program exited.";
}
