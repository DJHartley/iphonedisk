// FuseGlue.mm
// Authors: Allen Porter <allen@thebends.org>
// TODO: Nice UI for failure modes?

#import "callback-inl.h"

#import "FuseGlue.h"
#import "DiskController.h"
#import "manager.h"
#import "mounter.h"

static const char* kMediaLabel = "Media";
static const char* kMediaAFC = "com.apple.afc";

static const char* kRootLabel = "Root";
static const char* kRootAFC = "com.apple.afc2";

//
// Bridge between C++ iphonedisk library and DiskController
//

static void ConnectCallback(FuseGlue* glue)
{
  [glue deviceConnected];
}

static void DisconnectCallback(FuseGlue* glue)
{
  [glue deviceDisconnected];
}

static void UnmountCallback(FuseGlue* glue)
{
  [glue deviceUnmounted];
}

@implementation FuseGlue

- (id)initWithController:(DiskController*)controller
{
	self = [super init];
	diskController = controller;
	manager = iphonedisk::NewManager(
		ythread::NewCallback(&ConnectCallback, self),
		ythread::NewCallback(&DisconnectCallback, self));
	mounter = iphonedisk::NewMounter();
	afc = NULL;
	return self;
}

- (void)deviceConnected
{
    [[NSAutoreleasePool alloc] init];

#ifdef DEBUG
	NSLog(@"Device connected");
#endif
	if ([diskController mediaPartition]) {
		afc = manager->Open(kMediaAFC);
	} else {
		afc = manager->Open(kRootAFC);
	}
	if (afc == NULL) {
		NSLog(@"No AFC connection available");
		exit(1);
	}
	
	NSBundle *thisBundle = [NSBundle bundleForClass:[self class]];
	NSString* volIconPath =
		[thisBundle pathForResource:@"iPhoneDisk" ofType:@"icns"];
	const char* volicon = [volIconPath cString];

	ythread::Callback* unmount_cb = ythread::NewCallback(&UnmountCallback, self);

	if ([diskController mediaPartition]) {
		mounter->Start(afc, kMediaLabel, volicon, unmount_cb);
	} else {
		mounter->Start(afc, kRootLabel, volicon, unmount_cb);
	}
	[diskController performSelectorOnMainThread:@selector(deviceConnected:)
									 withObject:diskController
								  waitUntilDone:YES];
}

- (void)deviceUnmounted
{
#ifdef DEBUG
	NSLog(@"Device unmounted");
#endif
	if (afc == NULL) {
		NSLog(@"No AFC established?");
		return;
	}
}

- (void)deviceDisconnected
{
#ifdef DEBUG
	NSLog(@"Device disconnected");
#endif
	afc = NULL;
	mounter->Stop();
	[diskController performSelectorOnMainThread:@selector(deviceDisconnected:)
									 withObject:diskController
								  waitUntilDone:YES];
}

@end
