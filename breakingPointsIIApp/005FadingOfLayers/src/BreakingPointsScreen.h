//
//  BreakingPointsScreen.h
//  threadedImageLoaderExample
//
//  Created by Joel Gethin Lewis on 25/05/2011.
//  Copyright 2011 Hellicar&Lewis. All rights reserved.
//
#pragma once

#include "ofxThreadedImageLoader.h"
#include "ofMain.h"
#include "ofxOpenCv.h"

typedef struct{
    ofImage theImage;
    ofImage theWatershed;
    ofxCvContourFinder 	contourFinder;
	vector <ofImage> layers;
	vector <ofxCvColorImage> layerMasks;
    vector <float> layerFades;
    vector <float> fadeSpeeds;
    //stuff from watershed.cpp
    IplImage* marker_mask;
    IplImage* markers;
    IplImage* img0, *img, *img_gray, *wshed;
    CvPoint prev_pt;   
	CvRNG rng;    
    bool watershedDone;
    bool isStrobe;
} BreakingPointsImage;

class BreakingPointsScreen{
public:
	BreakingPointsScreen(){};
    ~BreakingPointsScreen(){};
    
    void    setup(string _name, ofPoint _position, ofPoint _size, ofxThreadedImageLoader* _loader);
    void    update();
    void    draw();
    void    drawCurrent();
	void    pickAndLoadNewImageAndWatershed();
    bool    isThreadRunning();
    bool    loaderFinishedLoading();
private:
    string  randomFileFromDirectory(ofDirectory* theDirectory);
    void    doWatershedAndLayers(BreakingPointsImage* theBreakingPointsImage);
    void    drawARandomLayer(BreakingPointsImage* theBreakingPointsImage);
    void    updateFadingLayers(BreakingPointsImage* theBreakingPointsImage, float timeElapsed);    
    void    drawFadedLayers(BreakingPointsImage* theBreakingPointsImage);
    
    string  name;
    ofPoint position;
    ofPoint size;

    BreakingPointsImage current;
    BreakingPointsImage next;
    
//    ofImage currentImage;
//    ofImage nextImage;
//    
//    ofImage currentWatershed;
//    ofImage nextWatershed;
    
    ofxThreadedImageLoader* loader;
    
    ofDirectory imageDirectory;
    ofDirectory watershedDirectory;
    
    bool    bWaiting;
    float   timeOfStartOfWait;
    float   waitPeriod;
    
    float timeOfLastFrame;
};