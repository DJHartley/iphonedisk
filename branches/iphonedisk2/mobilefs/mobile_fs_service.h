// Author: Allen Porter <allen@thebends.org>

#include "mobilefs/mobiledevice.h"

namespace proto {
class FsService;
}

namespace mobilefs {

proto::FsService* NewMobileFsService(afc_connection* conn);

}  // namespace mobilefs
