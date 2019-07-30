FROM alpine:latest

RUN apk --no-cache add --virtual .build-dependencies \
  autoconf \
  automake \
  confuse-dev \
  gcc \
  git \
  gnutls-dev \
  libc-dev \
  libtool \
  make

RUN git clone https://github.com/troglobit/inadyn.git
WORKDIR inadyn
RUN ./autogen.sh && ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var && make install

RUN apk del .build-dependencies

RUN apk --no-cache add \
  ca-certificates \
  confuse \
  gnutls

ENTRYPOINT [ "inadyn" ]
CMD [ "--foreground" ]
