package com.mavoar.activities;

import android.content.Intent;
import android.hardware.Camera;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import com.mavoar.R;

/**
 * Created by al on 18-02-2017.
 */

public class LoadingActivity extends AppCompatActivity {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_loading);


        Thread welcomeThread = new Thread() {
            float fl;
            @Override
            public void run() {
                try {
                    super.run();
                    Camera camera= Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK);
                    final Camera.Parameters params = camera.getParameters();
                    fl = params.getFocalLength();

                    camera.release();
                } catch (Exception e) {

                } finally {

                    Intent i = new Intent(LoadingActivity.this,
                            MenuActivity.class);
                    i.putExtra("fl",fl);
                    startActivity(i);
                    finish();
                }
            }
        };
        welcomeThread.start();

    }
}
