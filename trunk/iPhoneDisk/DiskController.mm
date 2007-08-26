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
	[partitionLabel setHidden:YES];
	[radio setHidden:YES];
	[progress stopAnimation:self];
	[progress setHidden:YES];
	[statusLabel setTitleWithMnemonic:@"iPhone connected, Eject disk safely"];
	NSRect rect = [window frame];
	rect.size.width = 184;
	[window setFrame:rect display:YES];
}

- (void)deviceDisconnected:(id)sender
{
	[partitionLabel setHidden:NO];
	[radio setHidden:NO];
	[progress startAnimation:self];
	[progress setHidden:NO];
	[statusLabel setTitleWithMnemonic:@"Waiting for iPhone..."];
	NSRect rect = [window frame];
	rect.size.width = 291;
	[window setFrame:rect display:YES];
}

- (void)awakeFromNib
{
	[progress startAnimation:self];
	[[FuseGlue alloc] initWithController:self];
}

@end
