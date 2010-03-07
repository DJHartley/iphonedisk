// Author: Allen Porter <allen@thebends.org>

#include "rpc/rpc.h"

namespace rpc {

Rpc::Rpc() {
  Reset();
}

Rpc::~Rpc() { }

void Rpc::Reset() {
  failed_ = false;
  error_text_.clear();
}

bool Rpc::Failed() const {
  return failed_;
}

std::string Rpc::ErrorText() const {
  return error_text_;
}

void Rpc::StartCancel() { }

  // Server-side methods
void Rpc::SetFailed(const std::string& reason) {
  failed_ = true;
  error_text_ = reason;
}

bool Rpc::IsCanceled() const {
  return false;
}

void Rpc::NotifyOnCancel(google::protobuf::Closure* callback) {
}

}  // namespace rpc
