FROM debian:latest as builder

RUN apt update && \
    apt install -y \
      automake \
      gcc \
      git \
      gnutls-dev \
      libconfuse-dev \
      libtool \
      make && \
    rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/troglobit/inadyn.git
WORKDIR inadyn
RUN ./autogen.sh && ./configure && make install

FROM debian:latest

RUN apt update && \
    apt install -y \
      ca-certificates \
      libconfuse1 \
      libgnutls30 && \
    rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/sbin/inadyn /usr/bin/inadyn
ENTRYPOINT [ "/usr/bin/inadyn" ]
CMD [ "--foreground" ]
