package markermodule.mavoar.com.menu;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Spinner;

import markermodule.mavoar.com.R;
import markermodule.mavoar.com.markers.ImageTargets;

public class MenuActivity extends AppCompatActivity {
    // Name of the native dynamic libraries to load:
    private static final String NATIVE_LIB_SAMPLE = "ImageTargetsNative";
    private static final String NATIVE_LIB_VUFORIA = "Vuforia";

    static {
        System.loadLibrary(NATIVE_LIB_SAMPLE);
        System.loadLibrary(NATIVE_LIB_VUFORIA);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button start= (Button) findViewById(R.id.start);

        final Spinner spin= (Spinner) findViewById(R.id.spin);

        start.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                Intent i = new Intent(MenuActivity.this,
                        ImageTargets.class);
                String key= getResources().getStringArray(R.array.dataset_keys)[spin.getSelectedItemPosition()];
                String name = (String) spin.getSelectedItem();

                i.putExtra("name",name);
                i.putExtra("key",key);
                startActivity(i);
                finish();
            }
        });
    }

}
