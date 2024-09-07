FROM alpine:3.14

WORKDIR /app

RUN apt-get update \
    && apt-get install \
    -y --no-install-recommends \
    libssl-dev openssl gcc g++

COPY bin/client_proxy

ENTRYPOINT ["./client_proxy"]
