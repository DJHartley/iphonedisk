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

  // Async notifications of connection and disconnection events.  May be
  // invoked from an internal connection thread.
  virtual void SetDisconnectCallback(ythread::Callback* cb) = 0;
  virtual void SetConnectCallback(ythread::Callback* cb) = 0;

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

Manager* NewManager();

}  // namespace iphonedisk

#endif  // __MANAGER_H__
