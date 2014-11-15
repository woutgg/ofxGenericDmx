/*
 * TODO:
 * - force sending a minimum of 25 channels? (by padding 0-valued channels)
 *   this may be necessary since the API mentions a mininum size of 25 bytes on page 5
 * - purging buffers in open() seems not to work since an invalid header is
 *   (often) received anyway after just having killed the process previously.
 *
 * FIXME:
 * - writing user configuration data somehow seems to write at most 256 bytes,
 *   any more are always read out as 0xff. strangely enough it did work before.
 *   Where is the bug?
 */
#include <assert.h>
#include <cstring>
#include <iostream> /* TEMP: for user configuration bug warnings */
#include <math.h> /* for lroundf() */
#include <unistd.h> /* for usleep() */
#include "DmxDevice.h"
#include "DmxUsbProDevice.h"

//public constants
const unsigned int DmxUsbProDevice::SN_NOT_PROGRAMMED = 0xFFFFFFFF;
const unsigned int DmxUsbProDevice::BREAK_TIME_UNITS_MIN = 9;
const unsigned int DmxUsbProDevice::BREAK_TIME_UNITS_MAX = 127;
const unsigned int DmxUsbProDevice::MAB_TIME_UNITS_MIN = 1;
const unsigned int DmxUsbProDevice::MAB_TIME_UNITS_MAX = 127;
const unsigned int DmxUsbProDevice::OUTPUT_RATE_MAX = 40;
const unsigned int DmxUsbProDevice::USER_CONFIG_MAX_LENGTH = 508;
const char* DmxUsbProDevice::USB_DESCRIPTION = "DMX USB PRO";

const int DmxUsbProDevice::RV_PACKET_TOO_LONG = -18000;
const int DmxUsbProDevice::RV_PACKET_SHORT_READ = -18001;
const int DmxUsbProDevice::RV_PACKET_INVALID = -18002;
const int DmxUsbProDevice::RV_PACKET_NO_MATCH = -18003;
const int DmxUsbProDevice::RV_PACKET_SHORT_WRITE = -18004;

//private constants
const float DmxUsbProDevice::BREAK_TIME_UNIT = 10.67f;
const float DmxUsbProDevice::MAB_TIME_UNIT = 10.67f;
const unsigned char DmxUsbProDevice::PACKET_START_CODE = 0x7E;
const unsigned char DmxUsbProDevice::PACKET_END_CODE = 0xE7;
const unsigned int DmxUsbProDevice::PACKET_MAX_DATA_SIZE = 600;

const int DmxUsbProDevice::REQUEST_REPLY_DELAY = 5; /* in milliseconds */


DmxUsbProDevice::DmxUsbProDevice()
: widgetParams_( 0 ), userConfigData_( 0 ), serialNumber_( 0 )
{ /* empty */ }

DmxUsbProDevice::~DmxUsbProDevice()
{
	delete widgetParams_;
	delete userConfigData_;
	delete serialNumber_;
}


int DmxUsbProDevice::writeDmx( const unsigned char* data, int length ) const
{
	assert( length <= 513 );
	return sendUsbProPacket( SET_DMX_TX_MODE, data, length );
}

DmxDevice::DMX_DEVICE_TYPE DmxUsbProDevice::getType() const
{
	return DmxDevice::DMX_DEVICE_ENTTECPRO;
}


/*
 * Attempt to set widget parameters, passing any user configuration data as a
 * vector.
 *
 * Returns true if the data has been written successfully, false otherwise.
 */
bool DmxUsbProDevice::setWidgetParameters( const widgetParameters* params,
																					const vec_uchar* userConfigData ) const
{
	return setWidgetParameters( params, reinterpret_cast<const unsigned char*>( &userConfigData[0] ), userConfigData->size() );
}

/*
 * Attempt to set the given widget parameters on the device, passing any user
 * configuration data as an unsigned char*.
 *
 * Returns true if the data has been written successfully, false otherwise.
 */
bool DmxUsbProDevice::setWidgetParameters( const widgetParameters* params,
																					const unsigned char* userConfigData,
																					unsigned int userConfigDataLength ) const
{
	if ( ! isOpen() || params == 0 ) return false;
	if ( userConfigDataLength > USER_CONFIG_MAX_LENGTH ) return false;
	
	//TEMP: warn user about user configuration size bug
	if ( userConfigDataLength > 256 ) {
		std::cerr << "in " << __FUNCTION__ << "() "
		<< "request for setting more than 256 bytes of user configuration data received; "
		<< "due to a strange bug, only the first 256 will probably be set." << std::endl;
		//std::cerr << "  (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;
	}
	
	unsigned int btUnits = lroundf( params->breakTime / BREAK_TIME_UNIT );
	unsigned int mabUnits = lroundf( params->mabTime / MAB_TIME_UNIT );
	
	if ( btUnits < BREAK_TIME_UNITS_MIN || btUnits > BREAK_TIME_UNITS_MAX ) return false;
	if ( mabUnits < MAB_TIME_UNITS_MIN || mabUnits > MAB_TIME_UNITS_MAX ) return false;
	if ( params->refreshRate > OUTPUT_RATE_MAX ) return false;
	
	DMXUSBPROSetParamsType setParams;
	setParams.userSizeLSB = userConfigDataLength & 0xFF;
	setParams.userSizeMSB = ( userConfigDataLength >> 8 ) & 0xFF;
	setParams.breakTime = btUnits;
	setParams.maBTime = mabUnits;
	setParams.refreshRate = params->refreshRate;
	
	bool success = true;
	int r;
	
	unsigned char* buf = new unsigned char[sizeof( setParams ) + userConfigDataLength];
	std::memcpy( buf, &setParams, sizeof( setParams ) );
	if ( userConfigDataLength > 0 ) {
		std::memcpy( buf + sizeof( setParams ), userConfigData, userConfigDataLength );
	}
	
	r = sendUsbProPacket( SET_WIDGET_PARAMS_RQ, buf,
											 sizeof( setParams ) + userConfigDataLength );
	success = ( r >= 0 );
	
	delete[] buf;
	
	if ( success && widgetParams_ != 0 ) {
		widgetParams_->breakTime = setParams.breakTime * BREAK_TIME_UNIT;
		widgetParams_->mabTime = setParams.maBTime * MAB_TIME_UNIT;
		widgetParams_->refreshRate = setParams.refreshRate;
	}
	
	return success;
}

/*
 * Attempt to fetch widget parameters, optionally including user configuration
 * data (with a maximum of USER_CONFIG_MAX_LENGTH bytes) and the serial number
 * from the device. Use getWidgetParameters(), getUserConfigurationData() and
 * getSerial() to retrieve the resulting data.
 *
 * Returns: true if all data has been fetched successfully, false otherwise.
 */
bool DmxUsbProDevice::fetchExtendedInfo( unsigned int userConfigLength ) const
{
	bool success = true;
	success = fetchWidgetParameters( userConfigLength ) ? success : false;
	success = fetchSerialNumber() ? success : false;
	return success;
}

const DmxUsbProDevice::widgetParameters* DmxUsbProDevice::getWidgetParameters() const
{ return widgetParams_; }

/*
 * Returns the cached user configuration data if it has been retrieved by
 * fetchExtendedInfo().
 */
const DmxUsbProDevice::vec_uchar* DmxUsbProDevice::getUserConfigurationData() const
{ return userConfigData_; }

/*
 * Returns the cached serial number of the device. Call fetchExtendedInfo()
 * first to have the data retrieved from the device.
 * The returned number is what the device replied with (which will be
 * SN_NOT_PROGRAMMED in case it does not have a serial number programmed into
 * it), or NULL if fetchExtendedInfo() has not been called or could not fetch
 * the number.
 */
const uint32_t* DmxUsbProDevice::getSerialNumber() const
{ return serialNumber_; }


/*********************
 * PRIVATE FUNCTIONS *
 *********************/

bool DmxUsbProDevice::fetchWidgetParameters( unsigned int userConfigLength ) const
{
	bool success = false;
	
	if ( ! isOpen() ) return false;
	if ( widgetParams_ != 0 && userConfigLength == 0 ) return true;
	
	if ( userConfigLength > USER_CONFIG_MAX_LENGTH ) userConfigLength = USER_CONFIG_MAX_LENGTH;
	
	//TEMP: warn user about user configuration size bug
	if ( userConfigLength > 256 ) {
		std::cerr << "in " << __FUNCTION__ << "() "
		<< "request for reading more than 256 bytes of user configuration data received; "
		<< "due to a strange bug, only the first 256 will probably be read." << std::endl;
		//std::cerr << "  (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;
	}
	
	int r;
	unsigned char reqParams[2] = {
		userConfigLength & 0xFF,
		( userConfigLength >> 8	) & 0xFF
	};
	int replyLen = sizeof( DMXUSBPROParamsType ) + userConfigLength;
	unsigned char* replyBuffer = new unsigned char[replyLen];
	
	r = sendUsbProPacket( GET_WIDGET_PARAMS_RQ, reqParams, sizeof( reqParams ) );
	if ( r >= 0 ) {
		usleep( REQUEST_REPLY_DELAY * 1000 );
		r = receiveUsbProPacket( GET_WIDGET_PARAMS_REPLY, replyBuffer, replyLen );
		if ( r >= 0 ) {
			DMXUSBPROParamsType* pData = static_cast<DMXUSBPROParamsType*>( (void*)replyBuffer );
			if ( widgetParams_ != 0 ) {
				delete widgetParams_; widgetParams_ = 0;
			}
			widgetParams_ = new widgetParameters();
			//FIXME: is this interpretation of the firmware version correct?
			widgetParams_->firmwareVersionMajor = pData->firmwareMSB;
			widgetParams_->firmwareVersionMajor = pData->firmwareLSB;
			
			widgetParams_->breakTime = pData->breakTime * BREAK_TIME_UNIT;
			widgetParams_->mabTime = pData->maBTime * MAB_TIME_UNIT;
			widgetParams_->refreshRate = pData->refreshRate;
			
			if ( userConfigLength > 0 ) {
				unsigned char* ucd = replyBuffer + sizeof( DMXUSBPROParamsType );
				if ( userConfigData_ != 0 ) {
					delete userConfigData_; userConfigData_ = 0;
				}
				userConfigData_ = new vec_uchar( userConfigLength );
				std::memcpy( &(*userConfigData_)[0], ucd, userConfigLength );
			}
			
			success = true;
		}
	}
	delete[] replyBuffer;
	
	return success;
}

bool DmxUsbProDevice::fetchSerialNumber() const
{
	if ( ! isOpen() ) return false;
	if ( serialNumber_ != 0 ) return true;
	
	bool success = false;
	int r;
	unsigned char serialNum[4];
	
	r = sendUsbProPacket( GET_WIDGET_SN_RQ, 0, 0 );
	if ( r >= 0 ) {
		usleep( REQUEST_REPLY_DELAY * 1000 );
		r = receiveUsbProPacket( GET_WIDGET_SN_REPLY, serialNum, sizeof( serialNum ) );
		if ( r >= 0 ) {
			//FIXME: I'm not completely sure if the snAdded code is correct and especially unsure about the BCD code.
			serialNumber_ = new uint32_t( 0 );
			uint32_t snAdded = serialNum[0] + serialNum[1] * 0x100 + serialNum[2] * 0x10000 + serialNum[3] * 0x1000000;
			if ( snAdded != SN_NOT_PROGRAMMED ) {
				//NOTE: to display the number 'correctly' pad it to 8 characters with leading zeroes
				*serialNumber_ += ( serialNum[0] & 0xF ) + ( serialNum[0] >> 4 ) * 10;
				*serialNumber_ += ( ( serialNum[1] & 0xF ) + ( serialNum[1] >> 4 ) * 10 ) * 100;
				*serialNumber_ += ( ( serialNum[2] & 0xF ) + ( serialNum[2] >> 4 ) * 10 ) * 10000;
				*serialNumber_ += ( ( serialNum[3] & 0xF ) + ( serialNum[3] >> 4 ) * 10 ) * 1000000;
			} else {
				*serialNumber_ = SN_NOT_PROGRAMMED;
			}
			success = true;
		}
	}
	
	return success;
}


/*
 * Attempts to read the requested number of bytes into the given buffer.
 *
 * Returns: 0 if the packet has been read successfully, or < 0 if an error occured.
 * If the device is not open, DmxDevice::DEVICE_NOT_OPEN is returned.
 */
int DmxUsbProDevice::receiveUsbProPacket( int label, const unsigned char* data, unsigned int length ) const
{
	//fprintf( stderr, "receiveUsbProPacket: about to read payload of %i bytes.\n", length ); //LOG
	
	int result, r;
	unsigned char ftdi_header[4];
	//NOTE: the timeout is long but this should not be a problem as long as not too much data is requested.
	static const unsigned int READ_TIMEOUT = 10000;
	
	if ( ! isOpen() ) return DmxDevice::RV_DEVICE_NOT_OPEN;
	if ( length > PACKET_MAX_DATA_SIZE ) return RV_PACKET_TOO_LONG;
	
	r = ftdiDevice_->readData( ftdi_header, sizeof( ftdi_header ) / sizeof( unsigned char ), READ_TIMEOUT );
	
	if ( r < 0 ) return r;
	if ( r < (int)sizeof( ftdi_header ) ) return RV_PACKET_SHORT_READ;
	
	if ( ftdi_header[0] != PACKET_START_CODE ) {
		ftdiDevice_->purgeBuffers( FtdiDevice::RX_BUFFER );
		return RV_PACKET_INVALID;
	}
	if ( ftdi_header[1] != label ) {
		ftdiDevice_->purgeBuffers( FtdiDevice::RX_BUFFER );
		return RV_PACKET_NO_MATCH;
	}
	int len = ftdi_header[2] + ( ftdi_header[3] << 8 );
	if ( len != (int)length ) {
		ftdiDevice_->purgeBuffers( FtdiDevice::RX_BUFFER );
		return RV_PACKET_NO_MATCH;
	}
	
	unsigned char* buf = new unsigned char[length + 1];
	
	result = ftdiDevice_->readData( buf, length + 1, READ_TIMEOUT );
	
	if ( result < 0 ) return result;
	
	if ( result < (int)length + 1 ) {
		delete[] buf;
		return RV_PACKET_SHORT_READ;
	}
	
	if ( buf[length] != PACKET_END_CODE ) {
		//Apparently this was not the end of the packet (even though the header
		//indicated it should be), so purge the read buffer.
		ftdiDevice_->purgeBuffers( FtdiDevice::RX_BUFFER );
		delete[] buf;
		return RV_PACKET_INVALID;
	}
	
	std::memcpy( const_cast<unsigned char*>( data ), buf, length );
	delete[] buf;
	
	return 0;
}

/*
 * Attempts to write the specified number of bytes from the given buffer.
 *
 * Returns: 0 if written successfully, or < 0 if an error occured. If the device
 * is not open, DmxDevice::DEVICE_NOT_OPEN is returned.
 */
int DmxUsbProDevice::sendUsbProPacket( int label, const unsigned char* data, unsigned int length ) const
{
	//fprintf( stderr, "sendUsbProPacket: about to send payload of %i bytes.\n", length ); //LOG
	
	if ( ! isOpen() ) return DmxDevice::RV_DEVICE_NOT_OPEN;
	if ( length > PACKET_MAX_DATA_SIZE ) return RV_PACKET_TOO_LONG;
	
	unsigned char* packet = new unsigned char[5 + length];
	
	packet[0] = PACKET_START_CODE;
	packet[1] = label;
	packet[2] = length & 0xFF;
	packet[3] = length >> 8;
	packet[4 + length] = PACKET_END_CODE;
	
	std::memcpy( &( packet[4] ), data, length );
	
	int r = ftdiDevice_->writeData( packet, length + 5 );
	
	if ( r < 0 ) return r;
	
	return ( r == (int)length + 5 ) ? 0 : RV_PACKET_SHORT_WRITE;
}
