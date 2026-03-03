# ============================================================
# Stage 1: build
# ============================================================
FROM alpine:3.21 AS builder

RUN apk add --no-cache gcc musl-dev make

WORKDIR /build

# コアライブラリとサーバーソースをコピー
COPY core/ core/
COPY sample/http_server_simple/ sample/http_server_simple/

# コアライブラリをビルド（_LINUX 定義を渡す）
RUN cd core/src && make DEFINE=-D_LINUX

# HTTP サーバーをビルド
RUN cd sample/http_server_simple && make

# ============================================================
# Stage 2: runtime
# ============================================================
FROM alpine:3.21

RUN adduser -D -H -s /sbin/nologin server

WORKDIR /opt/server

COPY --from=builder /build/sample/http_server_simple/qs_http_server ./qs_http_server
COPY sample/http_server_simple/www ./www
RUN echo 'server_port=80;' > ./server.conf

RUN chmod +x ./qs_http_server \
    && chown -R server:server /opt/server

USER server

EXPOSE 80

CMD ["./qs_http_server"]
