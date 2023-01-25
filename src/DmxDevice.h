#ifndef DMX_DEVICE_H
#define DMX_DEVICE_H

#include "FtdiDevice.h"
#include <string>

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
   
    virtual bool connect(int index = 0, unsigned int channels = 512 );
    const std::string getDeviceString() const;
    void setChannels(unsigned int channels = 24); // change the number of channels
    void setLevel(unsigned int channel, unsigned char level);
    unsigned char getLevel(unsigned int channel);

    void update(bool force = false);
    bool badChannel(unsigned int channel);
    void exit();
//    std::string getDeviceString();
//    std::string deviceString;
    //stephan
    //qynn
protected:
	FtdiDevice* ftdiDevice_;
	
private:
	DmxDevice( const DmxDevice& other );
	DmxDevice& operator=( const DmxDevice& other );
    
    std::vector<unsigned char> levels;
    
    bool needsUpdate;
};

#endif /* ! DMX_DEVICE_H */
