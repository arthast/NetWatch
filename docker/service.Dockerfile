ARG USERVER_IMAGE=ghcr.io/userver-framework/ubuntu-24.04-userver:latest
ARG NETWATCH_IMAGE_PLATFORM=linux/amd64

FROM --platform=${NETWATCH_IMAGE_PLATFORM} ${USERVER_IMAGE} AS builder

ARG SERVICE_TARGET
ARG BUILD_TYPE=Release
ARG BUILD_JOBS=1

WORKDIR /workspace
COPY . /workspace

RUN test -n "${SERVICE_TARGET}"
RUN cmake \
      -S /workspace \
      -B /tmp/netwatch-build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DUSERVER_FEATURE_GRPC=ON \
      -DUSERVER_FEATURE_POSTGRESQL=ON \
      -DCMAKE_INSTALL_PREFIX=/opt/netwatch \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF \
    && cmake --build /tmp/netwatch-build -j "${BUILD_JOBS}" --target "${SERVICE_TARGET}" \
    && cmake --install /tmp/netwatch-build --component "${SERVICE_TARGET}"

FROM --platform=${NETWATCH_IMAGE_PLATFORM} ${USERVER_IMAGE} AS runtime

ARG SERVICE_TARGET
ENV NETWATCH_SERVICE=${SERVICE_TARGET}
ENV USERVER_ENABLE_STACK_USAGE_MONITOR=0

COPY --from=builder /opt/netwatch /opt/netwatch

WORKDIR /opt/netwatch
CMD ["/bin/bash", "-lc", "exec /opt/netwatch/bin/${NETWATCH_SERVICE} --config /opt/netwatch/etc/${NETWATCH_SERVICE}/static_config.yaml --config_vars /opt/netwatch/etc/${NETWATCH_SERVICE}/config_vars.compose.yaml"]
