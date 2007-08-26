// DiskController.mm
// Authors: Allen Porter <allen@thebends.org>

#import "DiskController.h"
#include "manager.h"

static const NSString* kMediaAFC = @"com.apple.afc";
static const NSString* kRootAFC = @"com.apple.afc2";
static NSString* service = kMediaAFC;

static iphonedisk::Manager* manager = NULL;

@implementation DiskController

- (IBAction)button:(id)sender
{
	NSLog(@"button");
}

- (IBAction)mediaPressed:(id)sender
{
	service = kMediaAFC;
}

- (IBAction)rootPressed:(id)sender
{
	service = kRootAFC;
}

- (void)awakeFromNib
{
	[progress startAnimation:self];
	[button setHidden:YES];

//	manager = iphonedisk::NewManager(connect_cb, disconnect_cb);
//	manager->SetDisconnectCallback(
	
//  [media setEnabled:NO];
//  [root setEnabled:NO];
}

@end
