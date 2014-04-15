/*
 * This is the abstract base class for DMX devices. It provides a function to
 * open and close devices, read and write data and retrieve information about
 * connected FTDI USB devices.
 * The FtdiDevice instance is exposed only to subclasses; its functionality
 * relevant for external use is either forwarded here or has been declared
 * statically. In other words, from a user's point of view, the FtdiDevice
 * class' contents are not relevant.
 */
#include "DmxDevice.h"

/* NOTE: using a magic return value is not very elegant...oh well. */
const int DmxDevice::RV_DEVICE_NOT_OPEN = FtdiDevice::RV_DEVICE_NOT_OPEN;


DmxDevice::DmxDevice()
: ftdiDevice_( 0 )
{ /* empty */ }

DmxDevice::~DmxDevice()
{
	delete ftdiDevice_;
}


/*
 * Open a connected (USB-based) FTDI device with the given index.
 *
 * Returns: true if opening was successful or it was already open or false if
 * opening failed (getLastError() might provide details in this case).
 */
bool DmxDevice::open( const char* description, const char* serial, int index )
{
	bool success = true;
	
	if ( ftdiDevice_ == 0 || ! ftdiDevice_->isOpen() ) {
		ftdiDevice_ = new FtdiDevice();
		success = ftdiDevice_->open( description, serial, index );
		if ( success ) ftdiDevice_->purgeBuffers( FtdiDevice::RX_TX_BUFFER );
	}
	
	return success;
}

/*
 * Close the previously opened device.
 *
 * Returns: true if device has been closed successfully or if it was not open,
 * false otherwise (getLastError() might provide details in this case).
 */
bool DmxDevice::close()
{
	bool success = true;
	if ( isOpen() ) success = ftdiDevice_->close();
	return success;
}

/*
 * Return a flag indicating whether the device is currently open.
 */
bool DmxDevice::isOpen() const
{
	return ftdiDevice_ != 0 && ftdiDevice_->isOpen();
}


/************************
 * forwarding functions *
 ************************/

/*
 * Return the last error raised by libftdi.
 * Note: if a non-ftdi related error occured afterwards, the last libftdi error
 * will not be returned anymore so it is best to call this function as soon as
 * possible after a problem occured.
 *
 * Returns: the last libftdi error or an empty string if the libftdi error is outdated.
 */
const char* DmxDevice::getLastError() const
{ return ftdiDevice_->getLastError(); }

/*
 * Return some information on the USB device itself to which the FTDI-device is
 * connected.
 *
 * Returns: a usbInformation struct containing information or NULL if such
 * information could not be retrieved when the device was opened.
 */
const struct FtdiDevice::usbInformation* DmxDevice::getUsbInformation() const
{ return ftdiDevice_->getUsbInformation(); }

/*
 * Reset the FTDI device and return the call result.
 */
int DmxDevice::reset()
{ return ftdiDevice_->reset(); }

