// iphonedisk_mount.cpp
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
//
// This is a command line tool used to mount the iPhones disk manually.  See
// accompanying mount.sh shell script for reasonable default flags that should
// be provided to mac fuse.

#include "iphonedisk.h"
#include "connection.cpp"

#define SERVICE "com.apple.afc"

static struct fuse_operations iphone_oper;

class Watcher {
 public:
  Watcher(iphonedisk::Connection* conn) {
    conn->SetDisconnectCallback(
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
  iphonedisk::Connection* conn = iphonedisk::GetConnection(SERVICE);
  if (conn == NULL) {
    cerr << "Unable to initialize connection to device";
    return 1;
  }

  Watcher watcher(conn);
  cout << "Waiting for device..." << endl;
  if (!conn->WaitUntilConnected()) {
    cerr << "Unable to connect to device" << endl;
    exit(1);
  }

  cout << "Initializing filesystem." << endl;
  iphonedisk::InitFuseConfig(conn, &iphone_oper);

  cout << "Mounting iPhone Volume." << endl;
  return fuse_main(argc, argv, &iphone_oper, NULL);
}
