package com.mavoar.activities;

import android.app.Activity;
import android.content.res.Configuration;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.WindowManager;

import com.mavoar.R;
import com.mavoar.markers.ImageTargets;
import com.mavoar.renderer.GLRenderer;
import com.mavoar.renderer.VuforiaSampleGLView;
import com.mavoar.utils.DebugLog;
import com.mavoar.vo.SensorListener;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.CameraBridgeViewBase;
import org.opencv.android.JavaCameraView;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;
import org.opencv.core.Mat;
import org.opencv.core.Point;
import org.opencv.core.Scalar;
import org.opencv.core.Size;
import org.opencv.imgproc.Imgproc;

/**
 * Created by al on 22-03-2017.
 */

public class MAVOAR extends Activity{
    private float fl;
    private CameraBridgeViewBase mOpenCvCameraView;
    private String TAG= "COISO";

    private double ratio= 0.5;

    private Size downscaled;
    private Size upscaled;
    private SensorListener msensorListener;
    double scale=0.0;

    ImageTargets imageTargets;
    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i("OpenCV", "OpenCV loaded successfully");
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        fl= getIntent().getFloatExtra("fl",0.0f);
        Log.i(TAG, "focal length: "+ fl);

        msensorListener = new SensorListener(this);

        imageTargets = new ImageTargets(getIntent().getExtras(),MAVOAR.this,ratio,true);
    }


    protected void onResume()
    {
        DebugLog.LOGD("onResume");
        super.onResume();
        if (!OpenCVLoader.initDebug()) {
            Log.d("OpenCV", "Internal OpenCV library not found. Using OpenCV Manager for initialization");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_3_0_0, this, mLoaderCallback);
        } else {
            Log.d("OpenCV", "OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }
        imageTargets.resume();
    }
    protected void onPause()
    {
        DebugLog.LOGD("onPause");
        super.onPause();
        imageTargets.pause();
    }
    /** The final call you receive before your activity is destroyed. */
    protected void onDestroy() {
        DebugLog.LOGD("onDestroy");
        super.onDestroy();

        imageTargets.destroy();
    }

    /** Callback for configuration changes the activity handles itself */
    public void onConfigurationChanged(Configuration config)
    {
        DebugLog.LOGD("onConfigurationChanged");
        super.onConfigurationChanged(config);
        imageTargets.configurationchanged(config);
    }


    private double getScalefromSensorListener(){
        scale = msensorListener.getScale();

        return scale;
    }
}
