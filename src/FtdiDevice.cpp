/*
 * TODO:
 * - (open) Implement retry mechanism for opening device with 750/1000 ms in between?
 * - (open) add variant wrapping ftdi_usb_open_string
 * - (getDeviceList) generate warning if fetchUsbInformation() returns null?
 */
#include <unistd.h> /* for usleep() */
#include <sys/time.h> /* for gettimeofday and related macros */
#include "FtdiDevice.h"

/* public constants */
const int FtdiDevice::RV_DEVICE_NOT_OPEN = -19999;

/* private (constant) statics */
const int FtdiDevice::USB_VENDOR_ID = 0x0403;
const int FtdiDevice::USB_PRODUCT_ID = 0x6001;
const int FtdiDevice::USB_INFO_FIELD_LENGTH = 256;
FtdiDevice::vec_deviceInfo* FtdiDevice::s_deviceList = 0;
ftdi_device_list* FtdiDevice::s_ftdiDeviceList = 0;


FtdiDevice::FtdiDevice()
: context_( 0 ), usbInfo_( 0 ), hasFtdiError_( false )
{}

FtdiDevice::~FtdiDevice()
{
	if ( isOpen() ) close();
}


/*
 * Open an FTDI device. The device is selected with regard to the given restrictions;
 * any of those may be NULL and index is counted in the subset selected by the
 * description and/or serial if given.
 *
 * Returns: true if successfully opened, false otherwise.
 */
bool FtdiDevice::open( const char* description, const char* serial, int index )
{
	hasFtdiError_ = false;
	
	if ( isOpen() ) {
		hasFtdiError_ = false;
		return false;
	}
	
	context_ = ftdi_new();
	if ( context_ == 0 ) {
		hasFtdiError_ = true;
		return false;
	}
	
	bool success = false;
	vec_deviceInfo* devs = const_cast<vec_deviceInfo*>( getDeviceList( context_ ) );
	
	vec_deviceInfo::iterator it;
	
	if ( devs != 0 ) {
		for ( it = devs->begin(); it != devs->end(); ++it ) {
			const deviceInfo& di = *it;
			
			if ( description != 0 || serial != 0 ) {
				if ( di.usbInfo == 0 )
					break;
				
				if ( description != 0 && std::strncmp( di.usbInfo->description, description,
																							std::strlen( description ) ) != 0 )
					continue;
				
				if ( serial != 0 && std::strncmp( di.usbInfo->serial, serial,
																				 std::strlen( serial ) ) != 0 )
					continue;
			}
			
			if ( index-- != 0 )
				continue;
			
			//If control gets here, we have found a suitable device.
			if ( ftdi_usb_open_dev( context_, di.ftdiDevice ) < 0 ) {
				//do not free context since that would also free the error message
				hasFtdiError_ = true;
			}
			usbInfo_ = new usbInformation( *( di.usbInfo ) );
			success = true;
		}
	} else {
		hasFtdiError_ = ( devs == 0 );
	}
	
	if ( ! success && ! hasFtdiError_ ) {
		delete context_; context_ = 0;
	}
	
	if ( success ) {
		purgeBuffers();
		reset();
	}
	
	return success;
}


bool FtdiDevice::close()
{
	bool success = true;
	
	if ( isOpen() ) {
		purgeBuffers();
		int r = ftdi_usb_close( context_ );
		if ( r < 0 ) {
			success = false;
			hasFtdiError_ = true;
		} else {
			hasFtdiError_ = false;
		}
		
		ftdi_free( context_ );
		context_ = 0;
	}
	
	delete usbInfo_; usbInfo_ = 0;
	
	return success;
}

int FtdiDevice::setBaudRate( int baudRate ) const
{
	if ( context_ == 0 ) return RV_DEVICE_NOT_OPEN;
	int r = ftdi_set_baudrate( context_, baudRate );
	return ( r < 0 ) ? false : true;
}

int FtdiDevice::setLineProperties( FTDI_DATABITS_TYPE dataBits, FTDI_STOPBITS_TYPE stopBits,
											 FTDI_PARITY_TYPE parity, FTDI_BREAK_TYPE breakType ) const
{
	if ( context_ == 0 ) return RV_DEVICE_NOT_OPEN;
	int r = ftdi_set_line_property2( context_, (ftdi_bits_type)dataBits,
																	(ftdi_stopbits_type)stopBits, (ftdi_parity_type)parity,
																	(ftdi_break_type)breakType );
	return ( r < 0 ) ? false : true;
}

int FtdiDevice::setFlowControl( FTDI_FLOWCTL_TYPE flowCtl ) const
{
	if ( context_ == 0 ) return RV_DEVICE_NOT_OPEN;
	int r = ftdi_setflowctrl( context_, flowCtl );
	return ( r < 0 ) ? false : true;
}


bool FtdiDevice::isOpen() const
{
	return ( context_ != 0 );
}

const char* FtdiDevice::getLastError() const
{
	if ( context_ != 0 && hasFtdiError_ ) {
		return ftdi_get_error_string( context_ );
	} else {
			return "";
	}
}

const struct FtdiDevice::usbInformation* FtdiDevice::getUsbInformation() const
{
	return isOpen() ? usbInfo_ : 0;
}


/*
 * Attempts to read the requested number of bytes into the given buffer from the
 * device, optionally with the given timeout in milliseconds.
 *
 * Returns: the total number of bytes read, DEVICE_NOT_OPEN if the ftdi device
 * is not open, or another value < 0 representing an error code from ftdi_read_data.
 */
/* FIXME: trouble compiling gettimeofday() in windows? add this:
 * http://www.suacommunity.com/dictionary/gettimeofday-entry.php
 */
int FtdiDevice::readData( const unsigned char* data, int length, int timeout ) const
{
	//fprintf( stderr, "readData: about to read %i bytes.\n", length ); //LOG
	
	if ( ! isOpen() ) return RV_DEVICE_NOT_OPEN;
	struct timeval tNow, tEnd, tOut;
	
	tOut.tv_sec = timeout / 1000; tOut.tv_usec = ( timeout % 1000 ) * 1000;
	gettimeofday( &tNow, 0 );
	timeradd( &tNow, &tOut, &tEnd );
	
	int readTotal = 0, readLast = 0;
	bool reread = ( length > 0 );
	
	while ( reread ) {
		readTotal += readLast;
		if ( readTotal == length ) break;
		readLast = ftdi_read_data( context_,
															const_cast<unsigned char*>( data ) + readTotal,
															length - readTotal );
		
		gettimeofday( &tNow, 0 );
		reread = ( readLast >= 0 );
		if ( reread ) reread = ( readLast > 0 || timercmp( &tNow, &tEnd, < ) );
	}
	
	return readLast < 0 ? readLast : readTotal;
}

/*
 * Attempts to write the given buffer of given length to the device.
 *
 * Returns: the total number of bytes written, DEVICE_NOT_OPEN if the ftdi device
 * is not open, or another value < 0 representing an error code from ftdi_write_data.
 */
int FtdiDevice::writeData( const unsigned char* data, int length ) const
{
	//fprintf( stderr, "writeData: about to write %i bytes.\n", length ); //LOG
	
	if ( ! isOpen() ) return RV_DEVICE_NOT_OPEN;
	
	return ftdi_write_data( context_, const_cast<unsigned char*>( data ), length );
}

int FtdiDevice::purgeBuffers( int bufType ) const
{
	if ( ! isOpen() ) return RV_DEVICE_NOT_OPEN;
	
	int rv;
	switch ( bufType ) {
		case RX_BUFFER: rv = ftdi_usb_purge_rx_buffer( context_ ); break;
		case TX_BUFFER: rv = ftdi_usb_purge_tx_buffer( context_ ); break;
		case RX_TX_BUFFER: rv = ftdi_usb_purge_buffers( context_ ); break;
		default: assert( false ); rv = 0; break; //illegal argument given
	}
	
	//if ( rv < 0 ) fprintf( stderr, "purgeBuffers: purging failed (%i)\n", rv ); //LOG
	
	return rv;
}

int FtdiDevice::reset() const
{
	return ( context_ != 0 ) ? ftdi_usb_reset( context_ ) : 0;
}



/* PUBLIC STATIC FUNCTIONS */

/*
 * Return a list of devices found by libftdi together with a number of fields
 * describing the corresponding USB device.
 * The list is internally cached. To free it, call freeDeviceList().
 *
 * Returns: a vector pointer containing 0 or more devices, or NULL if an error
 * occured while retrieving the list. When NULL is returned, this is guaranteed
 * to be an ftdi error.
 */
const FtdiDevice::vec_deviceInfo* FtdiDevice::getDeviceList( ftdi_context* c )
{
	ftdi_context* context = c ? c : ftdi_new();
	if ( context == 0 ) return 0;
	
	if ( s_deviceList != 0 ) freeDeviceList();
	
	int r = ftdi_usb_find_all( context, &s_ftdiDeviceList, USB_VENDOR_ID, USB_PRODUCT_ID );
	s_deviceList = new vec_deviceInfo();
	if ( r >= 0 ) {
		struct ftdi_device_list* openDev = s_ftdiDeviceList;
		while ( openDev ) {
			struct deviceInfo info;
			info.ftdiDevice = openDev->dev;
			info.usbInfo = fetchUsbInformation( context, info.ftdiDevice );
			s_deviceList->push_back( info );
			
			openDev = openDev->next;
		}
	}
	
	//Only free the context if it has been allocated locally.
	if ( c == 0 ) ftdi_free( context );
	return s_deviceList;
}

/*
 * Free the internally cached device list if it is allocated.
 */
const void FtdiDevice::freeDeviceList()
{
	vec_deviceInfo::iterator it;
	for ( it = s_deviceList->begin(); it != s_deviceList->end(); ++it ) delete (*it).usbInfo;
	
	delete s_deviceList;
	s_deviceList = 0;
	
	if ( s_ftdiDeviceList ) {
		ftdi_list_free( &s_ftdiDeviceList );
		s_ftdiDeviceList = 0;
	}
}


/* PRIVATE FUNCTIONS */

struct FtdiDevice::usbInformation* FtdiDevice::fetchUsbInformation( ftdi_context* context, struct usb_device* dev )
{
	struct usbInformation* info = new struct usbInformation();
	
	int r = ftdi_usb_get_strings( context, dev,
															 info->manufacturer, USB_INFO_FIELD_LENGTH,
															 info->description, USB_INFO_FIELD_LENGTH,
															 info->serial, USB_INFO_FIELD_LENGTH );
	
	if ( r < 0 ) {
		delete info; info = 0;
	}
	
	return info;
}
