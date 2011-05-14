/*
 * //openFrameworks DMX Usb Pro addon.
 * //Copyright 2010, W. Reckman. All rights reserved.
 */
#include <iostream>
#include <iomanip>
#include <unistd.h> /* for usleep() */
#include <typeinfo>
#include "DmxDevice.h"
#include "DmxRawDevice.h"
#include "DmxUsbProDevice.h"
#include "ofxGenericDmx.h"

DmxDevice::DmxDevice* ofxGenericDmx::createDevice( DmxDevice::DMX_DEVICE_TYPE type )
{
	DmxDevice* dev = 0;
	switch ( type ) {
		case DmxDevice::DMX_DEVICE_RAW: dev = new DmxRawDevice(); break;
		case DmxDevice::DMX_DEVICE_ENTTECPRO: dev = new DmxUsbProDevice(); break;
	}
	return dev;
}

/*
 * NOTE: all log messages are logged to stdout, even error messages, this is because
 * openFrameworks (v0062) also behaves this way.
 */
DmxDevice* ofxGenericDmx::openFirstDevice( bool usbProOnly, bool verbose )
{
	DmxDevice* d = 0;
	int listIdx = -1;
	bool devIsUsbPro = false;
	const FtdiDevice::vec_deviceInfo* devs = getDeviceList();
	const int devCount = devs->size();
	
	if ( verbose ) {
		if ( devCount == 0 ) {
			std::cout << "----- ofxGenericDmx: no FTDI devices were found, there is no spoon" << std::endl;
		} else if ( usbProOnly ) {
			std::cout << "----- ofxGenericDmx: looking for Enttec DMX USB Pro device to open (" << devCount << " FTDI devices to choose from)" << std::endl;
		} else {
			std::cout << "----- ofxGenericDmx: looking for DMX device to open (" << devCount << " FTDI devices to choose from)" << std::endl;
		}
	}
	
	if ( devCount == 0 ) return 0; //NOTE: return if there are no devices to consider
	
	FtdiDevice::vec_deviceInfo::const_iterator it;
	for ( it = devs->begin(); it != devs->end(); ++it ) {
		const FtdiDevice::deviceInfo& di = *it;
		
		if ( di.usbInfo != 0 && std::strncmp( di.usbInfo->description,
																				 DmxUsbProDevice::USB_DESCRIPTION,
																				 std::strlen( DmxUsbProDevice::USB_DESCRIPTION ) ) == 0 ) {
			d = createDevice( DmxDevice::DMX_DEVICE_ENTTECPRO );
			listIdx = it - devs->begin();
			devIsUsbPro = true;
			break;
		} else if ( ! usbProOnly ) {
			d = createDevice( DmxDevice::DMX_DEVICE_RAW );
			listIdx = it - devs->begin();
			devIsUsbPro = false;
			break;
		}
	}
	
	if ( listIdx >= 0 ) {
		bool r = d->open( 0, 0, listIdx );
		if ( r ) {
			if ( verbose ) {
				const std::string devDesc = devIsUsbPro ? "Enttec DMX Usb pro" : "raw device";
				const FtdiDevice::usbInformation* usbInfo = d->getUsbInformation();
				
				if ( usbInfo ) {
					std::cout << "-- successfully opened DMX device (" << devDesc << ") with device index " << listIdx << std::endl;
					std::cout << "--   USB device information: manufacturer='" << usbInfo->manufacturer
							<< "', description='" << usbInfo->description << "', serial='" << usbInfo->serial << "'" << std::endl;
				} else {
					std::cout << "-- successfully opened DMX device (" << devDesc << ") with device index "
							<< listIdx << " (could not retrieve USB details)" << std::endl;
				}
				
				if ( devIsUsbPro ) {
					const DmxUsbProDevice* usbProDev = toUsbPro( d );
					const DmxUsbProDevice::widgetParameters* params = usbProDev->getWidgetParameters();
					if ( params != 0 ) {
						std::cout << "--   Enttec device information: firmware version=" << params->firmwareVersionMajor
								<< "." << params->firmwareVersionMinor << ", break time=" << params->breakTime
								<< ", MaB time=" << params->mabTime << ", refresh rate=" << params->refreshRate << std::endl;
					} else {
						std::cout << "--   could not fetch Enttec device information." << std::endl;
					}
					
					const uint32_t* serNum = usbProDev->getSerialNumber();
					if ( serNum != 0 && *serNum != DmxUsbProDevice::SN_NOT_PROGRAMMED ) {
						std::cout << "--   Enttec device serial number: " << std::setw( 8 ) << std::setfill( '0' ) << *serNum << std::endl;
					} else {
						std::cout << "--   Enttec device serial number could not be read, or does not exist" << std::endl;
					}
				}
				
				//log: usb info
				//if device is usbpro:log widgetparms & serial number, if any
			}
		} else {
			if ( verbose ) {
				std::cout << "-- failed to open device with index " << listIdx << " ('" << d->getLastError() << "')" << std::endl;
			}
			
			delete d; d = 0;
		}
	} else if ( verbose ) {
		std::cout << "-- no suitable DMX device was found" << std::endl;
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
