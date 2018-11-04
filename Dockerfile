FROM alpine:latest as builder

RUN apk --no-cache add \
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
RUN ./autogen.sh && ./configure && make install

FROM alpine:latest

COPY --from=builder /usr/local/sbin/inadyn /usr/bin/inadyn

ENTRYPOINT [ "/usr/bin/inadyn" ]
CMD [ "--foreground" ]
