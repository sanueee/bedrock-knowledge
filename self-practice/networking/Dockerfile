FROM gcc:latest
RUN apt-get update && apt-get install -y \
      libpcap-dev \
      libcap-ng-dev \
      libcap2-bin \
      iproute2 \
      iputils-ping \
      netcat-openbsd \
      python3 \
      tcpdump \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /work
CMD ["bash"]
