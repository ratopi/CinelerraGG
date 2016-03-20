#!/bin/bash

if [ `id -u` -ne 0 ]; then
  echo "you must be root"
fi

if [ $# -ne 1 ]; then
  echo "usage: $0 <os>"
  echo "  <os> = [centos | fedora | suse | ubuntu]"
fi

dir="$1"

case "$dir" in
"centos")
  yum -y install nasm libavc1394-devel libusb-devel flac-devel \
    libjpeg-devel libdv-devel libdvdnav-devel libdvdread-devel \
    libtheora-devel libiec61883-devel uuid-devel giflib-devel \
    ncurses-devel ilmbase-devel fftw-devel OpenEXR-devel \
    libsndfile-devel libXft-devel libXinerama-devel libXv-devel \
    xorg-x11-fonts-misc xorg-x11-fonts-cyrillic xorg-x11-fonts-Type1 \
    xorg-x11-fonts-ISO8859-1-100dpi xorg-x11-fonts-ISO8859-1-75dpi \
    libpng-devel bzip2-devel zlib-devel kernel-headers \
    libavc1394 festival-devel libiec61883-devel flac-devel \
    libsndfile-devel libtheora-devel linux-firmware ivtv-firmware \
    libvorbis-devel texinfo xz-devel lzma-devel cmake
    yasm=yasm-1.2.0-7.fc21.x86_64.rpm
    release=http://archives.fedoraproject.org/pub/fedora/linux/releases/21
    url=$release/Everything/x86_64/os/Packages/y/$yasm
    wget -P /tmp $url
    yum -y install /tmp/$yasm
    rm -f /tmp/$yasm
  ;;
"fedora")
  yum -y install nasm yasm libavc1394-devel libusb-devel flac-devel \
    libjpeg-devel libdv-devel libdvdnav-devel libdvdread-devel \
    libtheora-devel libiec61883-devel esound-devel uuid-devel \
    giflib-devel ncurses-devel ilmbase-devel fftw-devel OpenEXR-devel \
    libsndfile-devel libXft-devel libXinerama-devel libXv-devel \
    xorg-x11-fonts-misc xorg-x11-fonts-cyrillic xorg-x11-fonts-Type1 \
    xorg-x11-fonts-ISO8859-1-100dpi xorg-x11-fonts-ISO8859-1-75dpi \
    libpng-devel bzip2-devel zlib-devel kernel-headers libavc1394 \
    festival-devel libdc1394-devel libiec61883-devel esound-devel \
    flac-devel libsndfile-devel libtheora-devel linux-firmware \
    ivtv-firmware libvorbis-devel texinfo xz-devel lzma-devel cmake
  ;;
"suse")
  zypper -n install nasm gcc gcc-c++ zlib-devel texinfo libpng16-devel \
    freeglut-devel libXv-devel alsa-devel libbz2-devel ncurses-devel \
    libXinerama-devel freetype-devel libXft-devel giblib-devel ctags \
    bitstream-vera-fonts xorg-x11-fonts-core xorg-x11-fonts dejavu-fonts \
    openexr-devel libavc1394-devel festival-devel libjpeg8-devel libdv-devel \
    libdvdnav-devel libdvdread-devel libiec61883-devel libuuid-devel \
    ilmbase-devel fftw3-devel libsndfile-devel libtheora-devel flac-devel cmake
    if [ ! -f /usr/lib64/libtermcap.so ]; then
      ln -s libtermcap.so.2 /usr/lib64/libtermcap.so
    fi
  ;;
"ubuntu")
  apt-get -y install apt-file sox nasm yasm g++ build-essential libz-dev texinfo \
    libpng-dev freeglut3-dev libxv-dev libasound2-dev libbz2-dev \
    libncurses5-dev libxinerama-dev libfreetype6-dev libxft-dev giblib-dev \
    exuberant-ctags ttf-bitstream-vera xfonts-75dpi xfonts-100dpi \
    fonts-dejavu libopenexr-dev libavc1394-dev festival-dev \
    libdc1394-22-dev libiec61883-dev libesd0-dev libflac-dev \
    libsndfile1-dev libtheora-dev git cmake
  ;;
 *)
  echo "unknown os: $dir"
  exit 1;
  ;;
esac

rm -rf "/home/$dir"
mkdir -p "/home/$dir"
chmod a+rwx -R "/home/$dir"

