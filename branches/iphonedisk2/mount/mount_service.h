// Author: Allen Porter

#include <string>

namespace proto {
class MountService;
}

namespace fs {

proto::MountService* NewMountService(const std::string& fs_service_name);

}  // namespace
