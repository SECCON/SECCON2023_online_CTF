FROM ubuntu:22.04

ENV DEBIAN_FRONTEND noninteractive

RUN apt-get -y update --fix-missing
RUN apt-get -y upgrade
RUN apt-get -y install xinetd cpio hashcash
RUN groupadd -r pwn && useradd -r -g pwn pwn

ADD build/pwn.xinetd /etc/xinetd.d/pwn
ADD build/init.sh /etc/init.sh
ADD build/pow.sh /home/pwn/pow.sh
RUN chmod 700 /etc/init.sh
RUN chmod 550 /home/pwn/pow.sh
RUN chmod 1733 /tmp /var/tmp /dev/shm

WORKDIR /home/pwn
ADD files/ukqmemo/release .
RUN chmod a+x qemu-system-x86_64 run.sh

RUN mkdir rootfs
WORKDIR rootfs
RUN gzip -d < ../rootfs.cpio.gz | cpio -iv

ARG FLAG1
RUN echo "${FLAG1}" > flag1.txt
ARG FLAG2
RUN echo "${FLAG2}" > root/flag2.txt
RUN find . | cpio --quiet -o -H newc | gzip -c > ../rootfs.cpio.gz

WORKDIR /home/pwn
RUN rm -rf rootfs
ARG FLAG3
RUN echo "${FLAG3}" > flag.txt
RUN chmod 440 flag.txt
RUN mv flag.txt flag-$(md5sum flag.txt | awk '{print $1}').txt

RUN chown -R root:pwn /home/pwn
RUN chmod -R go-w /home/pwn
