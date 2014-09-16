/*
 */
#ifndef OFX_GENERIC_DMX_H
#define OFX_GENERIC_DMX_H

#include <stdint.h>
#include <vector>
#include "FtdiDevice.h"
#include "DmxUsbProDevice.h"

class DmxDevice;

class ofxGenericDmx {
public:
	static const char* VERSION_MAJOR;
	static const char* VERSION_MINOR;
	
	static DmxDevice* createDevice( DmxDevice::DMX_DEVICE_TYPE type = DmxDevice::DMX_DEVICE_ENTTECPRO );
	static DmxDevice* openFirstDevice( bool usbProOnly = true );
	static const std::vector<struct FtdiDevice::deviceInfo>* getDeviceList();
	
	static const DmxUsbProDevice* toUsbPro( const DmxDevice* dev );
	
private:
	ofxGenericDmx( const ofxGenericDmx& other );
	ofxGenericDmx& operator=( const ofxGenericDmx& other );
};

#endif /* ! OFX_GENERIC_DMX_H */
