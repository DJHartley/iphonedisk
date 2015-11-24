# Introduction #

iPhoneDisk is a MacFUSE filesystem plugin that allows you to read and write files on your iPhone.  It uses the MobileDevice library (like iTunes) to access the filesystem of the iPhone over a USB cable.  iPhoneDisk runs on Mac OS X only.

# Installation #

Download the latest version of iPhoneDisk on the [Downloads](http://code.google.com/p/iphonedisk/downloads/list) page.  Double click the .dmg file to mount the install image, then click **Disk for iPhone.mpkg** to install the program.  The .mpkg also installs the lastest version of MacFUSE in addition to the Disk for iPhone Launch Agent.   You must restart your computer before you can use Disk for iPhone (sorry!).

Once restarted, simply connect your iPhone (or similar device) to your computer over USB and the disk image will be mounted on the desktop.

# Advanced Settings #

The arguments to the Disk for iPhone Launch Agent can be modified by editing **`/Library/LaunchAgents/org.thebends.iphonedisk.mobile_fs_util.plist`**.

The first argument is the volume name used when the device is mounted.  The second argument is the icon shown.  The third argument is the afc service name of the Media partition (**com.apple.afc**).

Most "jailbreak" programs (or manual instructions) will set up a second afc service labeled _com.apple.afc2_ with full access to the **Root** partition (/).  There are many documents about [iphone jailbreak](http://www.google.com/search?q=iphone+jailbreak), but here are some links below that might be helpful:

  * [iFuntastic 3.0.3](http://www.macenstein.com/ifuntastic3mirror/iFuntastic_3.0.3.zip) (easy mode)
  * [Jailbreak tutorial with iPHUC](http://iphone.fiveforty.net/wiki/index.php/How_to_Escape_Jail) (hard mode)

# Troubleshooting #

If you run into any problems feel free to join the discussion group at http://groups.google.com/group/iphonedisk.  When posting a problem, it is helpful to include detailed log messages from the system log.  To check for relevant log messages perform these steps:
  1. Open _Console_ from Applications > Utilities.
  1. Click _All Messages_ under _LOG DATABASE QUERIES_
  1. Filter for _iphonedisk_

If things are working correctly you will see messages like
```
4/10/10 2010-04-10 21:49:59 PM org.thebends.iphonedisk.mobile_fs_util[2686] mobile_fs_util: Waiting for device connection 
4/10/10 2010-04-10 21:49:59 PM org.thebends.iphonedisk.mobile_fs_util[2686] mobile_fs_util: AFC Connection established 
4/10/10 2010-04-10 21:49:59 PM org.thebends.iphonedisk.mobile_fs_util[2686] mobile_fs_util: Device connected 
4/10/10 2010-04-10 21:49:59 PM org.thebends.iphonedisk.mobile_fs_util[2686] mobile_fs_util: Mounted mobile-fs as iPhoneDisk 
```