package com.mavoar.vomodule.vomodule;

import android.app.Activity;
import android.content.Intent;
import android.hardware.Camera;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import org.opencv.android.JavaCameraView;

/**
 * Created by al on 16-03-2017.
 */

public class MainActivity  extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        Camera camera= Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK);
        final Camera.Parameters params = camera.getParameters();
        float fl = params.getFocalLength();
        camera.release();

        Intent vo =  new Intent(MainActivity.this, VisualOdometry.class);
        vo.putExtra("fl",fl);


        MainActivity.this.startActivity(vo);

    }
}
