package markermodule.mavoar.com.menu;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;

import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;

import markermodule.mavoar.com.R;
import markermodule.mavoar.com.markers.ImageTargets;
import markermodule.mavoar.com.markers.dataset.Dataset;
import markermodule.mavoar.com.markers.dataset.SimpleDatasetInfo;

public class MenuActivity extends AppCompatActivity {
    // Name of the native dynamic libraries to load:
    private static final String NATIVE_LIB_SAMPLE = "ImageTargetsNative";
    private static final String NATIVE_LIB_VUFORIA = "Vuforia";

    ArrayList<SimpleDatasetInfo> datasets;

    static {
        System.loadLibrary(NATIVE_LIB_SAMPLE);
        System.loadLibrary(NATIVE_LIB_VUFORIA);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        Button start= (Button) findViewById(R.id.start);
        Gson gson=new Gson();
        String json= loadJSONFromAsset("datasets.json");
        datasets = gson.fromJson(json, new TypeToken<ArrayList<SimpleDatasetInfo>>(){}.getType());

        ArrayList<String> dataset_names= ExtractDatasetNames();

        final Spinner spin= (Spinner) findViewById(R.id.spin);
        ArrayAdapter<String> spinnerArrayAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, dataset_names); //selected item will look like a spinner set from XML
        spinnerArrayAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        spin.setAdapter(spinnerArrayAdapter);

        final Spinner mod_spin= (Spinner) findViewById(R.id.modules);

        start.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                switch (mod_spin.getSelectedItemPosition()){
                    case 0:
                        break;
                    case 1:
                        
                }
                int pos= spin.getSelectedItemPosition();

                SimpleDatasetInfo chosenDataset=datasets.get(pos);
                Gson gson=new Gson();
                Dataset dataset= gson.fromJson(loadJSONFromAsset(chosenDataset.getFolder()+"/dataset.json"),Dataset.class);
                dataset.setFolder(chosenDataset.getFolder());
                Intent i = new Intent(MenuActivity.this,
                        ImageTargets.class);

                i.putExtra("name",chosenDataset.getName());
                i.putExtra("key",dataset.getKey());
                i.putExtra("obj",dataset.getObj());
                i.putExtra("mtl",dataset.getMtl());
                i.putExtra("folder",dataset.getModelFolder());
                i.putExtra("xml",dataset.getXml());
                i.putExtra("scale",dataset.getScale());
                i.putExtra("markers",dataset.getMarkers());

                startActivity(i);
                finish();
            }
        });
    }

    public String loadJSONFromAsset(String file) {
        String json = null;
        try {
            InputStream is = getAssets().open(file);
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            json = new String(buffer, "UTF-8");
        } catch (IOException ex) {
            ex.printStackTrace();
            return null;
        }
        return json;
    }

    public ArrayList<String> ExtractDatasetNames(){
        ArrayList<String> names= new ArrayList<>();
        for(SimpleDatasetInfo s: datasets){
            names.add(s.getName());
        }

        return names;
    }

}
