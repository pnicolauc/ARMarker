package com.mavoar.activities;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.WindowManager;

import com.mavoar.R;
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

public class MAVOAR extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {
    private float fl;
    private CameraBridgeViewBase mOpenCvCameraView;
    private String TAG= "COISO";

    private int ratio= 8;

    private Size downscaled;
    private Size upscaled;
    private SensorListener msensorListener;
    double scale=0.0;


    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                    Log.i(TAG, "OpenCV loaded successfully");
                    mOpenCvCameraView.enableView();
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };

    @Override
    public void onResume()
    {
        super.onResume();
        if (!OpenCVLoader.initDebug()) {
            Log.d("OpenCV", "Internal OpenCV library not found. Using OpenCV Manager for initialization");
            OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_3_0_0, this, mLoaderCallback);
        } else {
            Log.d("OpenCV", "OpenCV library found inside package. Using it!");
            mLoaderCallback.onManagerConnected(LoaderCallbackInterface.SUCCESS);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.split_screen);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        fl= getIntent().getFloatExtra("fl",0.0f);
        Log.i(TAG, "focal length: "+ fl);


        msensorListener = new SensorListener(this);

        mOpenCvCameraView = (CameraBridgeViewBase)findViewById(R.id.java_surface_view);
        mOpenCvCameraView.setVisibility(SurfaceView.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);

    }
    @Override
    public void onCameraViewStarted(int width, int height) {
        downscaled = new Size(width/ratio,height/ratio);
        upscaled = new Size(width,height);

        float ppx = ((float)width)/2.0f;
        float ppy = ((float)height)/2.0f;
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        Mat rgb = inputFrame.rgba();

        return rgb;
    }

    @Override
    public void onCameraViewStopped() {
        //mRgba.release();
    }

    private double getScalefromSensorListener(){
        scale = msensorListener.getScale();

        return scale;
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
}
