#include "testApp.h"

	//stuff from watershed.cpp
IplImage* marker_mask = 0;
IplImage* markers = 0;
IplImage* img0 = 0, *img = 0, *img_gray = 0, *wshed = 0;
IplImage* pureWshed = 0;
CvPoint prev_pt = {-1,-1};

//--------------------------------------------------------------
void testApp::setup(){
	originalImage.loadImage("breakingPointsTestImage.tif");

    cvColourImg.allocate(originalImage.getWidth(), originalImage.getHeight());
	cvGreyImage.allocate(originalImage.getWidth(), originalImage.getHeight());
	
	cvColourImg.setFromPixels(originalImage.getPixels(), originalImage.getWidth(), originalImage.getHeight());
	
	bShowWatershed = false;
	
		//from watershed.cpp
	rng = cvRNG(-1);
	
	img = cvCloneImage( cvColourImg.getCvImage() );
    img_gray = cvCloneImage( cvColourImg.getCvImage() );
    wshed = cvCloneImage( cvColourImg.getCvImage() );
	pureWshed = cvCloneImage(cvColourImg.getCvImage());
    marker_mask = cvCreateImage( cvGetSize(img), 8, 1 );
    markers = cvCreateImage( cvGetSize(img), IPL_DEPTH_32S, 1 );
    cvCvtColor( img, marker_mask, CV_BGR2GRAY );
    cvCvtColor( marker_mask, img_gray, CV_GRAY2BGR );
	
    cvZero( marker_mask );
    cvZero( wshed );
	
	
	inputDirList.setVerbose(false);
	inputDirList.reset();
	inputDirList.allowExt("tif");
	strLoadLocation = "imagery/";
	strSaveLocation = "watersheds/";
	numberInputImages = inputDirList.listDir(strLoadLocation);
	cout << "Number of images found: " << numberInputImages << endl;
	
	iFileIndex = 0;
	
	if(numberInputImages > 0){
		setupForNewImage(iFileIndex);		
	}else{
		cout << "No input images found!" << endl;
	}
	
	bShowSketch = false;
	
	bShowPure = false;
	
	tempToDrawWith.allocate(originalImage.getWidth(), originalImage.getHeight());
	tempToDrawWithGrey.allocate(originalImage.getWidth(), originalImage.getHeight());
}

//--------------------------------------------------------------
void testApp::update(){
	ofBackground(0,0,0);

		//cvGreyImage = cvColourImg;
	
	if(bShowWatershed){
		cvCopy(wshed, cvColourImg.getCvImage());
		cvColourImg.flagImageChanged();
		bShowWatershed = false;	
	}
	
	int imageWidth = originalImage.getWidth();
	int imageHeight = originalImage.getHeight();
	
	cvCopy( pureWshed, tempToDrawWith.getCvImage() );
		//tempToDrawWith.draw(0,0); not in draw any more!
	
		//nasty as hell but lets do some image processing here to get going fast
	
	tempToDrawWithGrey = tempToDrawWith;//converting automatically i hope
	
	tempToDrawWithGrey.contrastStretch(); //as much contrast as we can get
	
	tempToDrawWithGrey.dilate(); //stretch out the white borders
	
	tempToDrawWithGrey.invert();	//make them black
	
	tempToDrawWithGrey.threshold(1); //make all the grey white
	
		//tempToDrawWithGrey.draw(0,0); //draw the black and white image, not in draw any more!
	
	contourFinder.findContours(tempToDrawWithGrey, 20, 0.9f*(float)(imageWidth * imageHeight), 10, true, true);
	
		//cout << contourFinder.blobs.size() << " was the number of blobs" << endl;
	if(contourFinder.blobs.size() > 0){
		layers.resize(contourFinder.blobs.size());
		layerMasks.resize(contourFinder.blobs.size());
		for(int i=0; i< contourFinder.blobs.size(); i++){
			layers[i].allocate(imageWidth, imageHeight,OF_IMAGE_COLOR_ALPHA);
			layerMasks[i].allocate(imageWidth, imageHeight);
			layerMasks[i].drawBlobIntoMe(contourFinder.blobs[i], 255);
			layerMasks[i].flagImageChanged();
			
			unsigned char * pixelsSrc   = originalImage.getPixels();
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
				//delete[] pixelsSrc; 
				//delete[] pixelsMask; 
			delete[] pixelsFinal; 
		}
	}

}

//--------------------------------------------------------------
void testApp::draw(){
	ofSetColor(255,255,255);
	
	if(!bShowPure){
		cvColourImg.draw(0,0);
		
		if(bShowSketch){
			cvGreyImage.draw(0,0);
		}
	}else{
		ofEnableAlphaBlending();
		
			//contourFinder.draw(0,0);
			//tempToDrawWithGrey.draw(0,0);
		
		if(contourFinder.blobs.size() > 0){
			int randomLayer = (int)ofRandom(0, contourFinder.blobs.size());
				//layerMasks[randomLayer].draw(0,0); ///crazzzze effect rave on
			randomLayer %= contourFinder.blobs.size();//safety
			layers[randomLayer].draw(0,0);
		}
			
		ofDisableAlphaBlending();
		
			//originalImage.draw(0,0);		
	}
}


//--------------------------------------------------------------
void testApp::keyPressed  (int key){

	switch (key){
		case 'r':{ //resets the drawing
			cvZero( marker_mask );
			cvZero( cvGreyImage.getCvImage());//NOT CLEAR SETS TO ZERO cvGreyImage.clear();
			cvGreyImage.flagImageChanged();
			cvColourImg.setFromPixels(originalImage.getPixels(), originalImage.getWidth(), originalImage.getHeight());			
            cvCopy( cvColourImg.getCvImage(), img );
			break;			
		}
		case 'w':{ //does the watershed
			doWatershed();
			break;
		}
		case 's':{ //does and saves the watershed
			doWatershed(); //just in case it hasn't been done			
			saveWatershed();
			break;
		}
		case OF_KEY_UP:{ 
			iFileIndex++;
			iFileIndex %= numberInputImages;
			cout << "New File Index is" << iFileIndex << endl;
			setupForNewImage(iFileIndex);
			break;
		}	
		case OF_KEY_DOWN:{ 
			iFileIndex--;
			if(iFileIndex < 0){
				iFileIndex = numberInputImages-1;
			}
			cout << "New File Index is" << iFileIndex << endl;
			setupForNewImage(iFileIndex);
			break;
		}	
		case 'p':{ //does the watershed
			bShowSketch = !bShowSketch;
			break;
		}
		case 'x':{ //does the pure watershed
			bShowPure = !bShowPure;
			break;
		}			
			
	}
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    CvPoint pt = cvPoint(x,y);
	if( prev_pt.x < 0 )
		prev_pt = pt;
	cvLine( marker_mask, prev_pt, pt, cvScalarAll(255), 5, 8, 0 );
	cvLine( img, prev_pt, pt, cvScalarAll(255), 5, 8, 0 );
	cvLine( cvGreyImage.getCvImage(), prev_pt, pt, cvScalarAll(255), 5, 8, 0 );
	cvGreyImage.flagImageChanged();
	prev_pt = pt;
	
	/*	//CVAPI(void)  cvLine( CvArr* img, CvPoint pt1, CvPoint pt2,
						CvScalar color, int thickness CV_DEFAULT(1),
						int line_type CV_DEFAULT(8), int shift CV_DEFAULT(0) );
	*/
	
	//cvShowImage( "image", img );
	
	cvCopy(img, cvColourImg.getCvImage()); //for displaying
	cvColourImg.flagImageChanged();
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
	prev_pt = cvPoint(x,y);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    prev_pt = cvPoint(-1,-1);
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

void testApp::doWatershed(){
	CvMemStorage* storage = cvCreateMemStorage(0);
	CvSeq* contours = 0;
	CvMat* color_tab;
	int i, j, comp_count = 0;
	
	cvFindContours( marker_mask, storage, &contours, sizeof(CvContour),
				   CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE );
	cvZero( markers );
	
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
	
	{
		double t = (double)cvGetTickCount();
		
			//make sure you are always working on the original
		
		ofxCvColorImage theOriginal;
		
		theOriginal.allocate(originalImage.getWidth(), originalImage.getHeight());
		
		theOriginal.setFromPixels(originalImage.getPixels(), originalImage.getWidth(), originalImage.getHeight());
		
		cvWatershed( theOriginal.getCvImage(), markers );
		
		t = (double)cvGetTickCount() - t;
		printf( "exec time = %gms\n", t/(cvGetTickFrequency()*1000.) );
	}
	
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

	cvCopy(wshed, pureWshed); //keep a copy for looking at....
	cvAddWeighted( wshed, 0.5, img_gray, 0.5, 0, wshed ); //this is essentially an alpha blend
		//cvShowImage( "watershed transform", wshed );
	bShowWatershed = true;
	cvReleaseMemStorage( &storage );
	cvReleaseMat( &color_tab );	
}

bool testApp::fileExists(string absolutePath){
	File fileExistsTester(absolutePath);
	return fileExistsTester.exists();
}

void testApp::saveWatershed(){
	
		//still fucking up....
		//so lets draw into a another image too above
	ofImage	waterShedToSave;
//	
	waterShedToSave.allocate(originalImage.getWidth(), originalImage.getHeight(), OF_IMAGE_GRAYSCALE);

	waterShedToSave.setFromPixels(cvGreyImage.getPixels(), originalImage.getWidth(), originalImage.getHeight(), OF_IMAGE_GRAYSCALE);

		//	
//		//thanks kyle! you rule! memcpy ickckkkkkk
//	
//	memcpy(waterShedToSave.getPixels(), cvPtr1D(marker_mask, 0, CV_8UC1), waterShedToSave.getWidth()*waterShedToSave.getHeight());
//	
//		//string str_PathToSaveTo = strSaveLocation + "temp.png";
	string str_PathToSaveTo = strSaveLocation + inputDirList.getName(iFileIndex);
	
	waterShedToSave.saveImage(str_PathToSaveTo);
}

void testApp::setupForNewImage(int imageIndex){
	cvZero( marker_mask ); //zero out the mask to start with
    cvZero( wshed );
	cvZero( pureWshed );
	cvZero( cvGreyImage.getCvImage());//NOT CLEAR SETS TO ZERO cvGreyImage.clear();
	cvGreyImage.flagImageChanged();	//added reset extra here
	layers.clear();
	layerMasks.clear();	
	
		//now check to see if the file exists in the watersheds folder
	string str_PathToCheckForExistenceOfWatershed = strSaveLocation + inputDirList.getName(imageIndex);
	
	bool waterShedExists = fileExists(ofToDataPath(str_PathToCheckForExistenceOfWatershed, true)); //true means to get abs
	
	if(waterShedExists){
			//then we already have a file there, so load it in
		cout  << "Found pre existing watershed image " << endl;
		
		ofImage	existingWaterShed;
		
		existingWaterShed.allocate(originalImage.getWidth(), originalImage.getHeight(), OF_IMAGE_GRAYSCALE);
		
		existingWaterShed.loadImage(str_PathToCheckForExistenceOfWatershed);
		
		ofxCvGrayscaleImage cvGrayWater;
		
		cvGrayWater.allocate(originalImage.getWidth(), originalImage.getHeight());
		
		cvGrayWater.setFromPixels(existingWaterShed.getPixels(), originalImage.getWidth(), originalImage.getHeight());
		cvCopy(cvGrayWater.getCvImage(), marker_mask);
		cvCopy(cvGrayWater.getCvImage(), cvGreyImage.getCvImage());
		
	}else {
			//then we don't have an existing file, so you don't have to do anything.
	}
	
	
	originalImage.loadImage(strLoadLocation + inputDirList.getName(imageIndex));
	
	cvColourImg.setFromPixels(originalImage.getPixels(), originalImage.getWidth(), originalImage.getHeight());			
	cvCopy( cvColourImg.getCvImage(), img );
		//img = cvCloneImage( cvColourImg.getCvImage() );
    img_gray = cvCloneImage( cvColourImg.getCvImage() );
    wshed = cvCloneImage( cvColourImg.getCvImage() );
	pureWshed = cvCloneImage(cvColourImg.getCvImage());
}