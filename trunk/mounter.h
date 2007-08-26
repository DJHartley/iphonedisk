// mounter.h
// Authors: Allen Porter <allen@thebends.org>

#include "ythread/thread.h"

struct afc_connection;

namespace iphonedisk {

class Mounter {
 public:
  virtual ~Mounter() { }

  // volicon may be empty
  // cb is invoked when the drive is unmounted.
  virtual void Start(afc_connection* handle,
                     const std::string& volume,
					 const std::string& volicon = "",
					 ythread::Callback* cb = NULL) = 0;
  virtual void Stop() = 0;

 protected:
  // cannot be instantiated directly
  Mounter() { }
};

Mounter* NewMounter();

}  // namespace iphonedisk
