// FuseGlue.mm
// Authors: Allen Porter <allen@thebends.org>

#import <Cocoa/Cocoa.h>

@class DiskController;
struct afc_connection;
namespace iphonedisk { class Manager; }
namespace iphonedisk { class Mounter; }

@interface FuseGlue : NSObject {
	iphonedisk::Manager* manager;
	iphonedisk::Mounter* mounter;
	DiskController* diskController;
	afc_connection* afc;
}

- (id)initWithController:(DiskController*)controller;
- (void)deviceConnected;
- (void)deviceDisconnected;
- (void)deviceUnmounted;
- (void)deviceReload;

@end
