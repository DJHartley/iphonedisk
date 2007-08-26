// iphonediskd.cpp
// Author: Allen Porter <allen@thebends.org>
//
// A standalone server that mounts the iphone when it is connected.
//
// TODO: Catch signals and shutdown fuse fonnection if started

#include <iostream>
#include <sys/wait.h>
#include <sys/types.h>

#include "ythread/callback-inl.h"

#include <CoreFoundation/CoreFoundation.h>
#include "mounter.h"
#include "manager.h"

#define VOLNAME_MEDIA "Media"
#define SERVICE_MEDIA "com.apple.afc"

#define VOLNAME_ROOT "Root"
#define SERVICE_ROOT "com.apple.afc2"

#define ICON "./iPhoneDisk.icns"

using namespace std;

class Controller {
 public:
  Controller() {
    manager_ = iphonedisk::NewManager(
        ythread::NewCallback(this, &Controller::Connect),
        ythread::NewCallback(this, &Controller::Disconnect));
    mounter1_ = iphonedisk::NewMounter();
//    mounter2_ = iphonedisk::NewMounter();
  }

  ~Controller() {
    delete manager_;
    delete mounter1_;
//    delete mounter2_;
  }

 protected:
  void Disconnect() {
    cout << "iPhone disconnected" << endl;
    mounter1_->Stop();
//    mounter2_->Stop();
  }

  void Connect() {
    cout << "iPhone connected" << endl;
    afc_connection* afc1 = manager_->Open(SERVICE_MEDIA);
    if (afc1 == NULL) {
      cerr << "Opening " << SERVICE_MEDIA << " failed" << endl;
      exit(1);
    }
/*
    afc_connection* afc2 = manager_->Open(SERVICE_ROOT);
    if (afc2 == NULL) {
      cerr << "Opening " << SERVICE_ROOT << " failed" << endl;
      exit(1);
    }
*/
    mounter1_->Start(afc1, VOLNAME_MEDIA, ICON);
//    mounter2_->Start(afc2, VOLNAME_ROOT);
  }

  iphonedisk::Manager* manager_;
  iphonedisk::Mounter* mounter1_;
  iphonedisk::Mounter* mounter2_;
};

int main(int argc, char* argv[]) {
  cout << "Initializaing." << endl;
  Controller controller;

  // Loop forever? sure
  select(0, NULL, NULL, NULL, NULL);

  return 0;
}
