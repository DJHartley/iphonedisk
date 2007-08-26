// iphonedisk_mount.cpp
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
//
// This is a command line tool used to mount the iPhones disk manually.  See
// accompanying mount.sh shell script for reasonable default flags that should
// be provided to mac fuse.

#include "ythread/callback-inl.h"
#include <iostream>
#include "connection.h"
#include "iphonedisk.h"
#include "manager.h"

using namespace std;

static struct fuse_operations iphone_oper;

class Watcher {
 public:
  Watcher(iphonedisk::Manager* manager) {
    manager->SetDisconnectCallback(
      ythread::NewCallback(this, &Watcher::Disconnect));
  }

 private:
  void Disconnect() {
    cerr << "Device disconnected!" << endl;
    exit(1);
  }
};

int main(int argc, char* argv[]) {
  cout << "Initializaing." << endl;
  iphonedisk::Manager* manager = iphonedisk::NewManager();
  if (manager == NULL) {
    cerr << "Unable to initialize connection to device";
    return 1;
  }

  Watcher watcher(manager);
  cout << "Waiting for device..." << endl;
  if (!manager->WaitUntilConnected()) {
    cerr << "Error while waiting for device" << endl;
    return 1;
  }

  afc_connection* afc = manager->Open("com.apple.afc");
  if (afc == NULL) {
    cerr << "Unable to initialize connection to service";
    return 1;
  }
  iphonedisk::Connection* conn = iphonedisk::NewConnection(afc);

  cout << "Initializing filesystem." << endl;
  iphonedisk::InitFuseConfig(conn, &iphone_oper);

  cout << "Mounting iPhone Volume." << endl;
  return fuse_main(argc, argv, &iphone_oper, NULL);
}
