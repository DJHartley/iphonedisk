// iphonedisk.h
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
// http://iphonedisk.googlecode.com/
//
// Initializes the iphonedisk fuse configuration with the specified connection.

#ifndef __IPHONEDISK_H__
#define __IPHONEDISK_H__

#define _FILE_OFFSET_BITS 64
#define __FreeBSD__ 10
#define FUSE_USE_VERSION 26

#include <fuse.h>

namespace iphonedisk {

class Connection;

void InitFuseConfig(Connection* connection, struct fuse_operations* oper);

}  // namespace iphonedisk

#endif  // __IPHONEDISK_H__
