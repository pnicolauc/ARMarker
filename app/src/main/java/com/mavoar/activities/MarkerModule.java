package com.mavoar.activities;

import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;

import com.mavoar.R;
import com.mavoar.markers.ImageTargets;
import com.mavoar.renderer.GLRenderer;
import com.mavoar.renderer.VuforiaSampleGLView;
import com.mavoar.utils.DebugLog;
import com.vuforia.Vuforia;

/**
 * Created by al on 22-03-2017.
 */

public class MarkerModule extends Activity {

    ImageTargets imageTargets;


    protected void onCreate(Bundle savedInstanceState)
    {
        DebugLog.LOGD("onCreate");
        super.onCreate(savedInstanceState);

        imageTargets = new ImageTargets(getIntent().getExtras(),MarkerModule.this,0.5,false);
    }

    protected void onResume()
    {
        DebugLog.LOGD("onResume");
        super.onResume();
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
}
