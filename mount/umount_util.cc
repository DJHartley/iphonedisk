// Author: Allen Porter <allen@thebends.org>
//
// Command line tool that issues Unmount RPCs to a mount service.

#include <iostream>
#include <string>
#include "proto/mount_service.pb.h"
#include "rpc/rpc.h"
#include "rpc/mach_channel.h"

using namespace google::protobuf;
using namespace rpc;
using namespace proto;
using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <mount service name> <fsid> <volume>\n",
            argv[0]);
    return 1;
  }
  RpcChannel* channel = NewMachChannel(argv[1]);
  if (channel == NULL) {
    cerr << "Unable to create channel for service: " << argv[1] << endl;
    return 1;
  }

  MountService* service = new MountService::Stub(channel);
  UnmountRequest request;
  request.set_fs_id(argv[2]);
  request.set_volume(argv[3]);
  Rpc rpc;
  UnmountResponse response;
  service->Unmount(&rpc, &request, &response, NewCallback(DoNothing));
  if (rpc.Failed()) {
    cerr << "Rpc failed: " << rpc.ErrorText() << endl;
  } else {
    cout << "OK" << endl;
  }
  delete service;
  delete channel;
  return rpc.Failed() ? 1 : 0;
}
