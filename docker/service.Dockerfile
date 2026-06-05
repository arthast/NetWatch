# syntax=docker/dockerfile:1.7
ARG USERVER_IMAGE=ghcr.io/userver-framework/ubuntu-24.04-userver:latest
ARG RUNTIME_IMAGE=ubuntu:24.04
ARG NETWATCH_IMAGE_PLATFORM=linux/amd64

FROM --platform=${NETWATCH_IMAGE_PLATFORM} ${USERVER_IMAGE} AS builder

ARG SERVICE_TARGET
ARG BUILD_TYPE=Release
ARG BUILD_JOBS=1

WORKDIR /workspace
COPY . /workspace

RUN test -n "${SERVICE_TARGET}"
RUN --mount=type=cache,id=netwatch-ccache,target=/root/.cache/ccache \
    set -eux; \
    ccache --zero-stats || true; \
    cmake \
      -S /workspace \
      -B /tmp/netwatch-build \
      -G Ninja \
      -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DUSERVER_FEATURE_GRPC=ON \
      -DUSERVER_FEATURE_KAFKA=ON \
      -DUSERVER_FEATURE_POSTGRESQL=ON \
      -DCMAKE_INSTALL_PREFIX=/opt/netwatch \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=OFF; \
    cmake --build /tmp/netwatch-build -j "${BUILD_JOBS}" --target "${SERVICE_TARGET}"; \
    ccache --show-stats || true; \
    cmake --install /tmp/netwatch-build --component "${SERVICE_TARGET}"

RUN set -eux; \
    binary="/opt/netwatch/bin/${SERVICE_TARGET}"; \
    strip --strip-unneeded "${binary}" || true; \
    mkdir -p /opt/netwatch/lib; \
    ldd "${binary}" \
      | awk '{ path = ($2 == "=>") ? $3 : $1; if (path ~ "^/") print path }' \
      | sort -u \
      | while read -r library; do \
          case "$(basename "${library}")" in \
            ld-linux*|libc.so.*|libdl.so.*|libm.so.*|libpthread.so.*|librt.so.*) continue ;; \
          esac; \
          cp -L "${library}" "/opt/netwatch/lib/$(basename "${library}")"; \
        done

FROM --platform=${NETWATCH_IMAGE_PLATFORM} ${RUNTIME_IMAGE} AS runtime

ARG SERVICE_TARGET
ENV NETWATCH_SERVICE=${SERVICE_TARGET}
ENV USERVER_ENABLE_STACK_USAGE_MONITOR=0
ENV LD_LIBRARY_PATH=/opt/netwatch/lib
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
      ca-certificates \
      curl \
      python3-minimal \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /opt/netwatch /opt/netwatch
COPY docker/runtime-entrypoint.sh /usr/local/bin/netwatch-runtime-entrypoint
RUN chmod +x /usr/local/bin/netwatch-runtime-entrypoint

WORKDIR /opt/netwatch
CMD ["/usr/local/bin/netwatch-runtime-entrypoint"]
