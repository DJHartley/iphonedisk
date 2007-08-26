// manager.h
// Authors: Allen Porter <allen@thebends.org>
//          Scott Turner <scottturner007@gmail.com>
//
// The Manager interface is used to get notifications when the device is
// connected.  

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <string>

struct afc_connection;
namespace ythread { class Callback; }

namespace iphonedisk {

class Manager {
 public:
  virtual ~Manager() { }

  // Blocks until the device has been physically connected.  Returns false
  // if there was a fatal error.
  virtual bool WaitUntilConnected() = 0;

  // Returns an afc_connection handle to the currently connected device, or
  // NULL if a connection could not be established.
  // service should be an AFC service name such as:
  //    com.apple.afc
  //    com.apple.afc2
  virtual afc_connection* Open(const std::string& service) = 0;

 protected:
  // cannot be instantiated directly
  Manager() { }
};

// Async notifications of connection and disconnection events are optional and
// may be NULL.
Manager* NewManager(ythread::Callback* connect_cb,
                    ythread::Callback* disconnect_cb);

}  // namespace iphonedisk

#endif  // __MANAGER_H__
