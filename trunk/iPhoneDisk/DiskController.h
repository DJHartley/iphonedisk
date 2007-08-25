/* DiskController */

#import <Cocoa/Cocoa.h>

@interface DiskController : NSObject
{
    IBOutlet NSButton *button;
    IBOutlet NSButtonCell *media;
    IBOutlet NSProgressIndicator *progress;
    IBOutlet NSMatrix *radio;
    IBOutlet NSButtonCell *root;
}
- (IBAction)button:(id)sender;
- (IBAction)mediaPressed:(id)sender;
- (IBAction)rootPressed:(id)sender;
@end
