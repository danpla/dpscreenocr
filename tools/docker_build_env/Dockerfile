# This Dockerfile not only uses Ubuntu 14.04 as the base image, but is
# also written to work with Docker 1.6.2 shipped with 14.04. This old
# Docker doesn't support some constructs in Dockerfile (like ARG), as
# well as some commands like:
#
# * "docker container prune", which can be replaced by:
#   docker rm $(docker ps --filter=status=exited --filter=status=dead -q)
#
# * "docker image prune", which can be replaced by:
#   docker rmi $(docker images --filter dangling=true -q)

FROM ubuntu:14.04

WORKDIR /tmp

# Install the basic tools available via the package manager, including
# the development libraries needed to build both other libraries and
# our program itself.
#
# GCC
# ===
#
# Ubuntu 14.04 is shipped with old GCC 4.8, while we need C++17
# support. ppa:ubuntu-toolchain-r/test provides newer GCC versions;
# GCC 9 is the latest version provided by this PPA for Ubuntu 14.04.
#
# Obviously, libstdc++ of all GCC from this PPA are newer than the
# the one from GCC 4.8, but we can pick the newest GCC anyway, becasue
# libstdc++ will be bundled, and all libstdc++ from PPA are linked
# against the stock libc of Ubuntu 14.04.
#
#
# libcurl
# =======
#
# Since we will load libcurl dynamically rather than linking against
# it, it doesn't actually matter which flavor of libcurl4-*-dev to
# install - we only need the headers.
RUN \
    export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y update \
    && apt-get -y install \
        software-properties-common \
    && add-apt-repository -y \
        ppa:ubuntu-toolchain-r/test \
    && apt-get -y update \
    && apt-get -y install \
        automake \
        ccache \
        g++-9 \
        gcc-9 \
        libcurl4-openssl-dev \
        libdbus-1-dev \
        libfontconfig1-dev \
        libfreetype6-dev \
        libglib2.0-dev \
        libpng-dev \
        libtool \
        libx11-dev \
        libx11-xcb-dev \
        libxcb-glx0-dev \
        libxcb-icccm4-dev \
        libxcb-image0-dev \
        libxcb-keysyms1-dev \
        libxcb-randr0-dev \
        libxcb-render-util0-dev \
        libxcb-shape0-dev \
        libxcb-shm0-dev \
        libxcb-sync0-dev \
        libxcb-xfixes0-dev \
        libxcb-xinerama0-dev \
        libxcb1-dev \
        libxext-dev \
        libxfixes-dev \
        libxi-dev \
        libxkbcommon-dev \
        libxkbcommon-x11-dev \
        libxrender-dev \
        make \
        patch \
        pkg-config \
        wget \
        zlib1g-dev \
    && rm -rf "/var/lib/apt/lists"/*

# Note that we can also set GCC via CC and CXX environment variables,
# but Qt ignores them when building the qmake tool. This can also be
# solved by installing default GCC 4.5, but we don't want to waste the
# disc space and network traffic.
RUN \
    update-alternatives \
        --install "/usr/bin/gcc" gcc "/usr/bin/gcc-9" 9 \
    && update-alternatives \
        --install "/usr/bin/g++" g++ "/usr/bin/g++-9" 9

# Remove outdated certificates so that wget works with
# https://ftp.gnu.org.
RUN \
    sed -i '/.*DST_Root_CA_X3.*/d' "/etc/ca-certificates.conf" \
    && dpkg-reconfigure -fnoninteractive ca-certificates

# CMake
#
# Ubuntu 14.04 has a cmake3 package of version 3.5, but we need at
# least 3.16.
RUN \
    VERSION="3.27.4" \
    && INSTALLER="cmake-$VERSION-linux-x86_64.sh" \
    && INSTALLER_SHA256="3c1af5d2375b00d0969b7295cfcd65c02e38ffc0629539ae40f0a46f8fec462b" \
    && wget \
        --progress=dot:mega \
        "https://github.com/Kitware/CMake/releases/download/v$VERSION/$INSTALLER" \
    \
    && echo "$INSTALLER_SHA256 *$INSTALLER" > "$INSTALLER.sha256" \
    && sha256sum -c "$INSTALLER.sha256" \
    && rm "$INSTALLER.sha256" \
    \
    && chmod +x "$INSTALLER" \
    && "./$INSTALLER" --skip-license --prefix="/usr/local" \
    && rm "$INSTALLER"

# Pandoc
#
# Pandoc on Ubuntu 14.04 is too old: it works, but the rendered HTML
# looks weird compared to newer versions.
RUN \
    VERSION="3.1.8" \
    && DEB="pandoc-$VERSION-1-amd64.deb" \
    && DEB_SHA256="b9755f35abc652bf8a64a4fa134babba4921f45d8a43bb9b88fed50f62b31622" \
    && wget \
        --progress=dot:mega \
        "https://github.com/jgm/pandoc/releases/download/$VERSION/$DEB" \
    \
    && echo "$DEB_SHA256 *$DEB" > "$DEB.sha256" \
    && sha256sum -c "$DEB.sha256" \
    && rm "$DEB.sha256" \
    \
    && dpkg -i "$DEB" \
    && rm "$DEB"

# Gettext
#
# Ubuntu 14.04 comes with gettext tools version 0.18, but we need at
# least 0.19 for the desktop file support in msgfmt.
#
# When removing the build directory, we ignore rm errors: the
# configure script tests long path support by creating a deeply nested
# chain of "confdir3" directories that rm cannot remove inside Docker
# due to apparmor/aufs restrictions. They can be fixed on the host
# side, but we don't want to require this from the user.
# https://github.com/moby/moby/issues/13451
# https://bugzilla.yoctoproject.org/show_bug.cgi?id=7338
RUN \
    VERSION="0.22" \
    && BASE="gettext-$VERSION" \
    && ARCHIVE="$BASE.tar.xz" \
    && ARCHIVE_SHA256="0e60393a47061567b46875b249b7d2788b092d6457d656145bb0e7e6a3e26d93" \
    && wget \
        --progress=dot:mega \
        "https://ftp.gnu.org/pub/gnu/gettext/$ARCHIVE" \
    \
    && echo "$ARCHIVE_SHA256 *$ARCHIVE" > "$ARCHIVE.sha256" \
    && sha256sum -c "$ARCHIVE.sha256" \
    && rm "$ARCHIVE.sha256" \
    \
    && tar -xf "$ARCHIVE" \
    && rm "$ARCHIVE" \
    && (cd "$BASE" \
        && ./configure \
            --enable-silent-rules \
            --disable-java \
            --disable-openmp \
            --without-emacs \
        && make -j$(nproc) \
        && make install-strip) \
    && rm -rf "/usr/local/share/doc/gettext/examples" \
    && (rm -rf "$BASE" || true)

# At-Spi2 for Qt (see comments below)
#
# We need at least 2.16 for ATSPI_STATE_READ_ONLY. 2.16 itself
# requires intltool even if NLS is explicitly disabled, so use 2.26,
# which is the last version with autotools support (so that we don't
# have to install meson for a newer version).
RUN \
    VERSION_ROOT="2.26" \
    && VERSION="$VERSION_ROOT.3" \
    && BASE="at-spi2-core-$VERSION"\
    && ARCHIVE="$BASE.tar.xz" \
    && ARCHIVE_SHA256="ebc9cdc4a1646c993735201426600c1f5432c694f95c69805ae16ad15065ccaf" \
    && wget \
        --progress=dot:mega \
        "https://download.gnome.org/sources/at-spi2-core/$VERSION_ROOT/$ARCHIVE" \
    \
    && echo "$ARCHIVE_SHA256 *$ARCHIVE" > "$ARCHIVE.sha256" \
    && sha256sum -c "$ARCHIVE.sha256" \
    && rm "$ARCHIVE.sha256" \
    \
    && tar -xf "$ARCHIVE" \
    && rm "$ARCHIVE" \
    && (cd "$BASE" \
        && ./configure \
            --enable-silent-rules \
            --disable-nls \
            --disable-x11 \
            --disable-gtk-doc-html \
        && make -j$(nproc) \
        && make install-strip) \
    && rm -rf "$BASE"

# This is the directory where we will put the license files of all
# manually built libraries. Each license should be in its own
# subdirectory named after the library.
ENV MIL_LICENSES_DIR_PATH="/usr/local/share/doc/manually-installed-lib-licenses"

# Define the Qt version.
#
# 5.12 is the last version of Qt the can be built on Ubuntu 14.04
# using the stock libraries:
#
# * 5.15 requires libxcb 1.11, while Ubuntu 14.04 comes with 1.10.
#
# * 5.13-14 requires xkbcommon/xkbcommon-compose.h, which is not
#   available in libxkbcommon-dev 0.4.1.
#
# * 5.12.4+ require the ATSPI_STATE_READ_ONLY constant introduced in
#   libatspi 2.16, while Ubuntu 14.04 comes with 2.10. Fortunately,
#   Qt doesn't actually link with libatspi, but only uses its
#   <atspi/atspi-constants.h> file, so we can install the library from
#   sources and build the latest version of Qt 5.12 (5.12.12).
ENV QT_VERSION_ROOT="5.12"
ENV QT_VERSION="$QT_VERSION_ROOT.12"

# Qt base
#
# Disabled features
# =================
#
# We try to disable unnecessary Qt features to minimize the bundle
# size. However, not all features can be disabled:
#
# * Disabling these features causes compilation errors:
#
#   cborstreamreader
#   datestring
#   datetimeparser
#   regularexpression
#   tabletevent
#   textcodec
#   textdate
#   timezone
#   xml*
#
# * Disabling filesystemwatcher results in empty file lists in file
#   dialogs.
#
# * Disabling imageformat_xpm removes the standard icons from message
#   boxes, and, most importantly, causes text fields to randomly
#   ignore text selection with the mouse.
#
#
# Patches
# =======
#
# x11-client-leader-window.patch fixes the bug that caused the
# application process in the task manager to be named "Qt Client
# Leader Window" instead of the executable name or window title.
# This was fixed in Qt 5.15.1; see
# https://bugreports.qt.io/browse/QTBUG-75526
COPY "include/qt-$QT_VERSION"*".patch" ./
RUN \
    BASE="qtbase-everywhere-src-$QT_VERSION" \
    && ARCHIVE="$BASE.tar.xz" \
    && ARCHIVE_SHA256="7c15c31590b6ec12408242b1a32075d1d545f35e1d41fdbad31ec670d00680f2" \
    && wget \
        --progress=dot:mega \
        "https://download.qt.io/archive/qt/$QT_VERSION_ROOT/$QT_VERSION/submodules/$ARCHIVE" \
    \
    && echo "$ARCHIVE_SHA256 *$ARCHIVE" > "$ARCHIVE.sha256" \
    && sha256sum -c "$ARCHIVE.sha256" \
    && rm "$ARCHIVE.sha256" \
    \
    && tar -xf "$ARCHIVE" \
    && rm "$ARCHIVE" \
    && (cd "$BASE" \
        && patch -p1 \
            -i "../qt-$QT_VERSION"*"-x11-client-leader-window.patch" \
        && ./configure \
            -silent \
            -opensource \
            -confirm-license \
            -release \
            -optimize-size \
            -strip \
            -nomake examples \
            -no-glib \
            -no-iconv \
            -no-icu \
            -qt-pcre \
            -system-zlib \
            -no-ssl \
            -no-system-proxies \
            -no-cups \
            -fontconfig \
            -system-freetype \
            -qt-harfbuzz \
            -no-gtk \
            -no-opengl \
            -no-egl \
            -no-eglfs \
            -qpa xcb \
            -no-directfb \
            -no-eglfs \
            -no-gbm \
            -no-kms \
            -no-linuxfb \
            -no-mirclient \
            -qt-xcb \
            -no-libudev \
            -no-evdev \
            -xcb-xinput \
            -xkbcommon \
            -no-gif \
            -no-ico \
            -system-libpng \
            -no-libjpeg \
            -no-sql-db2 \
            -no-sql-ibase \
            -no-sql-mysql \
            -no-sql-oci \
            -no-sql-odbc \
            -no-sql-psql \
            -no-sql-sqlite2 \
            -no-sql-sqlite \
            -no-sql-tds \
            \
            -no-feature-concurrent \
            -no-feature-network \
            -no-feature-sql \
            -no-feature-testlib \
            \
            -no-feature-animation \
            -no-feature-bearermanagement \
            -no-feature-big_codecs \
            -no-feature-calendarwidget \
            -no-feature-codecs \
            -no-feature-colordialog \
            -no-feature-colornames \
            -no-feature-commandlinkbutton \
            -no-feature-concurrent \
            -no-feature-datetimeedit \
            -no-feature-dial \
            -no-feature-dnslookup \
            -no-feature-dockwidget \
            -no-feature-dom \
            -no-feature-dtls \
            -no-feature-effects \
            -no-feature-fontcombobox \
            -no-feature-fontdialog \
            -no-feature-formlayout \
            -no-feature-ftp \
            -no-feature-future \
            -no-feature-gestures \
            -no-feature-graphicseffect \
            -no-feature-graphicsview \
            -no-feature-http \
            -no-feature-identityproxymodel \
            -no-feature-image_heuristic_mask \
            -no-feature-image_text \
            -no-feature-imageformat_bmp \
            -no-feature-imageformat_jpeg \
            -no-feature-imageformat_ppm \
            -no-feature-imageformatplugin \
            -no-feature-itemmodeltester \
            -no-feature-lcdnumber \
            -no-feature-listwidget \
            -no-feature-localserver \
            -no-feature-mdiarea \
            -no-feature-mimetype \
            -no-feature-movie \
            -no-feature-networkdiskcache \
            -no-feature-networkinterface \
            -no-feature-networkproxy \
            -no-feature-paint_debug \
            -no-feature-pdf \
            -no-feature-picture \
            -no-feature-printdialog \
            -no-feature-printer \
            -no-feature-printpreviewdialog \
            -no-feature-printpreviewwidget \
            -no-feature-process \
            -no-feature-resizehandler \
            -no-feature-rubberband \
            -no-feature-scroller \
            -no-feature-socks5 \
            -no-feature-splashscreen \
            -no-feature-sqlmodel \
            -no-feature-statemachine \
            -no-feature-style-stylesheet \
            -no-feature-syntaxhighlighter \
            -no-feature-tableview \
            -no-feature-tablewidget \
            -no-feature-textbrowser \
            -no-feature-textodfwriter \
            -no-feature-toolbox \
            -no-feature-topleveldomain \
            -no-feature-tuiotouch \
            -no-feature-udpsocket \
            -no-feature-undocommand \
            -no-feature-undogroup \
            -no-feature-undostack \
            -no-feature-undoview \
            -no-feature-whatsthis \
            -no-feature-wizard \
        && make -j$(nproc) \
        && make install) \
    \
    && mkdir -p "$MIL_LICENSES_DIR_PATH/qt5-base" \
    && cp \
        "$BASE/LICENSE.GPL3" "$BASE/LICENSE.LGPL3" \
        "$MIL_LICENSES_DIR_PATH/qt5-base" \
    \
    && rm -rf "$BASE" "qt-$QT_VERSION"*".patch"

ENV PATH="/usr/local/Qt-$QT_VERSION/bin:$PATH"
ENV CMAKE_PREFIX_PATH="/usr/local/Qt-$QT_VERSION"

# QT translations
#
# To compile translations, we need the lrelease tool. We can get it by
# building the qttools sumbodule, but it has tons of other tools which
# we can't disable with qmake during build time.
#
# Instead, we use the lrelease tool from Qt 5 shipped with Ubuntu
# 14.04. Its version is older that the one we compile, but it doesn't
# matter for lrelease. The total download size will be bigger than the
# source code of the quttools submodule (about 37.4 MB vs 9.4 MB), but
# we will save time by not having to wait for compilation.
RUN \
    export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y update \
    && apt-get -y install qttools5-dev-tools \
    \
    && BASE="qttranslations-everywhere-src-$QT_VERSION" \
    && ARCHIVE="$BASE.tar.xz" \
    && ARCHIVE_SHA256="b0f7f060ec5fd7b0d3de3df4db2f6c9e93ec70e0d2b5406d4d9024bc79da6023" \
    && wget \
        --progress=dot:mega \
        "https://download.qt.io/archive/qt/$QT_VERSION_ROOT/$QT_VERSION/submodules/$ARCHIVE" \
    \
    && echo "$ARCHIVE_SHA256 *$ARCHIVE" > "$ARCHIVE.sha256" \
    && sha256sum -c "$ARCHIVE.sha256" \
    && rm "$ARCHIVE.sha256" \
    \
    && tar -xf "$ARCHIVE" \
    && rm "$ARCHIVE" \
    \
    && "/usr/lib/x86_64-linux-gnu/qt5/bin/lrelease" \
        "$BASE/translations"/*".ts" \
    && mkdir -p "$(qmake -query QT_INSTALL_TRANSLATIONS)" \
    && cp \
        "$BASE/translations"/*".qm" \
        "$(qmake -query QT_INSTALL_TRANSLATIONS)" \
    \
    && mkdir -p "$MIL_LICENSES_DIR_PATH/qt5-translations" \
    && cp \
        "$BASE/LICENSE.GPL3-EXCEPT" \
        "$MIL_LICENSES_DIR_PATH/qt5-translations" \
    \
    && rm -rf "$BASE" \
    \
    && apt-get -y purge qttools5-dev-tools \
    && apt-get -y autoremove \
    && rm -rf "/var/lib/apt/lists"/*

# Leptonica
RUN \
    VERSION="1.83.1" \
    && BASE="leptonica-$VERSION" \
    && ARCHIVE="$BASE.tar.gz" \
    && ARCHIVE_SHA256="8f18615e0743af7df7f50985c730dfcf0c93548073d1f56621e4156a8b54d3dd" \
    && wget \
        --progress=dot:mega \
        "https://github.com/DanBloomberg/leptonica/releases/download/$VERSION/$ARCHIVE" \
    \
    && echo "$ARCHIVE_SHA256 *$ARCHIVE" > "$ARCHIVE.sha256" \
    && sha256sum -c "$ARCHIVE.sha256" \
    && rm "$ARCHIVE.sha256" \
    \
    && tar -xf "$ARCHIVE" \
    && rm "$ARCHIVE" \
    && (cd "$BASE" \
        && ./configure \
            CFLAGS=-DMINIMUM_SEVERITY=L_SEVERITY_NONE \
            --without-giflib \
            --without-jpeg \
            --without-libopenjpeg \
            --without-libpng \
            --without-libtiff \
            --without-libwebp \
            --without-libwebpmux \
            --without-zlib \
            --disable-programs \
        && make -j$(nproc) \
        && make install-strip) \
    \
    && mkdir -p "$MIL_LICENSES_DIR_PATH/leptonica" \
    && cp \
        "$BASE/leptonica-license.txt" \
        "$MIL_LICENSES_DIR_PATH/leptonica/LICENSE" \
    \
    && rm -rf "$BASE"

# Tesseract
RUN \
    VERSION="5.5.1" \
    && BASE="tesseract-$VERSION" \
    && ARCHIVE="$BASE.tar.gz" \
    && ARCHIVE_SHA256="a7a3f2a7420cb6a6a94d80c24163e183cf1d2f1bed2df3bbc397c81808a57237" \
    && wget \
        --progress=dot:mega \
        --output-document="$ARCHIVE" \
        "https://github.com/tesseract-ocr/tesseract/archive/refs/tags/$VERSION.tar.gz" \
    \
    && echo "$ARCHIVE_SHA256 *$ARCHIVE" > "$ARCHIVE.sha256" \
    && sha256sum -c "$ARCHIVE.sha256" \
    && rm "$ARCHIVE.sha256" \
    \
    && tar -xf "$ARCHIVE" \
    && rm "$ARCHIVE" \
    && (cd "$BASE" \
        && ./autogen.sh \
        && ./configure \
            --disable-doc \
            --disable-graphics \
            --disable-legacy \
            --disable-openmp \
            --without-archive \
            --without-curl \
        && make -j$(nproc) \
        && make install-strip) \
    \
    && mkdir -p "$MIL_LICENSES_DIR_PATH/tesseract" \
    && cp "$BASE/LICENSE" "$MIL_LICENSES_DIR_PATH/tesseract" \
    \
    && rm -rf "$BASE"

# Tesseract default data
RUN \
    VERSION="4.1.0" \
    && wget \
        --progress=dot:mega \
        --directory-prefix "/usr/local/share/tessdata" \
        "https://raw.githubusercontent.com/tesseract-ocr/tessdata_fast/$VERSION/eng.traineddata"

# Jansson
RUN \
    VERSION="2.14" \
    && BASE="jansson-$VERSION" \
    && ARCHIVE="$BASE.tar.bz2" \
    && ARCHIVE_SHA256="fba956f27c6ae56ce6dfd52fbf9d20254aad42821f74fa52f83957625294afb9" \
    && wget \
        --progress=dot:mega \
        "https://github.com/akheron/jansson/releases/download/v$VERSION/$ARCHIVE" \
    \
    && echo "$ARCHIVE_SHA256 *$ARCHIVE" > "$ARCHIVE.sha256" \
    && sha256sum -c "$ARCHIVE.sha256" \
    && rm "$ARCHIVE.sha256" \
    \
    && tar -xf "$ARCHIVE" \
    && rm "$ARCHIVE" \
    && (cd "$BASE" \
        && ./configure \
            --enable-silent-rules \
        && make -j$(nproc) \
        && make install-strip) \
    \
    && mkdir -p "$MIL_LICENSES_DIR_PATH/jansson" \
    && cp "$BASE/LICENSE" "$MIL_LICENSES_DIR_PATH/jansson" \
    \
    && rm -rf "$BASE"

# Add stb_image_resize2 license
RUN \
    wget \
        --directory-prefix "$MIL_LICENSES_DIR_PATH/stb_image_resize2" \
        "https://raw.githubusercontent.com/nothings/stb/master/LICENSE"

# Tell the linker about new libs in /usr/local/lib.
RUN ldconfig

COPY include/build-bundle.sh /usr/local/bin/build-bundle
RUN chmod +x /usr/local/bin/build-bundle

WORKDIR /workspace

CMD ["/bin/bash"]
