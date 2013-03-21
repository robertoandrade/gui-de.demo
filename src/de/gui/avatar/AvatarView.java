package de.gui.avatar;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL;

import android.content.Context;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.SurfaceTexture;
import android.opengl.GLUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.TextureView;
import android.widget.FrameLayout;
import de.gui.avatar.RenderThread.RenderingCallback;

public class AvatarView extends FrameLayout implements TextureView.SurfaceTextureListener {

	private static final String NATIVE_LIB_NAME = "sample";
	private static final String TAG = AvatarView.class.getSimpleName();

	public AvatarView(Context context, AttributeSet attrs) {
		super(context, attrs);
		
		initView(context);
	}
	
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
	    super.onMeasure(widthMeasureSpec, heightMeasureSpec);

	    int parentHeight = (int)(MeasureSpec.getSize(heightMeasureSpec) * .97); 
	    this.setMeasuredDimension(parentHeight, parentHeight);
	}
	
	private void initView(Context context) 
	{
        mAvatarView = new TextureView(context);
        mAvatarView.setSurfaceTextureListener(this);
        
        mAvatarView.setOpaque(false);
        
        this.addView(mAvatarView);
        
      	// load in avatar assets  
        copy2Local("comparts");
        copy2Local("face");
        copy2Local("hair");
        copy2Local("glasses");
        copy2Local("beard");
        copy2Local("voice");
	}
	
	/**
	 * plays the audio file
	 * 
	 * @param uripath string to the audio file
	 */
	public void lipSyncLoad(String uripath)
	{
		int ret = DemoRenderer.nativeLipSynchUri(uripath);	// seigo5
		if (ret != 0) {
			Log.e(TAG, "nativeLipSynchUri failed : ret=" + ret);
		}
		
	}
	
	public void lipSyncStart()
	{
		DemoRenderer.nativeLipSynchStart();
	}
	
	/**
	 * Change the avatar
	 * @param faceIndex
	 */
	public void changeAvatar(int faceIndex)
	{
		mRenderThread.changeFace(faceIndex);
	}
	
	/**
     * Loads in the avatar assets
     * @param dir asset directory name to load
     */
    private void copy2Local(String dir)
    {
    	String[] fileList;
		try {
			fileList = getResources().getAssets().list(dir);
		
	        if(fileList == null || fileList.length == 0){
	            return;
	        }
	        
	        File odir = new File(getContext().getFilesDir(), dir);
	        if(odir.exists()) {
	        	return;
	        } else {
	            odir.mkdir();
	        }
	
	        AssetManager as = getResources().getAssets();
	
	        for(String file : fileList) {
	
	            InputStream istr;
				try {
					istr = as.open(dir + "/" + file);
					
					File ofile = new File(odir, file);
		            OutputStream ostr = new FileOutputStream(ofile);
		            Log.i(TAG, "copying to local : " + dir + "/" + file);
	
		            int size;
		            while ((size = istr.available()) > 0) {
		                byte buf[] = new byte[Math.min(size, 4096)];
		                istr.read(buf);
		                ostr.write(buf);
		            }
		            istr.close();
		            ostr.close();
					
				} catch (IOException e) {
					Log.e(TAG, "Could not load file:" + e.getMessage());
				}
	            
	        }
        
		} catch (IOException e1) {
			Log.e(TAG, "Could not load local assets:" + e1.getMessage());
		}
    }

	private static native void nativeTouch(float x, float y);
    private static native void nativeTouchMove(float x, float y);
    private static native void nativeTouchFinish();

    public static native byte[] decodeString(String a);
    public static native String encodeData(byte[] a);

    private static native void nativeOnClick(String s);
    private static native int setFaceInfo(String newPath);

    public static String Avatar_encodeData(byte[] data){
        return encodeData(data);
    }
    public static  byte[] Avatar_decodeString(String dataString){
        byte[] ret;
        ret=decodeString(dataString);
        return ret;
    }
    
    private RenderThread mRenderThread;
    private TextureView mAvatarView;
    private RenderingCallback callback;

	@Override
	public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
		Log.i(TAG, "Starting RenderThread...");
		
        mRenderThread = new RenderThread(getResources(), surface, width, height); //TODO: needs to be 512
        
        mRenderThread.setCallback(callback);
        
        mRenderThread.start();
    }

	@Override
	public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
		mRenderThread.finish();
		return false;
	}

	@Override
	public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
	}

	@Override
	public void onSurfaceTextureUpdated(SurfaceTexture surface) {
		
	}
	
	static {
        System.loadLibrary(NATIVE_LIB_NAME);
    }
	
	public void setRenderingCallback(RenderingCallback callback) {
		Log.i(TAG, "Setting callback...");
		this.callback = callback;
	}
	
}

class RenderThread extends Thread {
	
    private static final String LOG_TAG = "GLTextureView";

    static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    static final int EGL_OPENGL_ES2_BIT = 4;

    private volatile boolean mFinished;

    private volatile int mFaceIdx = -1;		// seigo2
    
    private final Resources mResources;
    private final SurfaceTexture mSurface;
    
    private EGL10 mEgl;
    private EGLDisplay mEglDisplay;
    private EGLConfig mEglConfig;
    private EGLContext mEglContext;
    private EGLSurface mEglSurface;
    private GL mGL;
    private int _width;
    private int _height;
    
    private RenderingCallback callback;
    
    RenderThread(Resources resources, SurfaceTexture surface, int width, int height) {
    	
    	Log.i("RENDERTHREAD","RENDERTHREAD:" + width + ":" + height);
    	
        mResources = resources;
        mSurface = surface;
        _width = width;
        _height = height;
    }
    
    public void setCallback(RenderingCallback callback) {
    	this.callback = callback;
    }

    @Override
    public void run() {
    	Log.d("RENDERTHREAD","run");
        initGL();
        Log.d("RENDERTHREAD","initGL");
        DemoRenderer.nativeInit();
        Log.d("RENDERTHREAD","nativeInit");
        DemoRenderer.nativeResize(_width, _height);
        Log.d("RENDERTHREAD","nativeResize");
        
        boolean rendered = false;
        
        while (!mFinished) {
            checkCurrent();
            
            if(mFaceIdx >= 0) {
            	DemoRenderer.nativeLoadFaceNumber(mFaceIdx);
            	mFaceIdx = -1;
            }
            
            DemoRenderer.nativeRender();

            if (!mEgl.eglSwapBuffers(mEglDisplay, mEglSurface)) {
                throw new RuntimeException("Cannot swap buffers");
            }
            checkEglError();
            
            if (!rendered) {
                Log.i("RENDERTHREAD", "Done rendering...");
                if (callback != null) {
                	callback.onRendered();
                }
            }
            
            rendered = true;

            try {
                Thread.sleep(20);
            } catch (InterruptedException e) {
                // Ignore
            }
        }

        DemoRenderer.nativeDone();
        Log.d("RENDERTHREAD","nativeDone");
        finishGL();
        Log.d("RENDERTHREAD","finishGL");
    }

    private void checkEglError() {
        int error = mEgl.eglGetError();
        if (error != EGL10.EGL_SUCCESS) {
            Log.w(LOG_TAG, "EGL error = 0x" + Integer.toHexString(error));
        }
    }

    private void finishGL() {
        mEgl.eglDestroyContext(mEglDisplay, mEglContext);
        mEgl.eglDestroySurface(mEglDisplay, mEglSurface);
    }

    private void checkCurrent() {
        if (!mEglContext.equals(mEgl.eglGetCurrentContext()) ||
                !mEglSurface.equals(mEgl.eglGetCurrentSurface(EGL10.EGL_DRAW))) {
            if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
                throw new RuntimeException("eglMakeCurrent failed "
                        + GLUtils.getEGLErrorString(mEgl.eglGetError()));
            }
        }
    }
    
    private void initGL() {
    	
        mEgl = (EGL10) EGLContext.getEGL();
        
        mEglDisplay = mEgl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        if (mEglDisplay == EGL10.EGL_NO_DISPLAY) {
            throw new RuntimeException("eglGetDisplay failed "
                    + GLUtils.getEGLErrorString(mEgl.eglGetError()));
        }
        
        int[] version = new int[2];
        if (!mEgl.eglInitialize(mEglDisplay, version)) {
            throw new RuntimeException("eglInitialize failed " +
                    GLUtils.getEGLErrorString(mEgl.eglGetError()));
        }

        mEglConfig = chooseEglConfig();
        if (mEglConfig == null) {
            throw new RuntimeException("eglConfig not initialized");
        }
        
        Log.i(LOG_TAG, "eglConfig = " + mEglConfig);
        mEglContext = createContext(mEgl, mEglDisplay, mEglConfig);
        if (mEglContext == EGL10.EGL_NO_CONTEXT) {
        	Log.e(LOG_TAG, "createContext failed. err=" + mEgl.eglGetError());
        }

        mEglSurface = mEgl.eglCreateWindowSurface(mEglDisplay, mEglConfig, mSurface, null);

        if (mEglSurface == null || mEglSurface == EGL10.EGL_NO_SURFACE) {
            int error = mEgl.eglGetError();
            if (error == EGL10.EGL_BAD_NATIVE_WINDOW) {
                Log.e(LOG_TAG, "createWindowSurface returned EGL_BAD_NATIVE_WINDOW.");
                return;
            }
            throw new RuntimeException("createWindowSurface failed "
                    + GLUtils.getEGLErrorString(error));
        }

        if (!mEgl.eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext)) {
            throw new RuntimeException("eglMakeCurrent failed "
                    + GLUtils.getEGLErrorString(mEgl.eglGetError()));
        }

        mGL = mEglContext.getGL();
    }
    

    EGLContext createContext(EGL10 egl, EGLDisplay eglDisplay, EGLConfig eglConfig) {
        int[] attrib_list = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL10.EGL_NONE };
        return egl.eglCreateContext(eglDisplay, eglConfig, EGL10.EGL_NO_CONTEXT, attrib_list);            
    }

    private EGLConfig chooseEglConfig() {
        int[] configsCount = new int[1];
        EGLConfig[] configs = new EGLConfig[1];
        int[] configSpec = getConfig();
        if (!mEgl.eglChooseConfig(mEglDisplay, configSpec, configs, 1, configsCount)) {
            throw new IllegalArgumentException("eglChooseConfig failed " +
                    GLUtils.getEGLErrorString(mEgl.eglGetError()));
        } else if (configsCount[0] > 0) {
            return configs[0];
        }
        return null;
    }
    
    private int[] getConfig() {
        return new int[] {
                //EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL10.EGL_RED_SIZE, 8,
                EGL10.EGL_GREEN_SIZE, 8,
                EGL10.EGL_BLUE_SIZE, 8,
                EGL10.EGL_ALPHA_SIZE, 8,
                EGL10.EGL_DEPTH_SIZE, 0,
                EGL10.EGL_STENCIL_SIZE, 0,		// seigo5
//                 EGL10.EGL_SAMPLE_BUFFERS, 1,	// MSAA
//                 EGL10.EGL_SAMPLES, 4,		// MSAA
                EGL10.EGL_NONE
        };
    }

    void finish() {
        mFinished = true;
    }
    
    public void changeFace(int faceIdx) {
    	mFaceIdx = faceIdx;
    }
    
    public static interface RenderingCallback {
    	public void onRendered();
    }
}

class DemoRenderer {

	public static native void nativeLoadFaceNumber(int x);
	public static native void nativeInit();
	public static native void nativeResize(int w, int h);
	public static native void nativeRender();
	public static native void nativeDone();
	
    public static native int nativeLipSynchUri(String uri);
    public static native int nativeLipSynchStart();

}

