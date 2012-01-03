/*
 *  AppGlobals.h
 *  emptyExample
 *
 *  Created by Todd Vanderlin on 3/30/10.
 *  Copyright 2010 Interactive Design. All rights reserved.
 *
 */

#pragma once
#include "ofxXmlSettings.h"

class AppSettings {

protected:
	ofxXmlSettings xml;
public:
	
	int APP_W;
	int APP_H;
	
	AppSettings() {
	}
	
	void loadSettings(string file) {
		if(xml.loadFile("app_settings.xml")) {
			APP_W = xml.getValue("APP_W", 1280);
			APP_H = xml.getValue("APP_H", 800);
		}
		
	}
};


// so other apps can see it...
extern AppSettings settings;