/*
 */
#ifndef DMX_USB_PRO_DEVICE_H
#define DMX_USB_PRO_DEVICE_H

#include <stdint.h>
#include <vector>
#include "DmxDevice.h"

class DmxUsbProDevice : public DmxDevice {
public:
	struct widgetParameters {
		unsigned int firmwareVersionMajor;
		unsigned int firmwareVersionMinor;
		float breakTime;
		float mabTime;
		unsigned int refreshRate;
	};
	
	typedef std::vector<unsigned char> vec_uchar;
	
	static const unsigned int SN_NOT_PROGRAMMED;
	static const unsigned int BREAK_TIME_UNITS_MIN;
	static const unsigned int BREAK_TIME_UNITS_MAX;
	static const unsigned int MAB_TIME_UNITS_MIN;
	static const unsigned int MAB_TIME_UNITS_MAX;
	
	static const unsigned int OUTPUT_RATE_MAX;
	static const unsigned int USER_CONFIG_MAX_LENGTH;
	static const char* USB_DESCRIPTION;
	
	//magic return codes
	static const int RV_PACKET_TOO_LONG;
	static const int RV_PACKET_SHORT_READ;
	static const int RV_PACKET_INVALID;
	static const int RV_PACKET_NO_MATCH;
	static const int RV_PACKET_SHORT_WRITE;
	
	DmxUsbProDevice();
	~DmxUsbProDevice();
	
	int writeDmx( const unsigned char* data, int length ) const;
	DMX_DEVICE_TYPE getType() const;
	
	bool setWidgetParameters( const widgetParameters* params, const vec_uchar* userConfigData = 0 ) const;
	bool setWidgetParameters( const widgetParameters* params,
													 const unsigned char* userConfigData = 0,
													 unsigned int userConfigDataLength = 0 ) const;
	bool fetchExtendedInfo( unsigned int userConfigLength = 0 ) const;
	const widgetParameters* getWidgetParameters() const;
	const vec_uchar* getUserConfigurationData() const;
	const uint32_t* getSerialNumber() const;
	
private:
	/* START Enttec Dmx Usb Pro device declarations */
	
#pragma pack( 1 )
	typedef struct {
		unsigned char firmwareLSB;
		unsigned char firmwareMSB;
		unsigned char breakTime;
		unsigned char maBTime;
		unsigned char refreshRate;
	} DMXUSBPROParamsType;
	
	typedef struct {
		unsigned char userSizeLSB;
		unsigned char userSizeMSB;
		unsigned char breakTime;
		unsigned char maBTime;
		unsigned char refreshRate;
	} DMXUSBPROSetParamsType;
#pragma pack()
	
	struct ReceivedDmxCosStruct {
		unsigned char startChangedByteNumber;
		unsigned char changedByteArray[5];
		unsigned char changedByteData[40];
	};
	
	enum USBPRO_LABELS {
		REPROGRAM_FIRMWARE_RQ			= 1,
		PROGRAM_FLASH_PAGE_RQ			= 2,
		PROGRAM_FLASH_PAGE_REPLY	= 2,
		
		GET_WIDGET_PARAMS_RQ			= 3,
		GET_WIDGET_PARAMS_REPLY		= 3,
		SET_WIDGET_PARAMS_RQ			= 4,
		
		SET_DMX_RX_MODE						= 5,
		SET_DMX_TX_MODE						= 6,
		SEND_DMX_RDM_TX						= 7,
		RECEIVE_DMX_ON_CHANGE			= 8,
		RECEIVED_DMX_COS_TYPE			= 9,
		GET_WIDGET_SN_RQ					= 10,
		GET_WIDGET_SN_REPLY				= 10,
		SEND_RDM_DISCOVERY_RQ			= 11
	};
	
	static const float BREAK_TIME_UNIT;
	static const float MAB_TIME_UNIT;

	static const unsigned char PACKET_START_CODE;
	static const unsigned char PACKET_END_CODE;
	static const unsigned int PACKET_MAX_DATA_SIZE;
	
	/* END Enttec Dmx Usb Pro device declarations */
	
	
	static const int REQUEST_REPLY_DELAY;
	
	DmxUsbProDevice( const DmxUsbProDevice& other );
	DmxUsbProDevice& operator=( const DmxUsbProDevice& other );
	
	bool fetchWidgetParameters( unsigned int userConfigLength = 0 ) const;
	bool fetchSerialNumber() const;
	int receiveUsbProPacket( int label, const unsigned char* data, unsigned int length ) const;
	int sendUsbProPacket( int label, const unsigned char* data, unsigned int length ) const;
	
	mutable widgetParameters* widgetParams_;
	mutable vec_uchar* userConfigData_;
	mutable uint32_t* serialNumber_;
};

#endif /* DMX_USB_PRO_DEVICE_H */
