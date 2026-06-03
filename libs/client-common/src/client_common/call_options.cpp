#include <client_common/call_options.hpp>

#include <chrono>

namespace netwatch::client_common {

userver::ugrpc::client::CallOptions MakeGrpcCallOptions() {
  userver::ugrpc::client::CallOptions options;
  options.SetAttempts(1);
  options.SetTimeout(std::chrono::milliseconds{1000});
  return options;
}

}  // namespace netwatch::client_common
