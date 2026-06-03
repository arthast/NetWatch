#pragma once

#include <userver/ugrpc/client/call_options.hpp>

namespace netwatch::client_common {

userver::ugrpc::client::CallOptions MakeGrpcCallOptions();

}  // namespace netwatch::client_common
