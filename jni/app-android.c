/*
Copyright(C) 2011 MotionPortrait, Inc. All Rights Reserved.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it.
*/

#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>
#include <stdint.h>
#include "Base64Transcoder.h"
#include "app.h"

// seigo3
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>


int   gAppAlive   = 1;

int  gWindowWidth  = 512;
int  gWindowHeight = 512;
float gTouchX;
float gTouchY;

static long sTimeOffset   = 0;
static int  sTimeOffsetInit = 0;
static long sTimeStopped  = 0;

// seigo3
static int sVoiceFd = -1;


static long
_getTime(void)
{
    struct timeval  now;

    gettimeofday(&now, NULL);
    return (long)(now.tv_sec*1000 + now.tv_usec/1000);
}

/* Call to initialize the graphics state */
void
Java_de_gui_avatar_DemoRenderer_nativeInit( JNIEnv*  env )
{
    appInit();
    gAppAlive    = 1;
    sTimeOffsetInit = 0;
}

void
Java_de_gui_avatar_DemoRenderer_nativeResize( JNIEnv*  env, jobject  thiz, jint w, jint h )
{
	gWindowWidth  = w;
    gWindowHeight = h;
}

/* Call to finalize the graphics state */
void
Java_de_gui_avatar_DemoRenderer_nativeDone( JNIEnv*  env )
{
	appDeinit();
}
/* seigo : for SurfaceView scale animation */
void
Java_de_gui_avatar_DemoRenderer_nativeAnimateScreenSize( JNIEnv*  env, jobject thiz, jfloat jscale )
{
    appScale(jscale);
}


jint
Java_de_gui_avatar_DemoRenderer_nativeLipSynchUri( JNIEnv*  env, jobject thiz, jstring uri )
{
	int ret = 0;

    // convert Java string to UTF-8
    const jbyte *utf8 = (*env)->GetStringUTFChars(env, uri, NULL);

    char path[128];
	sprintf(path, "%s", utf8);

    __android_log_print(ANDROID_LOG_INFO, "GuideNative", "Loading lip sync: %s", path);

	speakWav(path);

    (*env)->ReleaseStringUTFChars(env, uri, utf8);

	return ret;
}

jint
Java_de_gui_avatar_DemoRenderer_nativeLipSynchStart( JNIEnv*  env, jobject thiz )
{
	int ret = 0;

    __android_log_print(ANDROID_LOG_INFO, "GuideNative", "Starting lip sync...");

    appAction("Speech");

	return ret;
}

/* This is called to indicate to the render loop that it should
 * stop as soon as possible.
 */
void
Java_de_gui_avatar_AvatarView_nativeTouch( JNIEnv*  env, jobject  thiz, jfloat x, jfloat y )
{
	gTouchX = x;
	gTouchY = y;
	appTouch() ;
}

void
Java_de_gui_avatar_AvatarView_nativeTouchMove( JNIEnv*  env, jobject  thiz, jfloat x, jfloat y )
{
	gTouchX = x;
	gTouchY = y;
	appTouchMove() ;
}

void
Java_de_gui_avatar_AvatarView_nativeTouchFinish( JNIEnv*  env )
{
	appTouchFinish() ;
}

/* Call to render the next GL frame */
void
Java_de_gui_avatar_DemoRenderer_nativeRender( JNIEnv*  env )
{
    long   curTime;

	curTime = _getTime() + sTimeOffset;
	if (sTimeOffsetInit == 0) {
		sTimeOffsetInit = 1;
		sTimeOffset     = -curTime;
		curTime         = 0;
	}

    appRender(curTime, gWindowWidth, gWindowHeight);
}

void
Java_de_gui_avatar_AvatarView_nativeOnClick( JNIEnv*  env, jobject  thiz, jstring button )
{
	const char *action = (*env)->GetStringUTFChars(env, button, NULL);
	appAction( action );
	(*env)->ReleaseStringUTFChars(env, button, action);
}

jbyteArray Java_de_gui_avatar_Avatar_decodeString( JNIEnv* env,
                                                  jobject thiz ,
                                                  jstring inStr)
{
	jboolean jbool;
	bool bRet;
	jbyteArray result;
	const char *cpStr;
	int len;
	int outLen;
	jbyte* bpOutData;
	jbyte* dst;
	int i;

	cpStr= (*env)->GetStringUTFChars(env, inStr, NULL);;

	if (cpStr == NULL) return NULL;
	len = (int)(*env)->GetStringUTFLength(env, inStr);

	outLen=(int) EstimateBas64DecodedDataSize(len);
	bpOutData=calloc(1,outLen);
	if(bpOutData!=NULL)
	{


		bRet=Base64DecodeData(cpStr, len, bpOutData, &outLen);
		if(bRet==false)
		{
			result=NULL;
		}
		else
		{
			result= (*env)->NewByteArray(env, outLen);
		    if (result == NULL) {
		    	(*env)->ReleaseStringUTFChars(env, inStr, cpStr);
		    	free(bpOutData);
		        return NULL;
		    }
		    dst = (*env)->GetByteArrayElements(env, result, NULL);
		    if (dst == NULL) {
		        (*env)->ReleaseStringUTFChars(env, inStr, cpStr);
		        free(bpOutData);
		        return NULL;
		    }
		    for(i = 0; i < outLen; i++){
		        dst[i] = bpOutData[i];
		    }
		    (*env)->ReleaseByteArrayElements(env, result, dst, 0);
		}
		free(bpOutData);

	}
	else
	{

	}

	(*env)->ReleaseStringUTFChars(env, inStr, cpStr);


	return result;
}


// encode to base64
jstring Java_de_gui_avatar_Avatar_encodeData( JNIEnv* env,
                                                  jobject thiz ,
                                                  jbyteArray  inStr)
{
	jbyte  * bpStr;
	jboolean b;
	jstring jsRet;
	int len;
	int outLen;
	bool bRet;
	char * cpOutStr;

	bpStr=(*env)->GetByteArrayElements(env, inStr, &b);
    len = (int)(*env)->GetArrayLength(env,inStr);
    outLen=(int)EstimateBas64EncodedDataSize(len);

    cpOutStr=calloc(1,outLen+1);
    if(cpOutStr==NULL)
    {
    	jsRet=(*env)->NewStringUTF(env, "");
    }
    else
    {
    	// encode
		bRet=Base64EncodeData(bpStr,len, cpOutStr, &outLen);
		if(bRet==true)
		{
			jsRet=(*env)->NewStringUTF(env, cpOutStr);
		}else
		{
			jsRet=(*env)->NewStringUTF(env, "");
		}

		free(cpOutStr);
    }

    (*env)->ReleaseByteArrayElements(env, inStr, bpStr,0);

    return jsRet;
}

void
Java_de_gui_avatar_DemoRenderer_nativeLoadFaceNumber(JNIEnv*  env, jobject  thiz, jint x)
{
	loadFaces(x);
}


jint Java_de_gui_avatar_AvatarView_setFaceInfo( JNIEnv* env,
                                                  jobject thiz ,
                                                  jstring  newStr)
{
	int ret;
	 const char *src;

	 	src = (*env)->GetStringUTFChars(env, newStr, NULL);
	    if (src == NULL){
	    	return -1;
	    }

	    ret=setFaceInfo(src);

	    (*env)->ReleaseStringUTFChars(env, newStr, src);
	   return ret;
}
