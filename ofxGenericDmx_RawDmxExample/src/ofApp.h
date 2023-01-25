#pragma once


#include "ofMain.h"
#include "ofxGenericDmx.h"

//NOTE: at least on one occasion, sending all 512 channels failed to work.
//Experiments led to stable operation by sending only 494 channels.
//#define DMX_DATA_LENGTH 494
#define DMX_DATA_LENGTH 512

// 513 values would create a maximum-sized packet (including the start code)
// This is enough data for a full frame DMX message (512 channels)
// http://en.wikipedia.org/wiki/DMX512#Protocol


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void exit();

		void keyPressed  (int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);

		//pointer to our Enntec DMX USB Pro object
		DmxDevice* dmxInterface_;

		//our DMX packet (which holds the channel values + 1st byte for the start code)
		unsigned char dmxData_[DMX_DATA_LENGTH];


		/*
		 *	the following stuff is here to enable some color variations.
		 *	there are three different modes in this example:
		 *	- Color Picker
		 *	- Static Green
		 *	- Dynamic Color (still TODO!)
		 */

		float red, green, blue;

		//here's where we will set/generate the color values
		void setColorsToSend();

		//colorPicker related
		ofImage colorPicker;
		void generateColorPicker(int width, int height);
		void pickColor(int x, int y);

		int frames;
		int demoMode;
};
