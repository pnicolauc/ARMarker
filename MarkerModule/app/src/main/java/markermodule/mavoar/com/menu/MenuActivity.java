package markermodule.mavoar.com.menu;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

import markermodule.mavoar.com.R;

public class MenuActivity extends AppCompatActivity {
    // Name of the native dynamic libraries to load:
    private static final String NATIVE_LIB_SAMPLE = "ImageTargetsNative";
    private static final String NATIVE_LIB_VUFORIA = "Vuforia";

    // Used to load the 'native-lib' library on application startup.
    static {

        System.loadLibrary("Vuforia");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


    }

}
