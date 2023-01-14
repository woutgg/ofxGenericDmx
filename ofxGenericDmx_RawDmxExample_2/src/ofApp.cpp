#include <stdlib.h>
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

//	ofSetFrameRate( 44 );
    ofSetFrameRate( 30 );
	//zero our DMX value array
//	memset( dmxData_, 0, DMX_DATA_LENGTH );

	//open the device
	dmx = ofxGenericDmx::createDevice(DmxDevice::DMX_DEVICE_RAW);
    bool opened = dmx->connect(0,512);
//	bool opened = dmx->open();
	if ( dmx == 0 || !opened ) printf( "No FTDI Device Found\n" );
	else printf( "isOpen: %i\n", dmx->isOpen() );
    ofLog()<<"deviceString "<<dmx->getDeviceString();

    //example/color-related-stuff
	generateColorPicker(ofGetWidth(), ofGetHeight());
	red=green=blue=0;
	demoMode = 0;

	frames = 0;

	printf("ofxGenericDmx addon version: %s.%s\n", ofxGenericDmx::VERSION_MAJOR, ofxGenericDmx::VERSION_MINOR);
}

//--------------------------------------------------------------
void ofApp::update(){

	/*
	 *	let's set some dmx values to be communicated with your dmx interface!
	 *
	 *	obviously this is based on the way you've set up your dmx network and devices.
	 *	namely the addressing (which light/device is set to which channel)
	 *	and the number of channels each device uses.
	 *
	 *	most lights for example use 3 channels for R,G & B values and oftentimes
	 *	also a fourth 'master' channel that's used to set the overall brightness.
	 *
	 *	this example is based on a dmx light thas uses 3 dmx channels (R,G & B)
	 *	the light's address is set to channel 10 (so R is channel 10, G = 11 & B = 12).
	 *
	 *  change this to match your own setup!
	 */

	//put some color in this example:
	setColorsToSend();

	//asign our colors to the right dmx channels
//	dmxData_[1] = int(red);
//	dmxData_[2] = int(green);
//	dmxData_[3] = int(blue);
    dmx->setLevel(1,int(red));
    dmx->setLevel(2,int(green));
    dmx->setLevel(3,int(blue));
    
	//force first byte to zero (it is not a channel but DMX type info - start code)
//	dmxData_[0] = 0;
    dmx->setLevel(0,0);

	if ( ! dmx || ! dmx->isOpen() ) {
		printf( "Not updating, enttec device is not open.\n");
	}
	else{
		//send the data to the dmx interface
//        dmx->writeDmx( dmxData_, DMX_DATA_LENGTH );
        dmx->update();
	}
}

//--------------------------------------------------------------
void ofApp::setColorsToSend(){

	/*
	 *	three different examples/demoModes that set/generate a color
	 *	the colorPicker is actually quite usefull!
	 */

	switch (demoMode) {
		case 0:
		//use the colorPicker
			pickColor(mouseX, mouseY);
			break;
		case 1:
		//static green!
			red		= 0;
			green	= 255;
			blue	= 0;
			break;
		case 2:
		//something a bit more dynamic
			frames = ++frames % 360;
			//static const float DegRadFactor = PI / 180.0f;
			float radR = ofDegToRad(frames);
			float radG = ofDegToRad(frames + 120);
			float radB = ofDegToRad(frames + 240);
			float intensity = 255;
			red		= ( cosf( radR ) + 1.0f ) * intensity / 2;
			green	= ( sinf( radG ) + 1.0f ) * intensity / 2;
			blue	= ( sinf( radB ) + 1.0f ) * intensity / 2;
			break;
	}
    
   
}

//--------------------------------------------------------------
void ofApp::draw(){

	//use the color for the app background
	ofBackground(red, green, blue);
	ofSetColor(255, 255, 255);

	//app info
	switch (demoMode) {
		case 0:
		//colorPicker
			colorPicker.draw(0,0);
			ofDrawBitmapString("Color Picker", 10, 20);
			break;
		case 1:
		//static green
			ofDrawBitmapString("Static Green", 10, 20);
			break;
		case 2:
		//dynamic
			ofDrawBitmapString("Dynamic Color", 10, 20);
			break;
	}

    ofDrawBitmapString(dmx->getDeviceString(), 10,40);
    
    stringstream ss;
    ss<<int(dmx->getLevel(1))<<",";
    ss<<int(dmx->getLevel(2))<<",";
    ss<<int(dmx->getLevel(3));
    ofDrawBitmapString(ss.str(), 10,60);
    
	ofDrawBitmapString("use arrow LEFT & RIGHT keys to change modes", 70, 500);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch (key) {
		case OF_KEY_RIGHT:
			demoMode++;
			if(demoMode>2)demoMode=0;
			break;
		case OF_KEY_LEFT:
			demoMode--;
			if(demoMode<0)demoMode=2;
			break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::exit(){

	if ( dmx && dmx->isOpen() ) {
		// send all zeros (black) to every dmx channel and close!
//		for ( int i = 0; i <= DMX_DATA_LENGTH; i++ ) dmxData_[i] = 0;
//        dmx->writeDmx( dmxData_, DMX_DATA_LENGTH );
//        dmx->close();
        dmx->exit();
	}
}

//--------------------------------------------------------------
void ofApp::generateColorPicker(int width , int height){

	//generate a colorPicker image (needs oF007 for the color operations)

	//this function will create a Radial Spectrum Color Chart
	//inspired by Mykhel Trinitaria:
	//http://mtrinitaria.com/mykhel/color-spectrum-chart-as3/
	//done in just a few lines of code by Rick Companje

	float w = width;
    float h = height;
    float cx = w/2;
    float cy = h/2;

    colorPicker.allocate(w,h,OF_IMAGE_COLOR);

    for (float y=0; y<h; y++) {
        for (float x=0; x<w; x++) {

            float angle = atan2(y-cy,x-cy)+PI;
            float dist = ofDist(x,y,cx,cy);
            float hue = angle/TWO_PI*255;
            float sat = ofMap(dist,0,w/4,0,255,true);
            float bri = ofMap(dist,w/4,w/2,255,0,true);

            colorPicker.setColor(x,y,ofColor::fromHsb(hue,sat,bri));
        }
    }
    colorPicker.update();
}

//--------------------------------------------------------------
void ofApp::pickColor(int x , int y){

	/*
	//get the color value in our colorPicker image under the mouse location
	int bpp = colorPicker.bpp /8;	//from bits per pixel to bytes per pixel
	//set our colors:
	red = colorPicker.getPixels()[ (y*512+x)*bpp ];
	green = colorPicker.getPixels()[ (y*512+x)*bpp +1 ];
	blue = colorPicker.getPixels()[ (y*512+x) *bpp +2 ];
	*/

	//get the color value in our colorPicker image under the mouse location
	red = colorPicker.getColor(x, y).r;
	green = colorPicker.getColor(x, y).g;
	blue = colorPicker.getColor(x, y).b;
}
