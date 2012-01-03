#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"

#include "imgLoader.h"

#include "ofxXmlSettings.h"

	//from ofx control panel

#include "ofxControlPanel.h"
	//from app settings

#include "AppSettings.h"

#include "Poco/File.h"

#include "ofxOpenCv.h"

using Poco::File;

class testApp : public ofBaseApp{
	
public:
	
	testApp();
	void setup();
	void update();
	void draw();
	
	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void resized(int w, int h);
	
		//first pass at image loading, need to add blending stuff - have two imageloaders?
		//or just take a direct copy of an image and the previous one to do blends?
	
	imgLoader   loader;
	ofImage* currImg;
	bool  loadNextImg;
	
		//audio bits, need to add xml load of data file for cues and track time properlyl
	
	ofSoundPlayer	audio;
	float			audioPosition;
	
		//xml audio bits
	ofxXmlSettings XML;	
	
	vector <float> cuePoints;
	
	int currentCue;
	
	float audioStartTime;
	
		//from ofxcontrolpanel
	ofxControlPanel gui;
	simpleFileLister lister;
	
	float elapsedTime;
	int appFrameCount;
	float appFrameRate;
	
	ofTrueTypeFont TTF;
	
		//cue time for gui ease of monitoring
	
	float nextCueTime;
	
		//showing and hiding mouse
	bool bShowMouse;
	
		//for doing nice fading
	ofxCvColorImage	cvFirstImage;
	ofxCvColorImage	cvSecondImage;
	
	float mixStartTime;
	bool mixDone;
	
		//from WATERSHED experiments
	void doWatershed();
	bool fileExists(string absolutePath);
	void setupForNewImage(string fileName);
	
	ofxCvColorImage	originalImage;
	
	bool bShowWatershed;
	
	CvRNG rng;
	
		//listing the directory of bits
	ofxDirList  inputDirList;
	
	string strLoadLocation; //load location for the imagery
	string waterShedLocation; //save location for the watersheds
	
	int iFileIndex; //current image from the installation that we are working on
	int numberInputImages;
	
	bool bShowSketch; //show the doodle?
	bool bShowPure; //show just the pure wshed
	
	ofxCvContourFinder 	contourFinder;
	
	vector <ofImage> layers;
	vector <ofxCvColorImage> layerMasks;
		
		//new method taking over from update() of watershed 
	void updateLayers();
	
	bool requestedNextImageAlready;
	
	float audioTime;
	
	float overAllFadeTime;
	vector	<float> layerFadeTimes;
};

#endif
