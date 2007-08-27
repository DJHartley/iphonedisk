/* DiskController */

#import <Cocoa/Cocoa.h>

@class FuseGlue;

@interface DiskController : NSObject
{
    IBOutlet id button;
    IBOutlet NSButtonCell *media;
    IBOutlet id mediaMenuItem;
    IBOutlet NSTextField *partitionLabel;
    IBOutlet NSProgressIndicator *progress;
    IBOutlet NSMatrix *radio;
    IBOutlet NSButtonCell *root;
    IBOutlet id quitMenuItem;
    IBOutlet id rootMenuItem;
    IBOutlet id statusMenu;

	FuseGlue *glue;
	BOOL mediaPartitionSelected;
    IBOutlet NSTextField *statusLabel;
    IBOutlet NSWindow *window;

    NSStatusItem *statusItem;
	NSImage *menuIcon;
}
- (IBAction)button:(id)sender;
- (IBAction)mediaPressed:(id)sender;
- (IBAction)quitPressed:(id)sender;
- (IBAction)rootPressed:(id)sender;

// YES for Media, NO for Root
- (BOOL)mediaPartition;
@end
