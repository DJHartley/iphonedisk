// Author: Allen Porter <allen@thebends.org>
//
// Dummy implementation of FsService that listens for RPCs from the test client.

#include <iostream>
#include <string>
#include "proto/fs_service.pb.h"
#include "rpc/mach_service.h"

using namespace google::protobuf;
using namespace rpc;
using namespace proto;
using namespace std;

#define SERVICE_NAME "org.thebends.iphonedisk.test_service.rpc"

class TestService : public FsService {
 public:
  virtual void GetAttr(RpcController* controller,
                       const GetAttrRequest* request,
                       GetAttrResponse* response,
                       Closure* done) {
    cout << "Request: " << request->ShortDebugString() << endl;
    done->Run();
  }
};

int main(int argc, char* argv[]) {
  FsService* service = new TestService;  
  if (!ExportService(SERVICE_NAME, service)) {
    cerr << "Failed to export " << SERVICE_NAME << endl;
    delete service;
    return 1;
  }
  delete service;
  return 0;
}
