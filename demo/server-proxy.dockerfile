FROM alpine:3.14

WORKDIR /app

RUN apt-get update \
    && apt-get install \
    -y --no-install-recommends \
    libssl-dev openssl gcc g++

COPY bin/server_proxy

ENTRYPOINT ["./server_proxy"]
