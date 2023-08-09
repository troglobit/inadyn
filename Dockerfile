FROM alpine:latest

WORKDIR /root
COPY . ./
RUN apk --update --no-cache add --virtual .build-dependencies \
  autoconf \
  automake \
  confuse-dev \
  gcc \
  gnutls-dev \
  libc-dev \
  libtool \
  make \
  && ./autogen.sh && ./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var && make install


FROM alpine:latest

RUN apk --update --no-cache add \
  ca-certificates \
  confuse \
  gnutls

COPY --from=0 /usr/sbin/inadyn /usr/sbin/inadyn
COPY --from=0 /usr/share/doc/inadyn /usr/share/doc/inadyn
ENTRYPOINT ["/usr/sbin/inadyn", "--foreground"]
