// Author: Allen Porter <allen@thebends.org>

#ifndef __MOBILE_FS_SERVICE_H__
#define __MOBILE_FS_SERVICE_H__

#include "mobilefs/mobiledevice.h"

namespace proto {
class FsService;
}

namespace mobilefs {

proto::FsService* NewMobileFsService(afc_connection* conn);

}  // namespace mobilefs

#endif  // __MOBILE_FS_SERVICE_H__
