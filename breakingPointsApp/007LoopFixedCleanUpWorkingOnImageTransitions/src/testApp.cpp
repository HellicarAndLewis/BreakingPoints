#include "testApp.h"
#include "stdio.h"

	//stuff from watershed.cpp
IplImage* marker_mask = 0;
IplImage* markers = 0;
IplImage* img0 = 0, *img = 0, *img_gray = 0, *wshed = 0;
CvPoint prev_pt = {-1,-1};

	//--------------------------------------------------------------
testApp::testApp(){
	
}

	//--------------------------------------------------------------
void testApp::setup(){
	loader.start();
    ofSetVerticalSync(true);
	ofHideCursor();
	ofSetFrameRate(30);
	
	// load the settings
	settings.loadSettings("app_settings.xml");	
	
    currImg = NULL;
    loadNextImg = false;
	
	currentCue = -1;
	nextCueTime = 0.f;
	
		//xml
	if( XML.loadFile("audio/breakingPointsCueList.xml") ){
		cout << "XML file loaded" << endl;
		
			//lets see how many <STROKE> </STROKE> tags there are in the xml file
		int numCuesTags = XML.getNumTags("CUES:CUE");
		
		cout << "Number of cues tags: " << numCuesTags;
		
			//if there is at least one <CUES> tag we can read the list of cues
			//and use that to work out all the cue times of the special transitions
		
		if(numCuesTags > 0){
			
				//we push into the first CUES tag
				//this temporarirly treats the tag as
				//the document root.
			XML.pushTag("CUES", 0);
			
				//we see how many points we have stored in <CUE> tags
			int numCueTags = XML.getNumTags("CUE");
			
			if(numCueTags > 0){
				
				cuePoints.resize(numCueTags);
				
					//now read in all the cue data
				
				for(int i = 0; i < numCueTags; i++){
						//the last argument of getValue can be used to specify
						//which tag out of multiple tags you are refering to.
					float cueTime = XML.getValue("CUE", -1.f, i);
					cuePoints[i] = cueTime;
					cout << i << "th cue is: " << cueTime << endl;
				}
				
				currentCue = 0; //now we have at least one cue, so this is safe, should check against
								//the size of the vector in future
			}
			
				//this pops us out of the CUE tags
				//sets the root back to the xml document
			XML.popTag();
		}		
	}else{
		cout << "Unable to load XML file" << endl;
	}
	
		//from ofxcontrol panel
	
	ofxControlPanel::setBackgroundColor(simpleColor(30, 30, 60, 200));
	ofxControlPanel::setTextColor(simpleColor(240, 50, 50, 255));
	
	gui.loadFont("MONACO.TTF", 8);		
	gui.setup("Breaking Points", 0, 0, ofGetWidth(), 700);
	gui.toggleView(); //to hide by default	
	gui.addPanel("Breaking Points Statistics", 4, false);	
	
	ofxControlPanel::setBackgroundColor(simpleColor(30, 30, 30, 200));	
	
		//--------- PANEL 1
	gui.setWhichPanel(0);
	
	gui.setWhichColumn(0);
	
	lister.listDir("imagery/");
	gui.addFileLister("Breaking Points Imagery", &lister, 200, 300);
	
	gui.setWhichColumn(1);
	
		//some dummy vars we will update to show the variable lister object
	elapsedTime		= ofGetElapsedTimef();
	appFrameCount	= ofGetFrameNum();	
	appFrameRate	= ofGetFrameRate();
	audioTime		= ofGetElapsedTimef();	
	
	vector <guiVariablePointer> vars;
	vars.push_back( guiVariablePointer("total elapsed time", &elapsedTime, GUI_VAR_FLOAT, 2) );
		//new bits for breaking points
	vars.push_back( guiVariablePointer("audio time", &audioTime, GUI_VAR_FLOAT, 2) );
	vars.push_back( guiVariablePointer("next cue index", &currentCue, GUI_VAR_INT) );
	vars.push_back( guiVariablePointer("next cue time", &nextCueTime, GUI_VAR_FLOAT, 2) );	
	vars.push_back( guiVariablePointer("elapsed frames", &appFrameCount, GUI_VAR_INT) );
	vars.push_back( guiVariablePointer("app fps", &appFrameRate, GUI_VAR_FLOAT, 2) );
	vars.push_back( guiVariablePointer("mouse x", &mouseX, GUI_VAR_INT) );
	vars.push_back( guiVariablePointer("mouse y", &mouseY, GUI_VAR_INT) );
	
	gui.addVariableLister("app vars", vars);
	
	gui.addChartPlotter("FPS Graph", guiStatVarPointer("app fps", &appFrameRate, GUI_VAR_FLOAT, true, 2), 200, 100, 200, 5, 40);
	
	//SETTINGS AND EVENTS
	
		//load from xml!
	gui.loadSettings("controlPanelSettings.xml");
	
		//if you want to use events call this after you have added all your gui elements
	gui.setupEvents();
	gui.enableEvents();
	
		//opencv bits for fading
	cvFirstImage.allocate(settings.APP_W, settings.APP_H);
	cvSecondImage.allocate(settings.APP_W, settings.APP_H);
	
	ofImage fillFirstImage;
	fillFirstImage.allocate(settings.APP_W, settings.APP_H, GL_RGB);
	fillFirstImage.loadImage("breakingPointsTestImage.tif");
	cvFirstImage.setFromPixels(fillFirstImage.getPixels(), settings.APP_W, settings.APP_H);
	
	mixDone = true;
	
	originalImage.allocate(settings.APP_W, settings.APP_H);
	
		//from watershed.cpp
	rng = cvRNG(-1);
	
    wshed = cvCloneImage( originalImage.getCvImage() );
    marker_mask = cvCreateImage( cvGetSize(wshed), 8, 1 );
    markers = cvCreateImage( cvGetSize(wshed), IPL_DEPTH_32S, 1 );
	
	inputDirList.setVerbose(false);
	inputDirList.reset();
	inputDirList.allowExt("tif");
	strLoadLocation = "imagery/";
	waterShedLocation = "watersheds/";
	numberInputImages = inputDirList.listDir(strLoadLocation);
	cout << "Number of images found: " << numberInputImages << endl;
	
		//audio
	audio.loadSound("audio/StereoMix.aif", true); //streaming
	audio.setVolume(0.75f);
	audio.setLoop(false);	
	audioStartTime = ofGetElapsedTimef();
	audio.play();
	audioTime = ofGetElapsedTimef()-audioStartTime;	
	requestedNextImageAlready = false;
	
	overAllFadeTime = 1.f;
}
	//--------------------------------------------------------------
void testApp::update(){
	ofBackground(0,0,0);   // black because threads are EVIL ;)
	
	// update the sound playing system:
	ofSoundUpdate();
	
	float now = ofGetElapsedTimef();
	
		//from ofxcontrolpanel
	elapsedTime		= ofGetElapsedTimef();		
	audioTime		= now-audioStartTime;
	appFrameCount	= ofGetFrameNum();	
	appFrameRate	= ofGetFrameRate();
	
	if(currentCue != -1){
			//then we managed to load some cues
		nextCueTime = cuePoints[currentCue];
		
		float audioLengthInSeconds = (float)audio.length/44100.f; //lenth is in samples NOT seconds!
		
			//cout << "Audio time" << audioTime << ", Audio length in seconds " << audioLengthInSeconds << endl;
		
		if(audioTime > audioLengthInSeconds){
				//then it's been playing longer than it's length, so go back to the beginning and start again
			audio.stop();
			audio.setPosition(0.f);
			currentCue = 0; //back to the beginning of the cues
			audio.play();			
			audioStartTime = ofGetElapsedTimef();
		}else {
				//if you have to reset then don't do this bit
			if ((audioTime > (nextCueTime-0.5f)) && !requestedNextImageAlready) {
					//if we are within 0.5 seconds of the cue, then load the next image ready
				loadNextImg = true;
				requestedNextImageAlready = true;
			}
			
			if ((audioTime > nextCueTime) && requestedNextImageAlready) {
					//so now it's time to start the fade
				mixStartTime = ofGetElapsedTimef();
				mixDone = false;
				
				overAllFadeTime = ofRandom(0.5f, 1.f);
				
				currentCue++;
				currentCue = currentCue % cuePoints.size();
			}			

		}
	}
	
	if(!mixDone){
		float timeSinceStartOfMix = now-mixStartTime;
		
		if(timeSinceStartOfMix > overAllFadeTime){
			cvFirstImage = cvSecondImage;
			mixDone = true;
			requestedNextImageAlready = false;
		}	
	}
	
	if(loadNextImg){
		string waterShedToUse = loader.getTextureFileName();
        ofImage * nextImg = loader.getNextTexture();
        if(nextImg!=NULL){
			currImg=nextImg;
			loadNextImg=false;
				//always loading into the second image
			cvSecondImage.setFromPixels(currImg->getPixels(),settings.APP_W, settings.APP_H);
			
				//this is the first instance that we have access to the data, so we should call all the watershed stuff now
			cout << "Just loaded in " << loader.getTextureFileName() << ", about to try to find corresponding older watershed" <<  waterShedToUse << endl;
			
			setupForNewImage(waterShedToUse);
			doWatershed();
			updateLayers();
        }
	}
	
	gui.update();
}


	//--------------------------------------------------------------
void testApp::draw(){
	ofSetColor(0xFFFFFF);

	float now = ofGetElapsedTimef();
	
	ofEnableAlphaBlending();
	
	cvFirstImage.draw(0, 0);
	
	if(!mixDone){
		float timeSinceStartOfMix = now-mixStartTime;
		
		if((timeSinceStartOfMix > 0.f) && (timeSinceStartOfMix < overAllFadeTime)){
			float mixAmount = timeSinceStartOfMix/overAllFadeTime;
			
				//this is to strobe
			if(contourFinder.blobs.size() > 0){
				int randomLayer = (int)ofRandom(0, contourFinder.blobs.size());
				randomLayer %= contourFinder.blobs.size();//safety
				cout << "Layer this time is" << randomLayer << endl;
				ofSetColor(255, 255, 255, mixAmount*255.f);			
				layers[randomLayer].draw(0,0);			
			}

			//this is to draw all layers with different fade rates..
//			for(int i = 0; i < contourFinder.blobs.size(); i++){
//				float fadeTime = layerFadeTimes[i];
//				float mixAmount = timeSinceStartOfMix/fadeTime;
//				if(mixAmount > 1.f){
//					mixAmount = 1.f;
//				}
//				
//				ofSetColor(255, 255, 255, mixAmount*255.f);
//				layers[i].draw(0,0);	
//			}
		}
	}	
	
	ofDisableAlphaBlending();
		//ofCircle(currX, ofGetHeight()/2, 50);
	
		//from ofx control panel
	ofSetColor(0xffffff);
	gui.draw();	
}

	//--------------------------------------------------------------
void testApp::keyPressed  (int key){
	switch (key){
		case 'f':{
			ofToggleFullscreen();
			break;				
		}
		case 'h':{
			gui.toggleView();	
			break;
		}
		case 'm': 		
			if(bShowMouse){
				ofHideCursor();
				bShowMouse = false;
			}else {
				ofShowCursor();
				bShowMouse = true;
			} 
			break;			
		default:{
			break;
		}
	}
}

	//--------------------------------------------------------------
void testApp::keyReleased(int key){
	
}

	//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
	
}

	//these three from ofxcontrol panel
	//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	gui.mouseDragged(x, y, button);
}

	//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	gui.mousePressed(x, y, button);
	
}

	//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	gui.mouseReleased();
}

	//--------------------------------------------------------------
void testApp::resized(int w, int h){
	
}

bool testApp::fileExists(string absolutePath){
	File fileExistsTester(absolutePath);
	return fileExistsTester.exists();
}

void testApp::setupForNewImage(string fileName){
	cvZero( marker_mask ); //zero out the mask to start with
	
		//now check to see if the file exists in the watersheds folder
	string str_PathToCheckForExistenceOfWatershed = waterShedLocation + fileName;
	
	bool waterShedExists = fileExists(ofToDataPath(str_PathToCheckForExistenceOfWatershed, true)); //true means to get abs
	
	if(waterShedExists){
			//then we already have a file there, so load it in
		cout  << "Found pre existing watershed image, " << str_PathToCheckForExistenceOfWatershed << endl;
		
		ofImage	existingWaterShed;
		
		existingWaterShed.allocate(settings.APP_W, settings.APP_H, OF_IMAGE_GRAYSCALE);
		
		existingWaterShed.loadImage(str_PathToCheckForExistenceOfWatershed);
		
		ofxCvGrayscaleImage cvGrayWater;
		
		cvGrayWater.allocate(settings.APP_W, settings.APP_H);
		
		cvGrayWater.setFromPixels(existingWaterShed.getPixels(), settings.APP_W, settings.APP_H);
		
		cvCopy(cvGrayWater.getCvImage(), marker_mask);
		
	}else {
			//then we don't have an existing file, so you don't have to do anything.
		
		cerr << "NO WATERSHED FOUND," << str_PathToCheckForExistenceOfWatershed << " does not exist" << endl; 
	}
}

void testApp::doWatershed(){
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contours = 0;
	CvMat* color_tab;
	int i, j, comp_count = 0;
	
	cvZero( markers );
	cvZero( wshed );
	
	cvFindContours( marker_mask, storage, &contours, sizeof(CvContour),
				   CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
	
	for( ; contours != 0; contours = contours->h_next, comp_count++ )
	{
		cvDrawContours( markers, contours, cvScalarAll(comp_count+1),
					   cvScalarAll(comp_count+1), -1, -1, 8, cvPoint(0,0) );
	}
	
	if(comp_count == 0){
		cout << "Can't do watershed with no contours! " << endl;
		return;
	}
	
	color_tab = cvCreateMat( 1, comp_count, CV_8UC3 );
	for( i = 0; i < comp_count; i++ )
	{
		uchar* ptr = color_tab->data.ptr + i*3;
		ptr[0] = (uchar)(cvRandInt(&rng)%180 + 50);
		ptr[1] = (uchar)(cvRandInt(&rng)%180 + 50);
		ptr[2] = (uchar)(cvRandInt(&rng)%180 + 50);
	}
	
		double t = (double)cvGetTickCount();
		
	//make sure you are always working on the second Image
	
	cvWatershed( cvFirstImage.getCvImage(), markers );
	
	t = (double)cvGetTickCount() - t;
	printf( "exec time = %gms\n", t/(cvGetTickFrequency()*1000.) );
	
		// paint the watershed image
	for( i = 0; i < markers->height; i++ ){
		for( j = 0; j < markers->width; j++ ){
			int idx = CV_IMAGE_ELEM( markers, int, i, j );
			uchar* dst = &CV_IMAGE_ELEM( wshed, uchar, i, j*3 );
			if( idx == -1 )
				dst[0] = dst[1] = dst[2] = (uchar)255;
			else if( idx <= 0 || idx > comp_count )
				dst[0] = dst[1] = dst[2] = (uchar)0; // should not get here
			else
			{
				uchar* ptr = color_tab->data.ptr + (idx-1)*3;
				dst[0] = ptr[0]; 
				dst[1] = ptr[1]; 
				dst[2] = ptr[2];
			}
		}		
	}
	
	cvReleaseMemStorage( &storage );
	cvReleaseMat( &color_tab );	
}

void testApp::updateLayers(){
	ofBackground(0,0,0);
	
	int imageWidth = settings.APP_W;
	int imageHeight = settings.APP_H;
	
	ofxCvColorImage tempToDrawWith;
	tempToDrawWith.allocate(imageWidth, imageHeight);
	ofxCvGrayscaleImage tempToDrawWithGrey;
	tempToDrawWithGrey.allocate(imageWidth, imageHeight);
	
	cvCopy(wshed, tempToDrawWith.getCvImage() );
	tempToDrawWith.flagImageChanged();
	
	tempToDrawWithGrey = tempToDrawWith;//converting automatically i hope
	
	tempToDrawWithGrey.contrastStretch(); //as much contrast as we can get
	
	tempToDrawWithGrey.dilate(); //stretch out the white borders
	
	tempToDrawWithGrey.invert();	//make them black
	
	tempToDrawWithGrey.threshold(1); //make all the grey white
	
		//tempToDrawWithGrey.draw(0,0); //draw the black and white image, not in draw any more!
	
	contourFinder.findContours(tempToDrawWithGrey, 20, 0.9f*(float)(imageWidth * imageHeight), 10, true, true);
	
		//cout << contourFinder.blobs.size() << " was the number of blobs" << endl;
	if(contourFinder.blobs.size() > 0){
		
		layers.clear();
		layerMasks.clear();
		layerFadeTimes.clear();
		
		layers.resize(contourFinder.blobs.size());
		layerMasks.resize(contourFinder.blobs.size());
		layerFadeTimes.resize(contourFinder.blobs.size());
		
		
		for(int i=0; i< contourFinder.blobs.size(); i++){
			layers[i].allocate(imageWidth, imageHeight,OF_IMAGE_COLOR_ALPHA);
			layerMasks[i].allocate(imageWidth, imageHeight);
			layerMasks[i].drawBlobIntoMe(contourFinder.blobs[i], 255);
			layerMasks[i].flagImageChanged();
			
			unsigned char * pixelsSrc   = cvSecondImage.getPixels();
			unsigned char * pixelsMask  = layerMasks[i].getPixels();
			unsigned char * pixelsFinal = new unsigned char[imageWidth*imageHeight*4];	//RGBA so *4		
			
			memset(pixelsFinal,0,imageWidth*imageHeight*4);
			
			for( int j = 0; j < imageWidth*imageHeight; j++)
			{
				if( pixelsMask[j*3] == 255 ) //i.e. if the mask is white at this point
				{
					pixelsFinal[j*4]    = pixelsSrc[ j*3 ];
					pixelsFinal[j*4+1]  = pixelsSrc[ j*3+1 ];
					pixelsFinal[j*4+2]  = pixelsSrc[ j*3+2 ];
					pixelsFinal[j*4+3]  = 255;
				}
				
			}
			
			layers[i].setFromPixels(pixelsFinal, imageWidth, imageHeight, OF_IMAGE_COLOR_ALPHA);
			
			layerFadeTimes[i] = ofRandom(0.1f, overAllFadeTime);
			
			delete[] pixelsFinal; 
		}
	}	
}


