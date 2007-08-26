// DiskController.mm
// Authors: Allen Porter <allen@thebends.org>

#import "DiskController.h"
#import "FuseGlue.h"

@implementation DiskController

- (IBAction)button:(id)sender
{
}

- (IBAction)mediaPressed:(id)sender
{
}

- (IBAction)rootPressed:(id)sender
{
}

- (BOOL)mediaPartition
{
	return ([media state] == NSOnState);
}

- (void)deviceConnected:(id)sender
{
	[media setEnabled:NO];
	[root setEnabled:NO];
	[progress stopAnimation:self];
	[progress setHidden:YES];
	[statusLabel setTitleWithMnemonic:@"iPhone connected"];
}

- (void)deviceDisconnected:(id)sender
{
	[media setEnabled:YES];
	[root setEnabled:YES];
	[progress startAnimation:self];
	[progress setHidden:NO];
	[statusLabel setTitleWithMnemonic:@"Waiting for iPhone..."];
}

- (void)awakeFromNib
{
	[progress startAnimation:self];
	[button setHidden:YES];
	[[FuseGlue alloc] initWithController:self];
}

@end
