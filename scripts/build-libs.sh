#!/bin/sh

#NOTES
# - to get fat binaries: http://www.mail-archive.com/libusbx-devel@lists.sourceforge.net/msg00442.html
#
#TODO
# - fix output logging (i.e., redirection) is not working properly for stderr and some commands (copying etc.)
# - make sure libftdi looks for libusb-config in the right place (maybe clear the corresponding hash? is the path order correct?)
# - find a way to avoid the warning from libftdi's configure script about missing Python.h on linux

# detect OS
OS_NAME=`uname -s`
if [ "x$OS_NAME" = "xDarwin" ]; then
	OS_NAME=osx
elif [ "x$OS_NAME" = "xLinux" ]; then
	OS_NAME=linux
else
	OS_NAME=""
fi

#initialize variables (the only things you should change are the ..._VERSION and ..._DL_URL variables)

#NOTE: this patch removes a redefenition error in libusb-compat introduced by libusbx.
#      according to this ticket (http://sourceforge.net/apps/trac/libusbx/ticket/31),
#      this will be fixed in libusbx 1.0.13.
COMPAT_X_PATCH=libusb-compat-0.1.4_libusbx.patch

# *** Extra compiler flags
#NOTE: ARCH_SPECS will be emptied on all OSes but OSX
if [ $OS_NAME = osx ]; then
	ARCH_SPECS="-arch ppc -arch i386 -arch x86_64"
else
	ARCH_SPECS=""
fi

EXTRA_CFLAGS=
EXTRA_LDFLAGS=

BASE_DIR=`pwd`/build-output
BUILD_DIR=$BASE_DIR/build
RESULT_DIR=$BASE_DIR/result
LOG_FILE=$BASE_DIR/build.log

USBX_VERSION=1.0.12
USBX_BASE_NAME=libusbx-$USBX_VERSION
USBX_ARCH_NAME=$USBX_BASE_NAME.tar.bz2
USBX_DL_URL="http://downloads.sourceforge.net/project/libusbx/releases/$USBX_VERSION/source/$USBX_ARCH_NAME"

USB_COMPAT_VERSION=0.1.4
USB_COMPAT_BASE_NAME=libusb-compat-$USB_COMPAT_VERSION
USB_COMPAT_ARCH_NAME=$USB_COMPAT_BASE_NAME.tar.bz2
USB_COMPAT_DL_URL="http://downloads.sourceforge.net/project/libusb/libusb-compat-0.1/$USB_COMPAT_BASE_NAME/$USB_COMPAT_ARCH_NAME"

FTDI_VERSION=0.20
FTDI_BASE_NAME=libftdi-$FTDI_VERSION
FTDI_ARCH_NAME=$FTDI_BASE_NAME.tar.gz
FTDI_DL_URL="http://www.intra2net.com/en/developer/libftdi/download/$FTDI_ARCH_NAME"


download_if_necessary() {
	FILE_NAME="$1"
	URL="$2"

	if [ ! -f "$FILE_NAME" ]; then
		echo "*** Downloading $FILE_NAME from $URL ***"
		wget "$URL" -O "$FILE_NAME" -a $LOG_FILE
	fi
}

exit_on_error() {
	if [ $? -ne 0 ]; then
		if [ $# -gt 0 ]; then
			ERRMSG=" ($1)"
		fi
		echo "An error occurred$ERRMSG, please review the log ($BUILD_LOG/$LOG_FILE)"
		exit 1
	fi
}



### handle command-line arguments
KEEP_INTERMEDIATES=no
if [ $# -gt 0 ]; then
	case $1 in
	-h)
		echo "This script is meant to compile the libraries needed by ofxGenericDmx."
		echo "The following options can be used:"
		echo "\t-h\tshow this help message"
		echo "\t-k\tkeep all intermediate files"
		exit 0
		;;
	-k)
		KEEP_INTERMEDIATES=yes
		;;
	*)
		echo "$0: Unknown arguments given, try running '$0 -h'"
		exit 1
		;;
	esac
fi


### prepare building
mkdir -p $BASE_DIR
cd $BASE_DIR
mkdir -p $BUILD_DIR
rm -f $LOG_FILE
>$LOG_FILE

if [ "x$OS_NAME" = "x" ]; then
	echo "!!! Error: unknown OS (`uname -s`)"
	exit 1
fi

echo "*** Building for OS: $OS_NAME ***" 2>&1 | tee -a $LOG_FILE
download_if_necessary $USBX_ARCH_NAME $USBX_DL_URL
download_if_necessary $USB_COMPAT_ARCH_NAME $USB_COMPAT_DL_URL
download_if_necessary $FTDI_ARCH_NAME $FTDI_DL_URL

tar xjf $USBX_ARCH_NAME 2>&1 > /dev/null
tar xjf $USB_COMPAT_ARCH_NAME 2>&1 > /dev/null
tar xzf $FTDI_ARCH_NAME 2>&1 > /dev/null
echo >> $LOG_FILE

cd $BASE_DIR

### build libusbx
echo "*** Building library $USBX_BASE_NAME ***" 2>&1 | tee -a $LOG_FILE
cd $USBX_BASE_NAME
CFLAGS="$CFLAGS $EXTRA_CFLAGS $ARCH_SPECS" LDFLAGS="$LDFLAGS $EXTRA_LDFLAGS" ./configure --prefix=$BUILD_DIR --disable-shared --disable-dependency-tracking 2>&1 >> $LOG_FILE
exit_on_error "could not configure $USBX_BASE_NAME"
make all install 2>&1 >> $LOG_FILE
exit_on_error "could not build $USBX_BASE_NAME"
cd ..
echo >> $LOG_FILE


### build libusb-compat
echo "*** Building library $USB_COMPAT_BASE_NAME ***" 2>&1 | tee -a $LOG_FILE
cd $USB_COMPAT_BASE_NAME
patch -uNp1 < $BASE_DIR/../$COMPAT_X_PATCH 2>&1 >> $LOG_FILE
exit_on_error "could not patch $_USB_COMPAT_BASE_NAME"
CFLAGS="$CFLAGS $EXTRA_CFLAGS $ARCH_SPECS" LDFLAGS="$LDFLAGS $EXTRA_LDFLAGS" \
	LIBUSB_1_0_CFLAGS="-I$BUILD_DIR/include -I$BUILD_DIR/include/libusb-1.0" LIBUSB_1_0_LIBS=-L$BUILD_DIR/lib \
	./configure --prefix=$BUILD_DIR --disable-shared --disable-dependency-tracking 2>&1 >> $LOG_FILE
exit_on_error "could not configure $USB_COMPAT_BASE_NAME"
make all install 2>&1 >> $LOG_FILE
exit_on_error "could not build $USB_COMPAT_BASE_NAME"
cd ..
echo >> $LOG_FILE


### build libftdi
echo "*** Building library $FTDI_BASE_NAME ***" 2>&1 | tee -a $LOG_FILE
cd $FTDI_BASE_NAME
PATH="$BUILD_DIR/bin:$PATH" CFLAGS="$CFLAGS $EXTRA_CFLAGS $ARCH_SPECS -I$BUILD_DIR/include" LDFLAGS="$LDFLAGS $EXTRA_LDFLAGS" \
	./configure --prefix=$BUILD_DIR --disable-shared --disable-dependency-tracking --disable-libftdipp --disable-python-binding --without-examples --without-docs 2>&1 >> $LOG_FILE
exit_on_error "could not configure $FTDI_BASE_NAME"
make all install 2>&1 >> $LOG_FILE
exit_on_error "could not build $FTDI_BASE_NAME"
cd ..
echo >> $LOG_FILE



### copy files to target directory structure
echo "*** Copying relevant files ***" 2>&1 | tee -a $LOG_FILE
mkdir -p $RESULT_DIR/libusbx/include $RESULT_DIR/libusbx/lib/$OS_NAME
mkdir -p $RESULT_DIR/libusb-compat/include $RESULT_DIR/libusb-compat/lib/$OS_NAME
mkdir -p $RESULT_DIR/libftdi/include $RESULT_DIR/libftdi/lib/$OS_NAME
cp -R $BUILD_DIR/include/libusb-1.0 $RESULT_DIR/libusbx/include
cp -R $BUILD_DIR/include/usb.h $RESULT_DIR/libusb-compat/include
cp -R $BUILD_DIR/include/ftdi.h $RESULT_DIR/libftdi/include
cp -R $BUILD_DIR/lib/libusb-1.0.a $RESULT_DIR/libusbx/lib/$OS_NAME
cp -R $BUILD_DIR/lib/libusb.a $RESULT_DIR/libusb-compat/lib/$OS_NAME
cp -R $BUILD_DIR/lib/libftdi.a $RESULT_DIR/libftdi/lib/$OS_NAME
echo >> $LOG_FILE



### clean up
echo "*** Cleaning up ***" 2>&1 | tee -a $LOG_FILE
if [ "x$KEEP_INTERMEDIATES" = "xno" ]; then
	rm -R $USBX_BASE_NAME
	rm -R $USB_COMPAT_BASE_NAME
	rm -R $FTDI_BASE_NAME
	rm -R $BUILD_DIR
else
	echo "*** Leaving all intermediate files in place."
fi
echo >> $LOG_FILE

### inform user of further actions
echo "The libraries have been built and copied into a newly created directory structure which is compatible with the ofxGenericDmx addon." 2>&1 | tee -a $LOG_FILE
echo "Please check if all files are present (libraries + headers for libusbx, libusb-compat and libftdi) and copy them to the addon." 2>&1 | tee -a $LOG_FILE
echo "Be careful not to accidentally overwrite w32/linux files." 2>&1 | tee -a $LOG_FILE
echo >> $LOG_FILE
