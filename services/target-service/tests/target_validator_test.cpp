#include <targets/validation/target_validator.hpp>

#include <userver/utest/utest.hpp>

namespace netwatch::target_service {
namespace {
CreateTargetRequest MakeHttpTarget() {
  return CreateTargetRequest{
      .name = "Main website",
      .type = TargetType::kHttp,
      .url = "https://example.com/health",
      .method = "GET",
      .expected_status_code = 200,
      .host = std::nullopt,
      .port = std::nullopt,
      .interval_seconds = 30,
      .timeout_ms = 1000,
  };
}

CreateTargetRequest MakeTcpTarget() {
  return CreateTargetRequest{
      .name = "Postgres",
      .type = TargetType::kTcp,
      .url = std::nullopt,
      .method = std::nullopt,
      .expected_status_code = std::nullopt,
      .host = "localhost",
      .port = 5432,
      .interval_seconds = 10,
      .timeout_ms = 500,
  };
}
}  // namespace
}  // namespace netwatch::target_service

UTEST(TargetValidator, AcceptsValidHttpTarget) {
  const auto error =
      netwatch::target_service::validator::ValidateCreateTargetRequest(
          netwatch::target_service::MakeHttpTarget());

  EXPECT_FALSE(error.has_value());
}

UTEST(TargetValidator, AcceptsValidTcpTarget) {
  const auto error =
      netwatch::target_service::validator::ValidateCreateTargetRequest(
          netwatch::target_service::MakeTcpTarget());

  EXPECT_FALSE(error.has_value());
}

UTEST(TargetValidator, RejectsHttpTargetWithoutUrl) {
  auto request = netwatch::target_service::MakeHttpTarget();
  request.url = std::nullopt;

  const auto error =
      netwatch::target_service::validator::ValidateCreateTargetRequest(request);

  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(*error, "http target requires url");
}

UTEST(TargetValidator, RejectsTcpTargetWithInvalidPort) {
  auto request = netwatch::target_service::MakeTcpTarget();
  request.port = 70000;

  const auto error =
      netwatch::target_service::validator::ValidateCreateTargetRequest(request);

  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(*error, "tcp target port must be between 1 and 65535");
}

UTEST(TargetValidator, RejectsTimeoutGreaterThanInterval) {
  auto request = netwatch::target_service::MakeHttpTarget();
  request.interval_seconds = 5;
  request.timeout_ms = 5000;

  const auto error =
      netwatch::target_service::validator::ValidateCreateTargetRequest(request);

  ASSERT_TRUE(error.has_value());
  EXPECT_EQ(*error,
            "timeout_ms must be less than interval_seconds in milliseconds");
}
