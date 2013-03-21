/*
Copyright(C) 2011 MotionPortrait, Inc. All Rights Reserved.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it.
*/

#ifndef APP_H_INCLUDED
#define APP_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif



// The simple framework expects the application code to define these functions.
extern void appInit();
extern void appDeinit();
extern void appRender(long tick, int width, int height);
//extern void appItem();
extern void appAction( const char *action );
//extern void appTouch( float x, float y );
extern void appTouch();
extern void appTouchMove();
extern void appTouchFinish();
extern void appScale(float scale);  // seigo
extern void loadFaces(int x);



/* Value is non-zero when application is alive, and 0 when it is closing.
 * Defined by the application framework.
 */
extern int gAppAlive;

extern int setFaceInfo( const char *  );
extern int setHairInfo( const char *  );


// seigo3

typedef struct _MyHeader {
	int wChannels;
	int dwSamplesPerSec;
	int dwAvgBytesPerSec;
	int wBlockAlign;
	int wBitsPerSample;
	int data_size;
} MyHeader;

extern void speakWav ( const char *buf );
extern int speakStart ();


#ifdef __cplusplus
}
#endif


#endif // !APP_H_INCLUDED

#define LOG_TAG "GuideNativeBridge"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , LOG_TAG, __VA_ARGS__)

