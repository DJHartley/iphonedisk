// Author: Allen Porter <allen@thebends.org>

#include <google/protobuf/service.h>

namespace google {
namespace protobuf {
class Closure;
}
}

namespace rpc {

// A simple RpcController implementation that does not support cancellation.
class Rpc : public google::protobuf::RpcController {
 public:
  Rpc();
  virtual ~Rpc();

  // Client-side methods
  virtual void Reset();
  virtual bool Failed() const;
  virtual std::string ErrorText() const;
  virtual void StartCancel();

  // Server-side methods
  virtual void SetFailed(const std::string& reason);
  virtual bool IsCanceled() const;
  virtual void NotifyOnCancel(google::protobuf::Closure* callback);

 private:
  bool failed_;
  std::string error_text_;
};

}  // namespace rpc
