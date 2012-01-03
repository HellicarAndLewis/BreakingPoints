//
//  BreakingPointsScreen.cpp
//  threadedImageLoaderExample
//
//  Created by Joel Gethin Lewis on 25/05/2011.
//  Copyright 2011 Hellicar&Lewis. All rights reserved.
//

#include "BreakingPointsScreen.h"

void BreakingPointsScreen::setup(string _name, ofPoint _position, ofPoint _size, ofxThreadedImageLoader* _loader){
    
//    float imageWidth = 1280;
//    float imageHeight = 800;
    
//    currentImage.allocate(imageWidth,imageHeight, OF_IMAGE_COLOR);
//    nextImage.allocate(imageWidth,imageHeight, OF_IMAGE_COLOR);
//    
//    currentWatershed.allocate(imageWidth,imageHeight, OF_IMAGE_GRAYSCALE);
//    nextWatershed.allocate(imageWidth,imageHeight, OF_IMAGE_GRAYSCALE);
    
    name = _name;
    
    position = _position;
    size = _size;
    
    imageDirectory.allowExt("tif");
    int numberOfImages = imageDirectory.listDir("imagery");
    
    watershedDirectory.allowExt("tif");
    int numberOfWatersheds = watershedDirectory.listDir("watersheds");    
    
    cout << "Number of images " << numberOfImages << endl;
    cout << "Number of watersheds " << numberOfWatersheds << endl;
    
    loader = _loader;
    
    if( (numberOfImages > 0) && (numberOfWatersheds > 0) && (numberOfImages == numberOfWatersheds)){ //files exist, and there is a match in count
        string currentImageFileName = randomFileFromDirectory(&imageDirectory);
        string nextImageFileName = randomFileFromDirectory(&imageDirectory);
        
        loader->loadFromDisk(&current.theImage, "imagery/"+currentImageFileName);
        loader->loadFromDisk(&next.theImage, "imagery/"+nextImageFileName);
        
        loader->loadFromDisk(&current.theWatershed, "watersheds/"+currentImageFileName);
        loader->loadFromDisk(&next.theWatershed, "watersheds/"+nextImageFileName);
        
//        loader.startThread(false, false);  
    }else{
        cerr << "Error loading images or watersheds, " << numberOfImages << " images found, with " << numberOfWatersheds << " watersheds found. " << endl;
    }
    
    bWaiting = false;
    timeOfStartOfWait = 0.f;
    waitPeriod = 5.f;
    
    current.rng = cvRNG(-1);
    next.rng = cvRNG(-1);
	
    ofxCvColorImage	originalImage;
    originalImage.allocate(1280,800); //naughty using constant value here!
    
    current.wshed = cvCloneImage( originalImage.getCvImage() );
    current.marker_mask = cvCreateImage( cvGetSize(current.wshed), 8, 1 ); //8 bit depth, 1 channel
    current.markers = cvCreateImage( cvGetSize(current.wshed), IPL_DEPTH_32S, 1 ); // 32bit short depth 1 channel
    
    next.wshed = cvCloneImage( originalImage.getCvImage() );
    next.marker_mask = cvCreateImage( cvGetSize(next.wshed), 8, 1 );//8 bit depth, 1 channel
    next.markers = cvCreateImage( cvGetSize(next.wshed), IPL_DEPTH_32S, 1 );    // 32bit short depth 1 channel
    
    current.watershedDone = false;
    next.watershedDone = false;
    
    timeOfLastFrame = ofGetElapsedTimef();
}

void BreakingPointsScreen::update(){
    
    float now = ofGetElapsedTimef();
    
    float elapsedSecondsSinceLastFrame = now - timeOfLastFrame;
    
    if(loaderFinishedLoading()){
        //safe to do watershed's at this stage
        if(!current.watershedDone){
            doWatershedAndLayers(&current);
        }
        
        if(!next.watershedDone){
            doWatershedAndLayers(&next);
        }
        
        updateFadingLayers(&current, elapsedSecondsSinceLastFrame);
        updateFadingLayers(&next, elapsedSecondsSinceLastFrame);
    }else{
        //not much to do but wait
    }
    
    timeOfLastFrame = now;
}

void BreakingPointsScreen::draw(){
    current.theImage.draw(position.x, position.y, size.x, size.y);
    
    ofEnableAlphaBlending();

    if(next.isStrobe){
        drawFadedLayers(&next);        
    }else{
        drawARandomLayer(&next);
    }

    ofDisableAlphaBlending();
}

void BreakingPointsScreen::drawCurrent(){
    //or just draw the current....
    current.theImage.draw(position.x, position.y, size.x, size.y);
}

void BreakingPointsScreen::pickAndLoadNewImageAndWatershed(){
    current.theImage.setFromPixels(next.theImage.getPixels(), next.theImage.getWidth(), next.theImage.getHeight(), OF_IMAGE_COLOR);
    current.theImage.update();
    current.theWatershed.setFromPixels(next.theWatershed.getPixels(), next.theWatershed.getWidth(), next.theWatershed.getHeight(), OF_IMAGE_GRAYSCALE);
    current.theWatershed.update();
    
    string newImageFilename = randomFileFromDirectory(&imageDirectory);
    
    cout << name << " is trying to load: " << newImageFilename << endl;
    
    loader->loadFromDisk(&next.theImage, "imagery/"+newImageFilename);
    loader->loadFromDisk(&next.theWatershed, "watersheds/"+newImageFilename);
    
    bWaiting = false;
    
    current.watershedDone = false;
    next.watershedDone = false;
}

bool BreakingPointsScreen::isThreadRunning(){
    return loader->isThreadRunning();
}

bool BreakingPointsScreen::loaderFinishedLoading(){
    return !loader->allImagesLoaded();
}

string BreakingPointsScreen::randomFileFromDirectory(ofDirectory* theDirectory){
    string fullPath = "EMPTY";
    
    int numberOfFilesInDirectory = theDirectory->size();
    
    if(numberOfFilesInDirectory > 0){
        int randomIndex = (int)ofRandom(0,numberOfFilesInDirectory);
        fullPath = theDirectory->getName(randomIndex);
    }
    
    return fullPath;
}

void BreakingPointsScreen::doWatershedAndLayers(BreakingPointsImage* theBreakingPointsImage){
    cvZero( theBreakingPointsImage->marker_mask ); //zero out the mask to start with
    
    int imageWidth = theBreakingPointsImage->theWatershed.width;
    int imageHeight = theBreakingPointsImage->theWatershed.height;
        
    ofxCvGrayscaleImage cvGrayWater;
    
    cvGrayWater.allocate(imageWidth, imageHeight);
    
    cvGrayWater.setFromPixels(theBreakingPointsImage->theWatershed.getPixels(), imageWidth, imageHeight);
    
    cvCopy(cvGrayWater.getCvImage(), theBreakingPointsImage->marker_mask);

    CvMemStorage* storage = cvCreateMemStorage(0);
    CvSeq* contours = 0;
    CvMat* color_tab;
    int i, j, comp_count = 0;
    
    cvZero( theBreakingPointsImage->markers );
    cvZero( theBreakingPointsImage->wshed );
    
    cvFindContours( theBreakingPointsImage->marker_mask, storage, &contours, sizeof(CvContour),
                   CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
    
    for( ; contours != 0; contours = contours->h_next, comp_count++ )
    {
        cvDrawContours( theBreakingPointsImage->markers, contours, cvScalarAll(comp_count+1),
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
        ptr[0] = (uchar)(cvRandInt(&theBreakingPointsImage->rng)%180 + 50);
        ptr[1] = (uchar)(cvRandInt(&theBreakingPointsImage->rng)%180 + 50);
        ptr[2] = (uchar)(cvRandInt(&theBreakingPointsImage->rng)%180 + 50);
    }
    
//    double t = (double)cvGetTickCount();
    
    ofxCvColorImage cvTempImage;
    
    cvTempImage.allocate(imageWidth, imageHeight);
    
    cvWatershed( cvTempImage.getCvImage(), theBreakingPointsImage->markers );
    
//    t = (double)cvGetTickCount() - t;
//    printf( "exec time = %gms\n", t/(cvGetTickFrequency()*1000.) );
    
    // paint the watershed image
    for( i = 0; i < theBreakingPointsImage->markers->height; i++ ){
        for( j = 0; j < theBreakingPointsImage->markers->width; j++ ){
            int idx = CV_IMAGE_ELEM( theBreakingPointsImage->markers, int, i, j );
            uchar* dst = &CV_IMAGE_ELEM( theBreakingPointsImage->wshed, uchar, i, j*3 );
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
    
    ofxCvColorImage tempToDrawWith;
    tempToDrawWith.allocate(imageWidth, imageHeight);
    ofxCvGrayscaleImage tempToDrawWithGrey;
    tempToDrawWithGrey.allocate(imageWidth, imageHeight);
    
    cvCopy(theBreakingPointsImage->wshed, tempToDrawWith.getCvImage() );
    tempToDrawWith.flagImageChanged();
    
    tempToDrawWithGrey = tempToDrawWith;//converting automatically i hope
    
    tempToDrawWithGrey.contrastStretch(); //as much contrast as we can get
    
    tempToDrawWithGrey.dilate(); //stretch out the white borders
    
    tempToDrawWithGrey.invert();	//make them black
    
    tempToDrawWithGrey.threshold(1); //make all the grey white
    
    theBreakingPointsImage->contourFinder.findContours(tempToDrawWithGrey, 20, 0.9f*(float)(imageWidth * imageHeight), 10, true, true);
    
    int numberOfBlobsFound = theBreakingPointsImage->contourFinder.blobs.size();
    
    //cout << contourFinder.blobs.size() << " was the number of blobs" << endl;
    if(numberOfBlobsFound > 0){
        theBreakingPointsImage->layers.clear();
        theBreakingPointsImage->layerMasks.clear();
        theBreakingPointsImage->layerFades.clear();
        theBreakingPointsImage->fadeSpeeds.clear();
        
        theBreakingPointsImage->layers.resize(numberOfBlobsFound);
        theBreakingPointsImage->layerMasks.resize(numberOfBlobsFound);
        theBreakingPointsImage->layerFades.resize(numberOfBlobsFound);
        theBreakingPointsImage->fadeSpeeds.resize(numberOfBlobsFound);       

        
        for(int i=0; i< numberOfBlobsFound; i++){
            theBreakingPointsImage->layers[i].allocate(imageWidth, imageHeight,OF_IMAGE_COLOR_ALPHA);
            theBreakingPointsImage->layerMasks[i].allocate(imageWidth, imageHeight);
            theBreakingPointsImage->layerMasks[i].drawBlobIntoMe(theBreakingPointsImage->contourFinder.blobs[i], 255);
            theBreakingPointsImage->layerMasks[i].flagImageChanged();
            
            unsigned char * pixelsSrc   = theBreakingPointsImage->theImage.getPixels();
            unsigned char * pixelsMask  = theBreakingPointsImage->layerMasks[i].getPixels();
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
            
            theBreakingPointsImage->layers[i].setFromPixels(pixelsFinal, imageWidth, imageHeight, OF_IMAGE_COLOR_ALPHA);
            
            delete[] pixelsFinal; 
            
            theBreakingPointsImage->layerFades[i] = 0; //start faded out, nahhhh random, nahh zero
            theBreakingPointsImage->fadeSpeeds[i] = ofRandomuf(); //ofRandomuf(); //random 0..1 fade speed
        }
    }	
    
    theBreakingPointsImage->watershedDone = true;
    
    if(ofRandomuf() > 0.5f){
        theBreakingPointsImage->isStrobe =  true;       
    }else{
        theBreakingPointsImage->isStrobe =  false;
    }
}

void BreakingPointsScreen::drawARandomLayer(BreakingPointsImage* theBreakingPointsImage){
    if(!theBreakingPointsImage->watershedDone){
        //if it isn't done, return
        return;
    }
    
    //this is to strobe
    if(theBreakingPointsImage->contourFinder.blobs.size() > 0){
        int randomLayer = (int)ofRandom(0, theBreakingPointsImage->contourFinder.blobs.size());
        randomLayer %= theBreakingPointsImage->contourFinder.blobs.size();//safety
        //cout << "Layer this time is" << randomLayer << endl;
        ofSetColor(255, 255, 255, 255);			
        theBreakingPointsImage->layers[randomLayer].draw(position.x, position.y, size.x, size.y);			
    }else{
        //cout << "No blobs in the contour finder!" << endl;
        
        return;
    }
}

void BreakingPointsScreen::updateFadingLayers(BreakingPointsImage* theBreakingPointsImage, float timeElapsed){
    if(!theBreakingPointsImage->watershedDone){
        //if it isn't done, return
        return;
    }
    
    int numberOfBlobsFound = theBreakingPointsImage->contourFinder.blobs.size();
    
    //this is to strobe
    if(numberOfBlobsFound > 0){
        for(int i=0; i< numberOfBlobsFound; i++){
            theBreakingPointsImage->layerFades[i] += (theBreakingPointsImage->fadeSpeeds[i] * timeElapsed);
            
            if(theBreakingPointsImage->layerFades[i] > 1.f){
                theBreakingPointsImage->layerFades[i] = 0.f;
            }
            
//            ofClamp(theBreakingPointsImage->layerFades[i], 0.f, 1.f);            
        }
    }else{
        //cout << "No blobs in the contour finder!" << endl;
        
        return;
    }    
}

void BreakingPointsScreen::drawFadedLayers(BreakingPointsImage* theBreakingPointsImage){
    if(!theBreakingPointsImage->watershedDone){
        //if it isn't done, return
        return;
    }
    
    int numberOfBlobsFound = theBreakingPointsImage->contourFinder.blobs.size();
    
    //this is to strobe
    if(numberOfBlobsFound > 0){
        for(int i=0; i< numberOfBlobsFound; i++){
            ofSetColor(255, 255, 255, (int)(255.f * theBreakingPointsImage->layerFades[i]));			
            theBreakingPointsImage->layers[i].draw(position.x, position.y, size.x, size.y);	            
        }
    }else{
        //cout << "No blobs in the contour finder!" << endl;
        
        return;
    } 
}


