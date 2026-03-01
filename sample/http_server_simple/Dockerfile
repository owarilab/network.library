FROM ubuntu:24.04

RUN useradd -m -s /usr/sbin/nologin server

WORKDIR /opt/server

COPY qs_http_server /opt/server/qs_http_server
COPY www /opt/server/www

RUN chmod +x /opt/server/qs_http_server \
    && echo 'server_port=80;' > /opt/server/server.conf \
    && echo 'echo("server_port = " + server_port + "\\n");' >> /opt/server/server.conf \
    && chown -R server:server /opt/server

USER server

EXPOSE 80

CMD ["./qs_http_server"]