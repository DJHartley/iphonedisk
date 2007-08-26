/* DiskController */

#import <Cocoa/Cocoa.h>

@class FuseGlue;

@interface DiskController : NSObject
{
    IBOutlet NSButtonCell *media;
    IBOutlet NSTextField *partitionLabel;
    IBOutlet NSProgressIndicator *progress;
    IBOutlet NSMatrix *radio;
    IBOutlet NSButtonCell *root;
    IBOutlet NSTextField *statusLabel;
    IBOutlet NSWindow *window;
	IBOutlet NSButton *button;
	
	FuseGlue* glue;
}
- (IBAction)button:(id)sender;
- (IBAction)mediaPressed:(id)sender;
- (IBAction)rootPressed:(id)sender;
- (void)deviceConnected:(id)sender;
- (void)deviceDisconnected:(id)sender;
- (void)deviceUnmounted:(id)sender;

// YES for Media, NO for Root
- (BOOL)mediaPartition;
@end
