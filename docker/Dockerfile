FROM php:7.3.5-alpine3.9

LABEL maintainer="codinghuang"

ENV LIBUV_VERSION="1.30.1"

# install compile tool
RUN apk add --no-cache \
    gcc \
    g++ \
    autoconf \
    automake \
    libtool \
    make

# install libuv
RUN cd /tmp \
    && wget https://github.com/libuv/libuv/archive/v${LIBUV_VERSION}.tar.gz \
    && tar -xvf v${LIBUV_VERSION}.tar.gz \
    && cd libuv-${LIBUV_VERSION} \
    && sh autogen.sh \
    && ./configure \
    && make \
    && make install \
    && cd /tmp && rm -r v${LIBUV_VERSION}.tar.gz libuv-${LIBUV_VERSION}

# install debug tools
RUN apk add --no-cache --update gdb