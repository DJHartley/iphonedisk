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
	mediaPartitionSelected = YES;
	[mediaMenuItem setState:NSOnState];
	[rootMenuItem setState:NSOffState];
	[statusItem setToolTip:@"iPhoneDisk:/Volumes/Media"];
	[glue deviceReload];
}

- (IBAction)rootPressed:(id)sender
{
	mediaPartitionSelected = NO;
	[mediaMenuItem setState:NSOffState];
	[rootMenuItem setState:NSOnState];
	[statusItem setToolTip:@"iPhoneDisk:/Volumes/Root"];
	[glue deviceReload];
}

- (IBAction)quitPressed:(id)sender
{
	[glue deviceDisconnected];
	exit(0);
}

- (BOOL)mediaPartition
{
	return mediaPartitionSelected;
}

- (void)awakeFromNib
{
	statusItem = [[[NSStatusBar systemStatusBar] 
      statusItemWithLength:NSVariableStatusItemLength]
      retain];
	  
	NSBundle *bundle = [NSBundle bundleForClass:[self class]];
	NSString *path = [bundle pathForResource:@"MenuItem" ofType:@"tif"];
	menuIcon = [[NSImage alloc] initWithContentsOfFile:path];

	[statusItem setTitle:[NSString stringWithString:@""]];
	[statusItem setImage:menuIcon];
	[statusItem setHighlightMode:YES];
	[statusItem setEnabled:YES];
	[statusItem setMenu:statusMenu];


	// Default; use media partition
	mediaPartitionSelected = YES;
	[mediaMenuItem setState:NSOnState];
	[rootMenuItem setState:NSOffState];
	[statusItem setToolTip:@"iPhoneDisk:/Volumes/Media"];	

	glue = [[FuseGlue alloc] initWithController:self];
}

@end
