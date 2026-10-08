FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends gcc-i686-linux-gnu make perl qemu-system-x86 \
 && rm -rf /var/lib/apt/lists/*

RUN printf '#!/bin/sh\nexec i686-linux-gnu-gcc -fno-pie "$@"\n' > /usr/local/bin/i386-elf-gcc \
 && chmod 755 /usr/local/bin/i386-elf-gcc \
 && ln -s /usr/bin/i686-linux-gnu-ld      /usr/local/bin/i386-elf-ld \
 && ln -s /usr/bin/i686-linux-gnu-objcopy /usr/local/bin/i386-elf-objcopy

RUN ln /usr/bin/qemu-system-x86_64 /usr/bin/qemu

COPY pintos/src/utils/backtrace pintos/src/utils/pintos pintos/src/utils/pintos-mkdisk \
     pintos/src/utils/pintos-set-cmdline pintos/src/utils/Pintos.pm pintos/src/utils/pintos-gdb \
     pintos/src/misc/gdb-macros /usr/bin/
RUN chmod 755 /usr/bin/backtrace /usr/bin/pintos /usr/bin/pintos-mkdisk \
              /usr/bin/pintos-set-cmdline /usr/bin/pintos-gdb \
 && chmod 644 /usr/bin/Pintos.pm /usr/bin/gdb-macros

WORKDIR /pintos/src