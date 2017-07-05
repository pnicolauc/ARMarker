package com.mavoar.activities;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;


import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import com.mavoar.R;
import com.mavoar.markers.dataset.Dataset;
import com.mavoar.markers.dataset.SimpleDatasetInfo;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

public class MenuActivity extends AppCompatActivity {
    // Name of the native dynamic libraries to load:
    private static final String NATIVE_LIB_SAMPLE = "MAVOAR";
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

        start.setOnClickListener(new View.OnClickListener() {
            Bundle setBundle(int pos){
                SimpleDatasetInfo chosenDataset=datasets.get(pos);
                Gson gson=new Gson();
                Dataset dataset= gson.fromJson(loadJSONFromAsset(chosenDataset.getFolder()+"/dataset.json"),Dataset.class);
                dataset.setFolder(chosenDataset.getFolder());
                dataset.reverseMarkersTransforms();
                Bundle b= new Bundle();


                b.putString("name",chosenDataset.getName());
                b.putString("key",dataset.getKey());
                b.putString("obj",dataset.getObj());
                b.putString("mtl",dataset.getMtl());
                b.putString("folder",dataset.getModelFolder());
                b.putString("xml",dataset.getXml());
                b.putFloat("scale",dataset.getScale());
                b.putSerializable("markers",dataset.getMarkers());

                return b;
            }
            @Override
            public void onClick(View view) {
                int pos= spin.getSelectedItemPosition();

                Bundle bund=setBundle(pos);
                Intent mavoar = new Intent(MenuActivity.this,
                        MAVOAR.class);
                mavoar.putExtras(bund);
                startActivity(mavoar);
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
