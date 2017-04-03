package com.mavoar.activities;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

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

import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
/**
 * Created by al on 22-03-2017.
 */

public class VOModule extends Activity implements CameraBridgeViewBase.CvCameraViewListener2 {

    private float fl;
    private CameraBridgeViewBase mOpenCvCameraView;
    private String TAG= "COISO";

    private int ratio= 8;

    private Size downscaled;
    private Size upscaled;
    private SensorListener msensorListener;
    double scale=0.0;


    ArrayList<Point> trajectory;


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
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        fl= getIntent().getFloatExtra("fl",0.0f);
        Log.i(TAG, "focal length: "+ fl);


        trajectory = new ArrayList<>();
        msensorListener = new SensorListener(this);

        mOpenCvCameraView = new JavaCameraView(this, -1);
        setContentView(mOpenCvCameraView);
        mOpenCvCameraView.setCvCameraViewListener(this);
    }
    @Override
    public void onCameraViewStarted(int width, int height) {
        downscaled = new Size(width/ratio,height/ratio);
        upscaled = new Size(width,height);

        float ppx = ((float)width)/2.0f;
        float ppy = ((float)height)/2.0f;

        init(fl,ppx,ppy);
    }

    @Override
    public Mat onCameraFrame(CameraBridgeViewBase.CvCameraViewFrame inputFrame) {
        Mat matGray = inputFrame.gray();
        double scale=getScalefromSensorListener();
        float[] rot=msensorListener.getRot();
        float[] rotmat=msensorListener.getRotMat();

        Imgproc.resize( matGray, matGray, downscaled );
        String message=processFrame(matGray.getNativeObjAddr(),scale,rotmat);
        Imgproc.resize( matGray, matGray, upscaled );


        Mat rgb= inputFrame.rgba();

        Matcher m = Pattern.compile("-?\\d+\\.\\d+").matcher(message);
        ArrayList<Float> translations= new ArrayList<Float>(); 
        while (m.find()) 
        {
            translations.add(Float.parseFloat(m.group(0)));
        }

        translations.add(0.0f);
        translations.add(0.0f);

        float x= translations.get(0);
        float y= translations.get(1);

        String message2= x+" "+y;

        int xx=(int)(200+(int)(x*20.0f));
        int yy=((int)(200+y*20.0));

        Point p= new Point(xx,yy);

        trajectory.add(p);
        for(Point point: trajectory){
            Imgproc.circle(rgb,point,5,new Scalar(255,0,0),-1);
        }
        



        Imgproc.putText(rgb,message,new Point(10, 50),               // point
                Core.FONT_HERSHEY_SIMPLEX ,      // front face
                1,                               // front scale
                new Scalar(255, 0, 0),             // Scalar object for color
                4 );
        
         Imgproc.putText(rgb,message2+"",new Point(10, 250),               // point
                Core.FONT_HERSHEY_SIMPLEX ,      // front face
                1,                               // front scale
                new Scalar(255, 0, 0),             // Scalar object for color
                4 );

        Imgproc.putText(rgb,scale+"",new Point(10, 200),               // point
                Core.FONT_HERSHEY_SIMPLEX ,      // front face
                1,                               // front scale
                new Scalar(255, 0, 0),             // Scalar object for color
                4 );

        Imgproc.putText(rgb,rot[0]+" " +rot[1]+" "+rot[2],new Point(10, 300),               // point
                Core.FONT_HERSHEY_SIMPLEX ,      // front face
                1,                               // front scale
                new Scalar(255, 0, 0),             // Scalar object for color
                4 );
        

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
    public native void init(float focalLength,float ppx,float ppy);
    public native String processFrame(long matPointer,double scale,float[] rot);
}