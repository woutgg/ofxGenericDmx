/*
 * NOTE: this code has not been tested!
 */
#include <assert.h>
#include "DmxDevice.h"
#include "DmxRawDevice.h"

DmxRawDevice::DmxRawDevice()
{ /* empty */ }


bool DmxRawDevice::open( const char* description, const char* serial, int index )
{
	bool success = DmxDevice::open( description, serial, index );
	
	if ( success ) {
		ftdiDevice_->reset();
		success = ftdiDevice_->setBaudRate( 250000 );
		if ( success ) success = ftdiDevice_->setLineProperties( FtdiDevice::DBITS_8, FtdiDevice::SBITS_2, FtdiDevice::PAR_NONE );
		if ( success ) success = ftdiDevice_->setFlowControl( FtdiDevice::FLOW_NONE );
		if ( success ) success = ftdiDevice_->setRts( false );
		if ( success ) success = ftdiDevice_->purgeBuffers();
		
		if ( ! success ) close();
	}
	
	return success;
}

int DmxRawDevice::writeDmx( const unsigned char* data, int length ) const
{
	assert( length <= 513 );
	ftdiDevice_->setBreak( FtdiDevice::BRK_ON );
	ftdiDevice_->setBreak( FtdiDevice::BRK_OFF );
	return ftdiDevice_->writeData( data, length );
}

DmxDevice::DMX_DEVICE_TYPE DmxRawDevice::getType() const
{
	return DmxDevice::DMX_DEVICE_RAW;
}
