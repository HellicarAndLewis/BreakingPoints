#pragma once

#include "ofMain.h"
#include "BreakingPointsScreen.h"
#include "ofxXmlSettings.h"


class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);		
		
    BreakingPointsScreen firstScreen;
    BreakingPointsScreen secondScreen;
    BreakingPointsScreen thirdScreen;    
	
    //showing and hiding mouse
	bool                bShowMouse;    
    
    //audio file
	ofFmodSoundPlayer	audio;
	float               audioPosition;
	float               audioStartTime;
    float               audioTime;
    float               audioLengthInSeconds;
	
    //xml cue points
	ofxXmlSettings      xml;	
	vector <float>      cuePoints;
	int                 currentCue;
    float               nextCueTime;
    
    //timings and booleans for the displaying of a single image, not mixing...
    float singleImageStartTime;
    bool singleImageDone;
    float singleImageDisplayTime; //display the image still for this amount of time....
    
    ofxThreadedImageLoader loader;
};
