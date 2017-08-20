/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.


Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/

package com.mavoar.markers;

import java.lang.ref.WeakReference;
import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.graphics.Color;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.DisplayMetrics;
import android.view.GestureDetector;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.CheckBox;
import android.widget.RelativeLayout;
import android.widget.Switch;
import android.widget.Toast;

import android.util.Log;
import com.mavoar.R;
import com.mavoar.markers.dataset.Marker;
import com.mavoar.renderer.GLRenderer;
import com.mavoar.renderer.VuforiaSampleGLView;
import com.mavoar.utils.DebugLog;
import com.vuforia.Vuforia;
import com.vuforia.INIT_ERRORCODE;
import com.vuforia.INIT_FLAGS;
import android.graphics.Bitmap;
import android.widget.ImageView;


import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.android.Utils;
import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.imgproc.Imgproc;

import java.io.File;
import java.io.FileOutputStream;

import static com.vuforia.CameraDevice.CAMERA_DIRECTION.CAMERA_DIRECTION_DEFAULT;

/** The main activity for the ImageTargets sample. */
public class ImageTargets
{
    //dataset variables
    private String dataset_key;
    private String dataset_name;
    private String dataset_obj;
    private String dataset_mtl;
    private String dataset_xml;
    private String dataset_folder;
    private float dataset_scale;
    private int dataset_mode;
    private ArrayList<Marker> dataset_markers;
    private boolean mvo;

    private double ratio;

    Activity activity;
    // Focus mode constants:
    private static final int FOCUS_MODE_NORMAL = 0;
    private static final int FOCUS_MODE_CONTINUOUS_AUTO = 1;

    // Application status constants:
    private static final int APPSTATUS_UNINITED = -1;
    private static final int APPSTATUS_INIT_APP = 0;
    private static final int APPSTATUS_INIT_VUFORIA = 1;
    private static final int APPSTATUS_INIT_TRACKER = 2;
    private static final int APPSTATUS_INIT_APP_AR = 3;
    private static final int APPSTATUS_LOAD_TRACKER = 4;
    private static final int APPSTATUS_INITED = 5;
    private static final int APPSTATUS_CAMERA_STOPPED = 6;
    private static final int APPSTATUS_CAMERA_RUNNING = 7;

    // Constants for Hiding/Showing Loading dialog
    static final int HIDE_LOADING_DIALOG = 0;
    static final int SHOW_LOADING_DIALOG = 1;

    private View mLoadingDialogContainer;

    // Our OpenGL view:
    private VuforiaSampleGLView mGlView;

    // Our renderer:
    private GLRenderer mRenderer;

    // Display size of the device:
    private int mScreenWidth = 0;
    private int mScreenHeight = 0;

    // Constant representing invalid screen orientation to trigger a query:
    private static final int INVALID_SCREEN_ROTATION = -1;

    private boolean et=false;

    // Last detected screen rotation:
    private int mLastScreenRotation = INVALID_SCREEN_ROTATION;

    // Keeps track of the current camera
    int mCurrentCamera = CAMERA_DIRECTION_DEFAULT;

    // The current application status:
    private int mAppStatus = APPSTATUS_UNINITED;

    // The async tasks to initialize the Vuforia SDK:
    private InitVuforiaTask mInitVuforiaTask;
    private LoadTrackerTask mLoadTrackerTask;

    // An object used for synchronizing Vuforia initialization, dataset loading
    // and
    // the Android onDestroy() life cycle event. If the application is destroyed
    // while a data set is still being loaded, then we wait for the loading
    // operation to finish before shutting down Vuforia:
    private Object mShutdownLock = new Object();

    // Vuforia initialization flags:
    private int mVuforiaFlags = 0;

    // Contextual Menu Options for Camera Flash - Autofocus
    private boolean mFlash = false;

    private RelativeLayout mUILayout;

    boolean mIsDroidDevice = false;

    /**
     * Creates a handler to update the status of the Loading Dialog from an UI
     * Thread
     */
    public static ImageView grayscale;

    static class LoadingDialogHandler extends Handler
    {
        private final WeakReference<ImageTargets> mImageTargets;


        LoadingDialogHandler(ImageTargets imageTargets)
        {
            mImageTargets = new WeakReference<ImageTargets>(imageTargets);
        }


        public void handleMessage(Message msg)
        {
            ImageTargets imageTargets = mImageTargets.get();
            if (imageTargets == null)
            {
                return;
            }

            if (msg.what == SHOW_LOADING_DIALOG)
            {
                imageTargets.mLoadingDialogContainer
                    .setVisibility(View.VISIBLE);

            } else if (msg.what == HIDE_LOADING_DIALOG)
            {
                imageTargets.mLoadingDialogContainer.setVisibility(View.GONE);
            }
        }
    }

    private Handler loadingDialogHandler = new LoadingDialogHandler(this);

    /** An async task to initialize Vuforia asynchronously. */
    private class InitVuforiaTask extends AsyncTask<Void, Integer, Boolean>
    {
        // Initialize with invalid value:
        private int mProgressValue = -1;


        protected Boolean doInBackground(Void... params)
        {
            // Prevent the onDestroy() method to overlap with initialization:
            synchronized (mShutdownLock)
            {

                Vuforia.setInitParameters(activity, mVuforiaFlags, dataset_key);

                do
                {
                    mProgressValue = Vuforia.init();
                    publishProgress(mProgressValue);
                } while (!isCancelled() && mProgressValue >= 0
                    && mProgressValue < 100);

                return (mProgressValue > 0);
            }
        }


        protected void onProgressUpdate(Integer... values)
        {
            // Do something with the progress value "values[0]", e.g. update
            // splash screen, progress bar, etc.
        }


        protected void onPostExecute(Boolean result)
        {
            // Done initializing Vuforia, proceed to next application
            // initialization status:
            if (result)
            {
                DebugLog.LOGD("InitVuforiaTask::onPostExecute: Vuforia "
                    + "initialization successful");

                updateApplicationStatus(APPSTATUS_INIT_TRACKER);
            } else
            {
                // Create dialog box for display error:
                AlertDialog dialogError = new AlertDialog.Builder(
                        activity).create();

                dialogError.setButton(DialogInterface.BUTTON_POSITIVE, "OK",
                    new DialogInterface.OnClickListener()
                    {
                        public void onClick(DialogInterface dialog, int which)
                        {
                            // Exiting application:
                            System.exit(1);
                        }
                    });

                String logMessage;

                // NOTE: Check if initialization failed because the device is
                // not supported. At this point the user should be informed
                // with a message.
                logMessage = getInitializationErrorString(mProgressValue);

                // Log error:
                DebugLog.LOGE("InitVuforiaTask::onPostExecute: " + logMessage
                    + " Exiting.");

                // Show dialog box with error message:
                dialogError.setMessage(logMessage);
                dialogError.show();
            }
        }
    }

    // Returns the error message for each error code
    private String getInitializationErrorString(int code)
    {
        if (code == INIT_ERRORCODE.INIT_DEVICE_NOT_SUPPORTED)
            return activity.getString(R.string.INIT_ERROR_DEVICE_NOT_SUPPORTED);
        if (code == INIT_ERRORCODE.INIT_NO_CAMERA_ACCESS)
            return activity.getString(R.string.INIT_ERROR_NO_CAMERA_ACCESS);
        if (code == INIT_ERRORCODE.INIT_LICENSE_ERROR_MISSING_KEY)
            return activity.getString(R.string.INIT_LICENSE_ERROR_MISSING_KEY);
        if (code == INIT_ERRORCODE.INIT_LICENSE_ERROR_INVALID_KEY)
            return activity.getString(R.string.INIT_LICENSE_ERROR_INVALID_KEY);
        if (code == INIT_ERRORCODE.INIT_LICENSE_ERROR_NO_NETWORK_TRANSIENT)
            return activity.getString(R.string.INIT_LICENSE_ERROR_NO_NETWORK_TRANSIENT);
        if (code == INIT_ERRORCODE.INIT_LICENSE_ERROR_NO_NETWORK_PERMANENT)
            return activity.getString(R.string.INIT_LICENSE_ERROR_NO_NETWORK_PERMANENT);
        if (code == INIT_ERRORCODE.INIT_LICENSE_ERROR_CANCELED_KEY)
            return activity.getString(R.string.INIT_LICENSE_ERROR_CANCELED_KEY);
        if (code == INIT_ERRORCODE.INIT_LICENSE_ERROR_PRODUCT_TYPE_MISMATCH)
            return activity.getString(R.string.INIT_LICENSE_ERROR_PRODUCT_TYPE_MISMATCH);
        else
        {
            return activity.getString(R.string.INIT_LICENSE_ERROR_UNKNOWN_ERROR);
        }
    }


    /** An async task to load the tracker data asynchronously. */
    private class LoadTrackerTask extends AsyncTask<Void, Integer, Boolean>
    {
        protected Boolean doInBackground(Void... params)
        {
            // Prevent the onDestroy() method to overlap:
            synchronized (mShutdownLock)
            {
                // Load the tracker data set:
                return (loadTrackerData(dataset_xml) > 0);
            }
        }

        protected void onPostExecute(Boolean result)
        {
            DebugLog.LOGD("LoadTrackerTask::onPostExecute: execution "
                + (result ? "successful" : "failed"));

            if (result)
            {
                // Done loading the tracker, update application status:
                updateApplicationStatus(APPSTATUS_INITED);
            } else
            {
                // Create dialog box for display error:
                AlertDialog dialogError = new AlertDialog.Builder(
                        activity).create();

                dialogError.setButton(DialogInterface.BUTTON_POSITIVE, "Close",
                    new DialogInterface.OnClickListener()
                    {
                        public void onClick(DialogInterface dialog, int which)
                        {
                            // Exiting application:
                            System.exit(1);
                        }
                    });

                // Show dialog box with error message:
                dialogError.setMessage("Failed to load tracker data.");
                dialogError.show();
            }
        }
    }

    /** Stores screen dimensions */
    private void storeScreenDimensions()
    {
        // Query display dimensions:
        DisplayMetrics metrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mScreenWidth = metrics.widthPixels;
        mScreenHeight = metrics.heightPixels;
    }


    /**
     * Called when the activity first starts or the user navigates back to an
     * activity.
     */
    public ImageTargets(Bundle args,Activity activity,double ratio,boolean mvo){
        
        this.activity = activity;
        this.ratio = ratio;
        this.mvo = mvo;
        dataset_key = args.getString("key");
        dataset_mode = args.getInt("mode");

        dataset_name = args.getString("name");
        dataset_obj = args.getString("obj");
        dataset_mtl = args.getString("mtl");
        dataset_xml = args.getString("xml");
        dataset_folder = args.getString("folder");
        dataset_scale = args.getFloat("scale");
        dataset_markers = (ArrayList<Marker>) args.getSerializable("markers");


        // Configure Vuforia to use OpenGL ES 2.0
        mVuforiaFlags = INIT_FLAGS.GL_20;

        // Update the application status to start initializing application:
        updateApplicationStatus(APPSTATUS_INIT_APP);

        mIsDroidDevice = android.os.Build.MODEL.toLowerCase().startsWith(
                "droid");
    }

    /** Native tracker initialization and deinitialization. */
    public native int initTracker(AssetManager c, String pathToInternalDir,
                                  String obj,String mtl,String xml,String folder,float scale, int markerNum,
                                  String[] markerNames,float[] markerRot,float[] markerTra, float[] markerSca,boolean mvo,int mode);

    public native void deinitTracker();
    /** Native functions to load and destroy tracking data. */
    public native int loadTrackerData(String xml);
    public native void destroyTrackerData();
    /** Native sample initialization. */
    public native void onVuforiaInitializedNative();
    /** Native methods for starting and stopping the desired camera. */
    private native void startCamera(int camera);
    private native void stopCamera();
    /** Native method for starting / stopping off target tracking */
    private native boolean startExtendedTracking();
    private native boolean stopExtendedTracking();

    /** Called when the activity will start interacting with the user. */
    public void resume()
    {
        // This is needed for some Droid devices to force portrait
        if (mIsDroidDevice)
        {
            activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
            activity.setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        }

        // Vuforia-specific resume operation
        Vuforia.onResume();

        // We may start the camera only if the Vuforia SDK has already been
        // initialized
        if (mAppStatus == APPSTATUS_CAMERA_STOPPED)
        {
            updateApplicationStatus(APPSTATUS_CAMERA_RUNNING);
        }

        // Resume the GL view:
        if (mGlView != null)
        {
            mGlView.setVisibility(View.VISIBLE);
            mGlView.onResume();
        }

    }


    /**
     * Updates projection matrix and viewport after a screen rotation change was
     * detected.
     */
    public void updateRenderView()
    {
        int currentScreenRotation = activity.getWindowManager().getDefaultDisplay()
            .getRotation();
        if (currentScreenRotation != mLastScreenRotation)
        {
            // Set projection matrix if there is already a valid one:
            if (Vuforia.isInitialized()
                && (mAppStatus == APPSTATUS_CAMERA_RUNNING))
            {
                DebugLog.LOGD("updateRenderView");

                // Query display dimensions:
                storeScreenDimensions();

                // Update viewport via renderer:
                mRenderer.updateRendering(mScreenWidth, mScreenHeight);

                // Cache last rotation used for setting projection matrix:
                mLastScreenRotation = currentScreenRotation;
            }
        }
    }


    /** Callback for configuration changes the activity handles itself */
    public void configurationchanged(Configuration config)
    {
        storeScreenDimensions();
        // Invalidate screen rotation to trigger query upon next render call:
        mLastScreenRotation = INVALID_SCREEN_ROTATION;
    }


    /** Called when the system is about to start resuming a previous activity. */
    public void pause()
    {
        if (mGlView != null)
        {
            mGlView.setVisibility(View.INVISIBLE);
            mGlView.onPause();
        }
        if (mAppStatus == APPSTATUS_CAMERA_RUNNING)
        {
            updateApplicationStatus(APPSTATUS_CAMERA_STOPPED);
        }

        // Vuforia-specific pause operation
        Vuforia.onPause();
    }


    /** Native function to deinitialize the application. */
    private native void deinitApplicationNative();

    private native void saveTrajectory();

    /** The final call you receive before your activity is destroyed. */
    public void destroy()
    {
        // Cancel potentially running tasks
        if (mInitVuforiaTask != null
            && mInitVuforiaTask.getStatus() != InitVuforiaTask.Status.FINISHED)
        {
            mInitVuforiaTask.cancel(true);
            mInitVuforiaTask = null;
        }
        if (mLoadTrackerTask != null
            && mLoadTrackerTask.getStatus() != LoadTrackerTask.Status.FINISHED)
        {
            mLoadTrackerTask.cancel(true);
            mLoadTrackerTask = null;
        }
        // Ensure that all asynchronous operations to initialize Vuforia
        // and loading the tracker datasets do not overlap:
        synchronized (mShutdownLock)
        {
            saveTrajectory();
            // Do application deinitialization in native code:
            deinitApplicationNative();
            // Destroy the tracking data set:
            destroyTrackerData();
            // Deinit the tracker:
            deinitTracker();
            // Deinitialize Vuforia SDK:
            Vuforia.deinit();
        }
        System.gc();
    }


    /**
     * NOTE: this method is synchronized because of a potential concurrent
     * access by onResume() and InitVuforiaTask.onPostExecute().
     */
    private synchronized void updateApplicationStatus(int appStatus)
    {
        // Exit if there is no change in status:
        if (mAppStatus == appStatus)
            return;
        // Store new status value:
        mAppStatus = appStatus;
        // Execute application state-specific actions:
        switch (mAppStatus)
        {
            case APPSTATUS_INIT_APP:
                // Initialize application elements that do not rely on Vuforia
                // initialization:
                initApplication();
                // Proceed to next application initialization status:
                updateApplicationStatus(APPSTATUS_INIT_VUFORIA);
                break;
            case APPSTATUS_INIT_VUFORIA:
                // Initialize Vuforia SDK asynchronously to avoid blocking the
                // main (UI) thread.
                //
                // NOTE: This task instance must be created and invoked on the
                // UI thread and it can be executed only once!
                try
                {
                    mInitVuforiaTask = new InitVuforiaTask();
                    mInitVuforiaTask.execute();
                } catch (Exception e)
                {
                    DebugLog.LOGE("Initializing Vuforia SDK failed");
                }
                break;

            case APPSTATUS_INIT_TRACKER:
                // Initialize the ObjectTracker:
                int markersNum= dataset_markers.size();
                String markerNames[]=new String[markersNum];
                float markerRot[]=new float[markersNum*4];
                float markerTra[]=new float[markersNum*3];
                float markerSca[]=new float[markersNum*3];

                int i=0;
                for(Marker m:dataset_markers){
                    markerNames[i]=m.getName();

                    markerRot[(i*4)]=m.getRotation().get(0);
                    markerRot[(i*4)+1]=m.getRotation().get(1);
                    markerRot[(i*4)+2]=m.getRotation().get(2);
                    markerRot[(i*4)+3]=m.getRotation().get(3);

                    markerTra[(i*3)]=m.getTranslation().get(0);
                    markerTra[(i*3)+1]=m.getTranslation().get(1);
                    markerTra[(i*3)+2]=m.getTranslation().get(2);

                    i++;
                }

                if (initTracker(activity.getAssets(),activity.getFilesDir().getAbsolutePath(),
                        dataset_obj,dataset_mtl,dataset_xml,dataset_folder,dataset_scale,markersNum,
                        markerNames,markerRot,markerTra,markerSca,mvo,dataset_mode) > 0)
                {
                    // Proceed to next application initialization status:
                    updateApplicationStatus(APPSTATUS_INIT_APP_AR);
                }


                break;

            case APPSTATUS_INIT_APP_AR:
                // Initialize Augmented Reality-specific application elements
                // that may rely on the fact that the Vuforia SDK has been
                // already initialized:
                initApplicationAR();

                // Proceed to next application initialization status:
                updateApplicationStatus(APPSTATUS_LOAD_TRACKER);
                break;

            case APPSTATUS_LOAD_TRACKER:
                // Load the tracking data set:
                //
                // NOTE: This task instance must be created and invoked on the
                // UI thread and it can be executed only once!
                try
                {
                    mLoadTrackerTask = new LoadTrackerTask();
                    mLoadTrackerTask.execute();
                } catch (Exception e)
                {
                    DebugLog.LOGE("Loading tracking data set failed");
                }



                break;

            case APPSTATUS_INITED:
                // Hint to the virtual machine that it would be a good time to
                // run the garbage collector:
                //
                // NOTE: This is only a hint. There is no guarantee that the
                // garbage collector will actually be run.
                System.gc();

                // Native post initialization:
                onVuforiaInitializedNative();

               // Activate the renderer:
                mRenderer.mIsActive = true;
                
                // Now add the GL surface view. It is important
                // that the OpenGL ES surface view gets added
                // BEFORE the camera is started and video
                // background is configured.
                activity.addContentView(mGlView, new LayoutParams(
                    LayoutParams.MATCH_PARENT, LayoutParams.MATCH_PARENT));

                // Sets the UILayout to be drawn in front of the camera
                mUILayout.bringToFront();

                // Start the camera:
                updateApplicationStatus(APPSTATUS_CAMERA_RUNNING);
                
                break;

            case APPSTATUS_CAMERA_STOPPED:
                // Call the native function to stop the camera:
                stopCamera();
                break;

            case APPSTATUS_CAMERA_RUNNING:
                // Call the native function to start the camera:
                startCamera(mCurrentCamera);

                // Hides the Loading Dialog
                loadingDialogHandler.sendEmptyMessage(HIDE_LOADING_DIALOG);

                // Sets the layout background to transparent
                mUILayout.setBackgroundColor(Color.TRANSPARENT);

                // Set continuous auto-focus if supported by the device,
                // otherwise default back to regular auto-focus mode.
                // This will be activated by a tap to the screen in this
                // application.
                boolean result = setFocusMode(FOCUS_MODE_CONTINUOUS_AUTO);

                //ViewGroup.LayoutParams layoutParams=mGlView.getLayoutParams();
                /*layoutParams.width=(int)(layoutParams.width*ratio);
                layoutParams.height=(int)(layoutParams.height*ratio);*/
                //mGlView.setLayoutParams(layoutParams);

                if(et){
                    boolean res =startExtendedTracking();
                    DebugLog.LOGE("extended tracking: "+res);
                }
                break;

            default:
                throw new RuntimeException("Invalid application state");
        }
    }


    /** Tells native code whether we are in portait or landscape mode */
    private native void setActivityPortraitMode(boolean isPortrait);


    /** Initialize application GUI elements that are not related to AR. */
    private void initApplication()
    {
        setActivityPortraitMode(true);

        // Query display dimensions:
        storeScreenDimensions();

        // As long as this window is visible to the user, keep the device's
        // screen turned on and bright:
        activity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }


    /** Native function to initialize the application. */
    private native void initApplicationNative(int width, int height);


    /** Initializes AR application components. */
    private void initApplicationAR()
    {
        // Do application initialization in native code (e.g. registering
        // callbacks, etc.):
        initApplicationNative(mScreenWidth, mScreenHeight);

        // Create OpenGL ES view:
        int depthSize = 16;
        int stencilSize = 0;
        boolean translucent = Vuforia.requiresAlpha();

        mGlView =  new VuforiaSampleGLView(activity);


        mGlView.init(translucent, depthSize, stencilSize);


        mRenderer = new GLRenderer();
        mRenderer.mActivity = this;

        mGlView.setRenderer(mRenderer);


        LayoutInflater inflater = LayoutInflater.from(activity);
        mUILayout = (RelativeLayout) inflater.inflate(R.layout.camera_overlay,
            null, false);

        mUILayout.setVisibility(View.VISIBLE);
        mUILayout.setBackgroundColor(Color.BLACK);

        // Gets a reference to the loading dialog
        mLoadingDialogContainer = mUILayout
            .findViewById(R.id.loading_indicator);
        //grayscale =(ImageView) mUILayout.findViewById(R.id.grayscale);

        // Shows the loading indicator at start
        loadingDialogHandler.sendEmptyMessage(SHOW_LOADING_DIALOG);

        // Adds the inflated layout to the view
        activity.addContentView(mUILayout, new LayoutParams(LayoutParams.MATCH_PARENT,
            LayoutParams.MATCH_PARENT));


    }


    public static void setGrayscale(Mat m) {
        // make a mat and draw something
        Imgproc.putText(m, "hi there ;)", new Point(30,80), Core.FONT_HERSHEY_SCRIPT_SIMPLEX, 2.2, new Scalar(200,200,0),2);

        // convert to bitmap:
        Bitmap bm = Bitmap.createBitmap(m.cols(), m.rows(),Bitmap.Config.ARGB_8888);
        Utils.matToBitmap(m, bm);

        // find the imageview and draw it!
        grayscale.setImageBitmap(bm);
    }

    /** Tells native code to switch dataset as soon as possible */
    private native void switchDatasetAsap(String dataset);
    private native boolean autofocus();
    private native boolean setFocusMode(int mode);
    /** Activates the Flash */
    private native boolean activateFlash(boolean flash);

}
