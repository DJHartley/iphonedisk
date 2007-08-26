// DiskController.mm
// Authors: Allen Porter <allen@thebends.org>

#import "DiskController.h"
#import "FuseGlue.h"

@implementation DiskController

- (IBAction)button:(id)sender
{
	[glue deviceConnected];
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

- (void)setShowPartitions:(BOOL)show
{
	NSRect rect = [window frame];
	rect.size.width = show ? 291 : 184;
	[window setFrame:rect display:YES];
}

- (void)deviceConnected:(id)sender
{
	[partitionLabel setHidden:YES];
	[radio setHidden:YES];
	[progress stopAnimation:self];
	[progress setHidden:YES];
	[statusLabel setTitleWithMnemonic:@"Connected"];
	[self setShowPartitions:NO];
	[button setHidden:YES];
}

- (void)deviceDisconnected:(id)sender
{
	[partitionLabel setHidden:NO];
	[radio setHidden:NO];
	[progress startAnimation:self];
	[progress setHidden:NO];
	[statusLabel setTitleWithMnemonic:@"Waiting for iPhone..."];
	[self setShowPartitions:YES];
	[button setHidden:YES];
}

- (void)deviceUnmounted:(id)sender
{
	[partitionLabel setHidden:NO];
	[radio setHidden:NO];
	[progress stopAnimation:self];
	[progress setHidden:YES];
	[statusLabel setTitleWithMnemonic:@"Connected; iPhone not mounted"];
	[self setShowPartitions:YES];
	[button setHidden:NO];
}

- (void)awakeFromNib
{
	[self deviceDisconnected:self];
	glue = [[FuseGlue alloc] initWithController:self];
}

@end
