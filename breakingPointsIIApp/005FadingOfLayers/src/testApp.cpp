#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    ofSetVerticalSync(true);
	ofHideCursor();
	ofSetFrameRate(30);
    
//    int screenWidth = 1280/4;
//    int screenHeight = 800/4;
    
    int screenWidth = 800;
    int screenHeight = 600;
    
    loader.startThread(false, false);
    
    firstScreen.setup("First", ofPoint(0,0), ofPoint(screenWidth,screenHeight), &loader);
    secondScreen.setup("Second" ,ofPoint(screenWidth,0), ofPoint(screenWidth,screenHeight), &loader);
    thirdScreen.setup("Third", ofPoint(screenWidth*2,0), ofPoint(screenWidth,screenHeight), &loader);
    
	currentCue = -1;
	nextCueTime = 0.f;
	
    //xml
	if( xml.loadFile("audio/breakingPointsCueList.xml") ){
		cout << "XML file loaded" << endl;
		
		int numCuesTags = xml.getNumTags("CUES:CUE");
		
		cout << "Number of cues tags: " << numCuesTags;
		
        //if there is at least one <CUES> tag we can read the list of cues
        //and use that to work out all the cue times of the special transitions
		
		if(numCuesTags > 0){
			
            //we push into the first CUES tag
            //this temporarirly treats the tag as
            //the document root.
			xml.pushTag("CUES", 0);
			
            //we see how many points we have stored in <CUE> tags
			int numCueTags = xml.getNumTags("CUE");
			
			if(numCueTags > 0){
				
				cuePoints.resize(numCueTags);
				
                //now read in all the cue data
				
				for(int i = 0; i < numCueTags; i++){
                    //the last argument of getValue can be used to specify
                    //which tag out of multiple tags you are refering to.
					float cueTime = xml.getValue("CUE", -1.f, i);
					cuePoints[i] = cueTime;
					cout << i << "th cue is: " << cueTime << endl;
				}
				
				currentCue = 0;
			}
			
            //this pops us out of the CUE tags
            //sets the root back to the xml document
			xml.popTag();
		}		
	}else{
		cout << "Unable to load XML file" << endl;
	}
    
    //audio
	audio.loadSound("audio/StereoMix.aif", true); //streaming
	audio.setVolume(1.f); //silent/not for now!
	audio.setLoop(false);
	audioLengthInSeconds = (float)audio.length/44100.f; //lenth is in samples NOT seconds!
	audioStartTime = ofGetElapsedTimef();
	audio.play();
	audioTime = ofGetElapsedTimef()-audioStartTime;	  
    
    //displaying a single image...
    singleImageStartTime = 0.f;
    singleImageDone = true;
    singleImageDisplayTime = 1.f; //display the image still for this amount of time....    
    
}

//--------------------------------------------------------------
void testApp::update(){
    // update the sound playing system:
	ofSoundUpdate();
    
    firstScreen.update();
    secondScreen.update();
    thirdScreen.update();
    
	float now = ofGetElapsedTimef();
		
	audioTime = now-audioStartTime;
	
	if(currentCue != -1){
        //then we managed to load some cues
		nextCueTime = cuePoints[currentCue];
		
//		if(audioTime > 10.f){ //lets loop at 10 seconds!
		if(audioTime > audioLengthInSeconds){            
            //then it's been playing longer than it's length, so go back to the beginning and start again
			audio.stop();
			audio.setPosition(0.f);
			currentCue = 0; //back to the beginning of the cues
			audio.play();			
			audioStartTime = ofGetElapsedTimef();
            cout << "Back to zero cue, which is at:" << cuePoints[currentCue] << endl;
		}else {
			if ((audioTime > nextCueTime)) {
                //so now it's time to start the display of the single image, so load a new image...
                firstScreen.pickAndLoadNewImageAndWatershed();
                secondScreen.pickAndLoadNewImageAndWatershed();
                thirdScreen.pickAndLoadNewImageAndWatershed();
                
				singleImageStartTime = ofGetElapsedTimef();
				singleImageDone = false;
				
				singleImageDisplayTime = 1.f; //display the image still for this amount of time....
				
				currentCue++;
				currentCue = currentCue % cuePoints.size();
			}			
            
		}
	}
    
    if(!singleImageDone){
		float timeSinceStartOfSingle = now-singleImageStartTime;
		
		if(timeSinceStartOfSingle > singleImageDisplayTime){
			singleImageDone = true;
		}	
	}
    
}

//--------------------------------------------------------------
void testApp::draw(){	
	ofBackground(0,0,0);   // black because threads are EVIL ;)    
    ofSetColor(255, 255, 255);
    
    if(singleImageDone){
        firstScreen.draw();
        secondScreen.draw();
        thirdScreen.draw();        
    }else{
        firstScreen.drawCurrent();
        secondScreen.drawCurrent();
        thirdScreen.drawCurrent();        
    }
    
// to prove trhreaded ness....
//    ofSetColor(255, 0, 0);
//    ofDrawBitmapString(ofToString(ofGetFrameNum()), ofPoint(ofGetWidth()/2, ofGetHeight()/2));
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
	switch (key){
		case 'f':{
			ofToggleFullscreen();
			break;				
		}
//		case 'h':{
//			gui.toggleView();	
//			break;
//		}
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

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}
