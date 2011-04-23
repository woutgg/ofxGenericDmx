/*
 * NOTE: this code has not been tested!
 */
#ifndef DMX_RAW_DEVICE_H
#define DMX_RAW_DEVICE_H

#include "DmxDevice.h"

class DmxRawDevice : public DmxDevice {
public:
	DmxRawDevice();
	
	bool open( const char* description = 0, const char* serial = 0, int index = 0 );
	
	int writeDmx( const unsigned char* data, int length ) const;
	DMX_DEVICE_TYPE getType() const;
	
private:
	static const int REQUEST_REPLY_DELAY;
};

#endif /* DMX_RAW_DEVICE_H */
