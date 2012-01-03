#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"

#include "ofxOpenCv.h"
#include "ofxDirList.h"

#include "Poco/File.h"

using Poco::File;

class testApp : public ofBaseApp{

	public:

		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
	
	void doWatershed();
	void saveWatershed();
	bool fileExists(string absolutePath);
	void setupForNewImage(int imageIndex);

	ofImage	originalImage;
	
    ofxCvColorImage			cvColourImg;
    ofxCvGrayscaleImage 	cvGreyImage;
	
	bool bShowWatershed;
	
	CvRNG rng;
	
		//listing the directory of bits
	ofxDirList  inputDirList;
	
	string strLoadLocation; //load location for the watersheds
	string strSaveLocation; //save location for the watersheds
	
	int iFileIndex; //current image from the installation that we are working on
	int numberInputImages;
	
	bool bShowSketch; //show the doodle?
	bool bShowPure; //show just the pure wshed
	
	ofxCvContourFinder 	contourFinder;
	
	vector <ofImage> layers;
	vector <ofxCvColorImage> layerMasks;
	
	ofxCvColorImage tempToDrawWith;
	ofxCvGrayscaleImage tempToDrawWithGrey;
};

#endif
