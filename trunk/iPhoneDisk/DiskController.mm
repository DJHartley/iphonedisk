#import "DiskController.h"
#include "connection.h"

static iphonedisk::Connection* conn = NULL;
static const char* kMediaAFC = "com.apple.afc";
static const char* kRootAFC = "com.apple.afc";

@implementation DiskController

- (IBAction)button:(id)sender
{
	NSLog(@"button");
}

- (IBAction)mediaPressed:(id)sender
{
	conn->SetService(kMediaAFC);
}

- (IBAction)rootPressed:(id)sender
{
	conn->SetService(kRootAFC);
}

- (void)awakeFromNib
{
	conn = iphonedisk::GetConnection(kMediaAFC);
	[progress startAnimation:self];
	[button setHidden:YES];
//  [media setEnabled:NO];
//  [root setEnabled:NO];
}

@end
