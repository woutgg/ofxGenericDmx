/*
 * //openFrameworks DMX Usb Pro addon.
 * //Copyright 2010, W. Reckman. All rights reserved.
 */
#include <unistd.h> /* for usleep() */
#include <typeinfo>
#include "DmxDevice.h"
#include "DmxRawDevice.h"
#include "DmxUsbProDevice.h"
#include "ofxGenericDmx.h"

DmxDevice* ofxGenericDmx::createDevice( DmxDevice::DMX_DEVICE_TYPE type )
{
	DmxDevice* dev = 0;
	switch ( type ) {
		case DmxDevice::DMX_DEVICE_RAW: dev = new DmxRawDevice(); break;
		case DmxDevice::DMX_DEVICE_ENTTECPRO: dev = new DmxUsbProDevice(); break;
	}
	return dev;
}

DmxDevice* ofxGenericDmx::openFirstDevice( bool usbProOnly )
{
	DmxDevice* d = 0;
	int listIdx = -1;
	const FtdiDevice::vec_deviceInfo* devs = getDeviceList();
	
	FtdiDevice::vec_deviceInfo::const_iterator it;
	for ( it = devs->begin(); it != devs->end(); ++it ) {
		const FtdiDevice::deviceInfo& di = *it;
		
		if ( di.usbInfo != 0 && std::strncmp( di.usbInfo->description,
																				 DmxUsbProDevice::USB_DESCRIPTION,
																				 std::strlen( DmxUsbProDevice::USB_DESCRIPTION ) ) == 0 ) {
			d = createDevice( DmxDevice::DMX_DEVICE_ENTTECPRO );
			listIdx = it - devs->begin();
			break;
		} else if ( ! usbProOnly ) {
			d = createDevice( DmxDevice::DMX_DEVICE_RAW );
			listIdx = it - devs->begin();
			break;
		}
	}
	
	if ( listIdx >= 0 ) {
		bool r = d->open( 0, 0, listIdx );
		if ( ! r ) {
			delete d; d = 0;
		}
	}
	
	
	return d;
}

const std::vector<struct FtdiDevice::deviceInfo>* ofxGenericDmx::getDeviceList()
{
	return FtdiDevice::getDeviceList();
}

/*
 * Returns the given DmxDevice object as a DmxUsbProDevice object if it is one,
 * otherwise it returns NULL.
 *
 * NOTE: this function merely exists to relieve users of the trouble of casting
 * themselves. However since it's not a dynamic cast, this is unsafe if the
 * wrong type is passed in.
 */
const DmxUsbProDevice* ofxGenericDmx::toUsbPro( const DmxDevice* dev )
{
	return static_cast<const DmxUsbProDevice*>( dev );
}
