/*
Copyright(C) 2011-2012 MotionPortrait, Inc. All Rights Reserved.

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it.
*/

#include <math.h>
#include <float.h>
#include <assert.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include "app.h"

#include <android/log.h>

#include "motiport.h"
#include "util.h"
#include "gl_draw_layer.h"

#define BG_R (((float) 0) / 255)
#define BG_G (((float) 0) / 255)
#define BG_B (((float) 0) / 255)
#define IS_DIR           (2)
#define IS_FILE         (1)
#define NO_FILE_EXIST     (0)
#define MAX_PATH_LENGTH 2048

static struct mpRenderingContext rc;

static mpFace *aPhotoFace;
static mpHair *aHair = NULL ;
static mpGlasses *aGlasses = NULL ;
static mpHige *aBeard = NULL ;
static mpVoice voice ;

static struct mpTexture *texPhoto[MP_TEX_LAST];
static struct mpTexture *texHair[MP_TEX_HAIR_LAST];
static struct mpTexture *texGlasses[MP_TEX_GLASSES_LAST];
static mpTexture *glassesTexOpt[MP_TEX_GLASSESOPT_LAST];
static struct mpTexture *texBeard[MP_TEX_HIGE_LAST];

static unsigned char *imgPhoto[MP_IMG_LAST];

static int numExpr;

static int clicked = 0 ;
static char g_currentAction[64] = { "" };
static int g_touched = 0;
static int idxExpr ;

static const char *localDir = "/data/data/de.gui.avatar/files/";

static int face_idx =0;   // face pattern
static int face_max = 12;   // face count
static int hair_idx =0;   // hair pattern
static int hair_max =2;   // hair count
static char usrFace[MAX_PATH_LENGTH] = "";

static char localVoiceFile[MAX_PATH_LENGTH] = "";

// seigo
static float g_scale = 1.0f;

static void loadFace(int index);

static void fillAlpha(int w, int h);        // seigo

typedef struct {
    float x;
    float y;
    float scale;
} ViewScale;
static ViewScale sViewScale = { 0.0f, 0.0f, 1.0f };


// Called from the app framework.
void appInit()
{
    glClearColor(BG_R, BG_G, BG_B, 0.0f);   // seigo : clear alpha=0
    //glClearStencil(0);          // seigo : stencil 0

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mpContext ctxt = { 20, 20, 1, 1 };	// seigo5
    if (mpInit(&rc, &ctxt) == 0) {
        __android_log_print(ANDROID_LOG_INFO, "MP sample",
                        "Failed to initialize motion portrait library");
        return ;
    }

    char comm[2048] ;
    strcpy(comm, localDir);
    strcat(comm, "comparts");
    mpuSetCommonPartsDir(comm);

    // Create face model
    loadFace(face_idx);
}


// Called from the app framework.
void appDeinit()
{
    mpuCloseFace(aPhotoFace, texPhoto, imgPhoto);
    mpClose();
    aPhotoFace = NULL;
}

 static void checkAction();

// Called from the app framework.
/* The tick is current time in milliseconds, width and height
 * are the image dimensions to be rendered.
 */
void appRender(long tick, int width, int height)
{
    int screen = (width > height)? height:width ;

    // seigo
    screen = (int)(screen * g_scale);

    int sx = 0;
    int sy = 0;
    sx = (int)(screen * sViewScale.x);
    sy = (int)(screen * sViewScale.y);
    screen = (int)(screen * sViewScale.scale);

    if (!gAppAlive) return ;

    // seigo5 : delete stencil settings. just clear color
    glClear(GL_COLOR_BUFFER_BIT);

//    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);   // seigo clear stencil
//
//    glEnable(GL_STENCIL_TEST);  // seigo : enable stencil test
//    glStencilFunc(GL_ALWAYS, 1, ~0);    // seigo : drawn stencil is 1
//    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  // seigo

    if(g_touched == 0) checkAction();

    mpAnimate(aPhotoFace, tick);
    mpDraw(aPhotoFace, sx, sy, screen, screen);

    // seigo5 remove fillAlpha
    //fillAlpha(screen, screen); // seigo5
}

static void Hair()
{
    char hair[MAX_PATH_LENGTH] ;

    if(++hair_idx >= hair_max) hair_idx = 0;

    if(aHair != NULL) {
        mpUnsetHair(aPhotoFace, aHair);
        mpuCloseHair(aHair, texHair);
        aHair = NULL ;
    }

    if(hair_idx==1){    // if idx is 0, this means no hair
        strcpy(hair, localDir);
        strcat(hair, "hair");
        if ((aHair = mpuCreateHair(hair, texHair, &rc)) == NULL) {
            __android_log_print(ANDROID_LOG_INFO, "MP sample",
                        "Failed to load specified hair model");
        } else {
            mpSetHair(aPhotoFace, aHair);
            mpSetHairAdjust(aPhotoFace, aHair, 1.0, 1.0, 0.0, 0.05);
        }
    }
}

#define REFLECT_LENS (0)
#define COLOR_LENS (1)
#define MIRROR_LENS (2)

static void Glasses()
{

    static int lens ;
    static mpGlassesAdjust adjust = {
        1.0f,                   // scale
        {0.0f, 0.0f, 0.0f},     // rot
        {0.0f, 0.0f}            // ends of temple offset
    };
    static mpColor reflectCol = { 1.0, 1.0, 1.0, 0.5 } ;
    static mpColor ShadowCol = { 0.25, 0.1, 0.1, 0.5 } ;
    static mpColor color = {1.0, 0.0, 0.0, 0.5} ;

    char glasses[MAX_PATH_LENGTH] ;

    if(aGlasses == NULL) {
        // set color of reflection and shadow
        // you have to set them before calling mpuCreateGlasses
        mpuSetLensReflectColor(&reflectCol);
        mpuSetLensShadowColor(&ShadowCol);

        strcpy(glasses, localDir);
        strcat(glasses, "glasses");
        if ((aGlasses = mpuCreateGlasses(glasses, texGlasses, &rc)) == NULL)
            __android_log_print(ANDROID_LOG_INFO, "MP sample",
                        "Failed to load specified glasses model");
        else {
            mpSetGlasses(aPhotoFace, aGlasses, &adjust);
            lens = REFLECT_LENS ;
        }
    } else {
        strcpy(glasses, localDir);
        switch(lens) {
            case REFLECT_LENS:
                strcat(glasses, "glasses/col.png");
                mpuSetGlassesCol(aGlasses, glasses, glassesTexOpt, &rc);
                mpSetGlassesLensColor(aGlasses, &color);
                lens = COLOR_LENS ;
                break ;
            case COLOR_LENS:
                mpuCloseGlassesCol(aGlasses, glassesTexOpt);
                strcat(glasses, "glasses/land.png");
                mpuSetGlassesMirror(aGlasses, glasses, glassesTexOpt, &rc);
                mpSetGlassesMirrorAlpha(aGlasses, 0.5f);
                lens = MIRROR_LENS ;
                break ;
            case MIRROR_LENS:
                mpuCloseGlassesMirror(aGlasses, glassesTexOpt);
                mpSetGlasses(aPhotoFace, NULL, NULL);
                mpuCloseGlasses(aGlasses, texGlasses);
                aGlasses = NULL ;
                break ;
        }
    }
}

static void Beard()
{
    char beard[MAX_PATH_LENGTH] ;

    if(aBeard == NULL) {
        strcpy(beard, localDir);
        strcat(beard, "beard");
        if ((aBeard = mpuCreateHige(beard, texBeard, &rc)) == NULL)
            __android_log_print(ANDROID_LOG_INFO, "MP sample",
                        "Failed to load specified beard model");
        else
            mpSetHige(aPhotoFace, aBeard);
    } else {
        mpUnsetHige(aPhotoFace, aBeard);
        mpuCloseHige(aBeard, texBeard);
        aBeard = NULL ;
    }
}

static void CloseEye()
{
    static int flagCloseEye = 0;

    float close;
    if (flagCloseEye == 1) {
        flagCloseEye = 0;
        close = 0.0f;
    } else {
        flagCloseEye = 1;
        close = 0.5f;
    }
    mpCloseEye(aPhotoFace, 1000, close);
}

static void Speech()
{
    char voicestr[MAX_PATH_LENGTH] ;

    strcpy(voicestr, localDir);
    strcpy(voicestr, localVoiceFile);

    if( voice.buf != NULL ) {
        mpSpeakStop( aPhotoFace );
        mpuCloseVoice( &voice ) ;
    }

	__android_log_print(ANDROID_LOG_INFO, "MP sample", "Speech %s\n", voicestr);

    voice.gain = 0.3;

    if(!mpuCreateVoice(&voice, voicestr)) {
        __android_log_print(ANDROID_LOG_INFO, "MP sample",
                    "Failed to load %s\n", voicestr);
    } else {
        mpSpeak(aPhotoFace, &voice, 0, 0);
    }
}

static void DisplayExpression()
{
    int i;
    float gains[MP_MAX_NUM_EXPR];
    for (i = 0; i < numExpr; i++) gains[i] = 0.0f;
    gains[idxExpr++] = 1.0f;
    mpExpress(aPhotoFace, 1000, gains, 0.2f);
    if (idxExpr == numExpr) idxExpr = 0;
}

static void CancelExpression()
{
    int i;
    float gains[MP_MAX_NUM_EXPR];
    for (i = 0; i < numExpr; i++) gains[i] = 0.0f;
    mpExpress(aPhotoFace, 1000, gains, 1.0f);
    idxExpr = 0;
}

void loadFaces(int x) {
	loadFace(x);
}
static void loadFace(int index)
{
    char facestr[MAX_PATH_LENGTH] ;

    if(aHair) {
        mpuCloseHair(aHair, texHair);
        aHair = NULL ;
        hair_idx=0;
    }
    if(aGlasses) {
        mpuCloseGlasses(aGlasses, texGlasses);
        aGlasses = NULL ;
    }
    if(aBeard) {
        mpuCloseHige(aBeard, texBeard);
        aBeard = NULL ;
    }

    if(aPhotoFace)
        mpuCloseFace(aPhotoFace, texPhoto, imgPhoto);

    char str[50];

    sprintf(str, "working on face: %d", index);

    __android_log_print(ANDROID_LOG_INFO, "MP sample", str);

    sViewScale.x = 0.0f;
    sViewScale.y = 0.0f;
    sViewScale.scale = 1.0f;


    if(index == 1)
        sprintf(facestr, "%s/face/aiko.bin", localDir);
    else if(index == 0 || index == 4)
        sprintf(facestr, "%s/face/ani3000.bin", localDir);
    else if(index == 2)
    	sprintf(facestr, "%s/face/akira.bin", localDir);
    else if(index == 3){

    	//sViewScale.x = -0.25f;
    	//sViewScale.y = -0.5f;
    	//sViewScale.scale = 1.55f;
    	sprintf(facestr, "%s/face/andromeda.bin", localDir);

    }

    else if(index == 5)
    	    sprintf(facestr, "%s/face/claire.bin", localDir);
    else if(index == 6)
    	sprintf(facestr, "%s/face/eve.bin", localDir);
    else if(index == 7)
    	sprintf(facestr, "%s/face/grace.bin", localDir);
    else if(index == 8)
    	sprintf(facestr, "%s/face/justin.bin", localDir);
    else if(index == 9)
    	sprintf(facestr, "%s/face/kiki.bin", localDir);
    else if(index == 10)
    	sprintf(facestr, "%s/face/kit.bin", localDir);
    else if(index == 11)
    {
    	//sViewScale.x = -0.25f;
    	//sViewScale.y = -0.5f;
    	//sViewScale.scale = 1.5f;
    	sprintf(facestr, "%s/face/lisa.bin", localDir);
    }
    else if(index == 12)
    	sprintf(facestr, "%s/face/littlestar.bin", localDir);
    else if(index == 13)
    	sprintf(facestr, "%s/face/mario.bin", localDir);
    else if(index == 14)
    	sprintf(facestr, "%s/face/pip.bin", localDir);
    else if(index == 15)
    	sprintf(facestr, "%s/face/sayeko.bin", localDir);
    else if(index == 16)
    	sprintf(facestr, "%s/face/sue.bin", localDir);
    else if(index == 17)
    	sprintf(facestr, "%s/face/trinity.bin", localDir);
    else if(index == 18)
    	sprintf(facestr, "%s/face/jade.bin", localDir);
    else if(index == 19)
    	sprintf(facestr, "%s/face/uki.bin", localDir);
    else
        sprintf(facestr, "%s/face/ani3000.bin", localDir);

    aPhotoFace = mpuCreateFace(facestr, texPhoto, imgPhoto, &rc);

    if(aPhotoFace == NULL) {
        __android_log_print(ANDROID_LOG_INFO, "MP sample",
                    "Failed to load specified face model: %s\n", facestr);
        exit(1);
    }

    numExpr = mpGetFaceParami(aPhotoFace, MP_NUM_EXPR);
    mpSetAnimParamf(aPhotoFace,MP_NECK_X_MAX_ROT,2.0f);
    mpSetAnimParamf(aPhotoFace,MP_NECK_Y_MAX_ROT,2.0f);
    mpSetAnimParamf(aPhotoFace,MP_NECK_Z_MAX_ROT,0.3f);
    mpSetAnimParami(aPhotoFace, MP_PUPIL_ENABLE, 0);
}

static void Face()
{
    //face selectindex reset
    if(++face_idx >= face_max) face_idx = 0;
    loadFace(face_idx);
}

static void checkAction()
{
    if(g_currentAction[0] == '\0') return;

    if(strcmp(g_currentAction, "Hair")==0)
        Hair();
    else if(strcmp(g_currentAction, "Glasses")==0)
        Glasses();
    else if(strcmp(g_currentAction, "Beard")==0)
        Beard();
    else if(strcmp(g_currentAction, "Eye")==0)
        CloseEye();
    else if(strcmp(g_currentAction, "Speech")==0)
        Speech();
    else if(strcmp(g_currentAction, "Display Expression")==0)
        DisplayExpression();
    else if(strcmp(g_currentAction, "Cancel Expression")==0)
        CancelExpression();
    else if(strcmp(g_currentAction, "Change Avatar")==0)
        Face();
    g_currentAction[0] = '\0';
}

void appAction(const char *action)
{
    if(!action)
        g_currentAction[0] = '\0';
    else if(strcmp(action, "Speech")==0) {
       Speech();
        g_currentAction[0] = '\0';
    } else {
        strcpy(g_currentAction, action);
        __android_log_print(ANDROID_LOG_INFO, "MP sample", "appAction(): action=%s\n", g_currentAction);
    }
}

void appTouch()
{
    extern float gTouchX;
    extern float gTouchY;
    extern int  gWindowWidth;
    extern int  gWindowHeight;

    g_touched++;

    mpVector2    pos;
    pos.x = gTouchX/(float) gWindowWidth;
    pos.y = 1.2f - gTouchY/(float) gWindowHeight;

    mpLookAt(aPhotoFace, 300, &pos, 0.1f);
}

void appTouchMove()
{
    extern float gTouchX;
    extern float gTouchY;
    extern int  gWindowWidth;
    extern int  gWindowHeight;

    g_touched++;

    mpVector2    pos;
    pos.x = gTouchX/(float) gWindowWidth;
    pos.y = 1.2f - gTouchY/(float) gWindowHeight;

    mpLookAt(aPhotoFace, 0, &pos, 0.1f);
}

void appTouchFinish()
{
    g_touched = 0;

    mpVector2    pos;
    pos.x = 0.5f;
    pos.y = 0.5f;
    mpLookAt(aPhotoFace, 500, &pos, 1.0f);
}

// seigo
void appScale(float scale)
{
    g_scale = scale;
}


//
// register new face path
//
int setFaceInfo(const char *face)
{
    strcpy(usrFace, face);
    face_idx=1;
    face_max=3;
    return 0;
}

// seigo5 remove fillAlpha()

// seigo3
void speakWav( const char *buf )
{
	strcpy(localVoiceFile, buf);

	__android_log_print(ANDROID_LOG_INFO, "MP Sample", "Voice: %s", localVoiceFile);
}

