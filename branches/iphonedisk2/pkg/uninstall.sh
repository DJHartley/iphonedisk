#!/bin/bash
#
# This script should be run with "sudo"

SERVICE_NAME=org.thebends.iphonedisk.mobile_fs_util
BINARY_DIR=/Library/iphonedisk
AGENT_PLIST=/Library/LaunchAgents/org.thebends.iphonedisk.mobile_fs_util.plist

echo "Stopping $SERVICE_NAME"
/bin/launchctl stop $SERVICE_NAME
echo "Unloading $AGENT_PLIST"
/bin/launchctl unload $AGENT_PLIST
echo "Removing $AGENT_PLIST"
if [ -e $AGENT_PLIST ]; then
  /bin/rm -f $AGENT_PLIST
fi
echo "Removing $BINARY_DIR"
if [ -d $BINARY_DIR ]; then
  /bin/rm -fr $BINARY_DIR
fi
echo "Done"
