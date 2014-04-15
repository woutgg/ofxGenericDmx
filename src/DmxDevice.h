#ifndef DMX_DEVICE_H
#define DMX_DEVICE_H

#include "FtdiDevice.h"

class DmxDevice {
public:
	enum DMX_DEVICE_TYPE {
		DMX_DEVICE_RAW,
		DMX_DEVICE_ENTTECPRO
	};
	
	static const int RV_DEVICE_NOT_OPEN;
	
	
	DmxDevice();
	virtual ~DmxDevice();
	
	virtual bool open( const char* description = 0, const char* serial = 0, int index = 0 );
	virtual bool close();
	bool isOpen() const;
	
	//virtual int readDmx( const unsigned char* data, int length ) const = 0;
	virtual int writeDmx( const unsigned char* data, int length ) const = 0;
	virtual DMX_DEVICE_TYPE getType() const = 0;
	
	//forwarding functions for FtdiDevice
	const char* getLastError() const;
	const struct FtdiDevice::usbInformation* getUsbInformation() const;
	virtual int reset();
	
protected:
	FtdiDevice* ftdiDevice_;
	
private:
	DmxDevice( const DmxDevice& other );
	DmxDevice& operator=( const DmxDevice& other );
};

#endif /* ! DMX_DEVICE_H */
