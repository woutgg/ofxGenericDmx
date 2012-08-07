/*
 */
#ifndef FTDI_DEVICE_H
#define FTDI_DEVICE_H

#include <cstring>
#include <vector>

//NOTE: this attempt to prevent warnings about constructors being hidden does not work
extern "C" {
	#include "ftdi.h"
}

struct ftdi_context;
struct ftdi_device_list;

class FtdiDevice {
public:
	enum FTDI_BUFFER_TYPE { RX_TX_BUFFER, RX_BUFFER, TX_BUFFER };
	
	/* NOTE: enum members must have different names than in libftdi in order to
	   access them. Such beauty. :S */
	enum FTDI_DATABITS_TYPE { DBITS_7 = BITS_7, DBITS_8 = BITS_8 };
	
	enum FTDI_STOPBITS_TYPE {
		SBITS_1 = STOP_BIT_1, SBITS_15 = STOP_BIT_15, SBITS_2 = STOP_BIT_2
	};
	
	enum FTDI_PARITY_TYPE {
		PAR_NONE = NONE, PAR_ODD = ODD, PAR_EVEN = EVEN, PAR_MARK = MARK, PAR_SPACE = SPACE
	};
	
	enum FTDI_BREAK_TYPE { BRK_ON = BREAK_ON, BRK_OFF = BREAK_OFF };
	
	enum FTDI_FLOWCTL_TYPE {
		FLOW_NONE = SIO_DISABLE_FLOW_CTRL, FLOW_RTS_CTS = SIO_RTS_CTS_HS,
		FLOW_DTR_DSR = SIO_DTR_DSR_HS, FLOW_XON_XOFF = SIO_XON_XOFF_HS
	};
	
	struct usbInformation {
		char* const manufacturer;
		char* const description;
		char* const serial;
		
		usbInformation()
		: manufacturer( new char[USB_INFO_FIELD_LENGTH] ),
		  description( new char[USB_INFO_FIELD_LENGTH] ),
		  serial( new char[USB_INFO_FIELD_LENGTH] )
		{}
		
		usbInformation( const usbInformation& other )
		: manufacturer( new char[USB_INFO_FIELD_LENGTH] ),
		  description( new char[USB_INFO_FIELD_LENGTH] ),
		  serial( new char[USB_INFO_FIELD_LENGTH] )
		{
			std::strncpy( manufacturer, other.manufacturer, USB_INFO_FIELD_LENGTH );
			std::strncpy( description, other.description, USB_INFO_FIELD_LENGTH );
			std::strncpy( serial, other.serial, USB_INFO_FIELD_LENGTH );
		}
		
		~usbInformation() {
			delete[] manufacturer;
			delete[] description;
			delete[] serial;
		}
		
	private:
		usbInformation& operator=( const usbInformation& other );
	};
	
	struct deviceInfo {
	public:
		struct usbInformation* usbInfo;
		
		deviceInfo()
		: usbInfo( 0 )
		{}
		
		//NOTE: no destructor present since ownership of usbInfo is taken over by creator of struct
		
	private:
		//NOTE: Apart from FtdiDevice, no classes have access to the ftdiDevice struct.
		struct libusb_device* ftdiDevice;
		
		friend class FtdiDevice;
	};
	
	typedef std::vector<deviceInfo> vec_deviceInfo;
	
	static const int RV_DEVICE_NOT_OPEN;
	
	
	FtdiDevice();
	~FtdiDevice();
	
	bool open( const char* description = 0, const char* serial = 0, int index = 0 );
	bool close();
	
	int setBaudRate( int baudRate ) const;
	int setLineProperties( FTDI_DATABITS_TYPE dataBits, FTDI_STOPBITS_TYPE stopBits,
												 FTDI_PARITY_TYPE parity, FTDI_BREAK_TYPE breakType = BRK_OFF ) const;
	int setFlowControl( FTDI_FLOWCTL_TYPE flowCtl ) const;
	
	int purgeBuffers( int bufType = RX_TX_BUFFER ) const;
	int reset() const;
	
	int setBreak( FTDI_BREAK_TYPE breakType ) const;
	int setDtr( bool dtrEnabled ) const;
	int setRts( bool rtsEnabled ) const;
	
	bool isOpen() const;
	const char* getLastError() const;
	const struct usbInformation* getUsbInformation() const;
	
	int readData( const unsigned char* data, int length, int timeout = 0 ) const;
	int writeData( const unsigned char* data, int length ) const;
	
	/* static functions */
	
	static const vec_deviceInfo* getDeviceList( ftdi_context* c = 0 );
	static const void freeDeviceList();
	
private:
	static const int USB_VENDOR_ID;
	static const int USB_PRODUCT_ID;
	static const int USB_INFO_FIELD_LENGTH;
	static vec_deviceInfo* s_deviceList;
	static struct ftdi_device_list* s_ftdiDeviceList;
	
	FtdiDevice( const FtdiDevice& other );
	FtdiDevice& operator=( const FtdiDevice& other );
	
	static struct usbInformation* fetchUsbInformation( ftdi_context* context, struct libusb_device* dev );
	
	struct ftdi_context* context_;
	const struct usbInformation* usbInfo_;
	mutable bool hasFtdiError_;
	
	mutable FTDI_DATABITS_TYPE dataBits_;
	mutable FTDI_STOPBITS_TYPE stopBits_;
	mutable FTDI_PARITY_TYPE parity_;
	mutable FTDI_BREAK_TYPE breakType_;	
};

#endif /* ! FTDI_DEVICE_H */
