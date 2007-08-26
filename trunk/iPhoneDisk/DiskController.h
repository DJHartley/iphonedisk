/* DiskController */

#import <Cocoa/Cocoa.h>

@interface DiskController : NSObject
{
    IBOutlet NSButton *button;
    IBOutlet NSButtonCell *media;
    IBOutlet NSTextField *partitionLabel;
	IBOutlet NSTextField *statusLabel;
    IBOutlet NSProgressIndicator *progress;
    IBOutlet NSMatrix *radio;
    IBOutlet NSButtonCell *root;
}
- (IBAction)button:(id)sender;
- (IBAction)mediaPressed:(id)sender;
- (IBAction)rootPressed:(id)sender;
- (void)deviceConnected:(id)sender;
- (void)deviceDisconnected:(id)sender;

// YES for Media, NO for Root
- (BOOL)mediaPartition;
@end
