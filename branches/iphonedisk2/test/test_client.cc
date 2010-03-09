// Author: Allen Porter <allen@thebends.org>

#include <iostream>
#include <string>
#include "proto/fs_service.pb.h"
#include "rpc/rpc.h"
#include "rpc/mach_channel.h"

using namespace google::protobuf;
using namespace rpc;
using namespace proto;
using namespace std;

#define SERVICE_NAME "org.thebends.iphonedisk.test_service.rpc"

int main(int argc, char* argv[]) {
  RpcChannel* channel = NewMachChannel(SERVICE_NAME);
  if (channel == NULL) {
    cerr << "Unable to create channel for service: " << SERVICE_NAME << endl;
    return 1;
  }
  FsService* service = new FsService::Stub(channel);
  GetAttrRequest request;
  request.mutable_header()->set_fs_id("test-id");
  request.set_path("/dev/null");
  while (true) {
    cout << "Sending request: " << request.ShortDebugString() << endl;
    Rpc rpc;
    GetAttrResponse response;
    service->GetAttr(&rpc, &request, &response, NewCallback(DoNothing));
    if (rpc.Failed()) {
      cerr << "Rpc failed: " << rpc.ErrorText() << endl;
      return 1;
    }
    cout << "Response: " << response.ShortDebugString() << endl;
    sleep(1);
  }
  delete service;
  delete channel;
  return 0;
}
