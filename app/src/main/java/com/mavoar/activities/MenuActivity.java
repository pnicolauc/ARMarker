package com.mavoar.activities;

import android.content.Intent;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.Spinner;
import java.io.BufferedReader;
import java.io.FileReader;
import com.mavoar.utils.Unzip;


import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;
import com.mavoar.R;
import com.mavoar.markers.dataset.Dataset;
import com.mavoar.markers.dataset.SimpleDatasetInfo;
import android.widget.EditText; 

import java.io.IOException;
import java.io.File;


import java.io.InputStream;
import java.util.ArrayList;

public class MenuActivity extends AppCompatActivity {
    // Name of the native dynamic libraries to load:
    private static final int REQUEST_PATH = 1;
	String curFileName;
    String path;
	EditText edittext;


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

        edittext = (EditText)findViewById(R.id.editText);

        Button start= (Button) findViewById(R.id.start);


        start.setOnClickListener(new View.OnClickListener(){
             Bundle setBundle(){
                Gson gson=new Gson();
                Bundle b= new Bundle();
                try{
                    BufferedReader br = new BufferedReader(new FileReader("/sdcard/Download/tmpardata/dataset.json"));
                
                Dataset dataset= gson.fromJson(br,Dataset.class);
                dataset.setFolder("/sdcard/Download/tmpardata");
                dataset.reverseMarkersTransforms();

                b.putString("name","NAME");
                b.putString("key",dataset.getKey());
                b.putString("obj",dataset.getVirtual());
                b.putString("mtl",dataset.getReal());
                b.putString("folder",dataset.getModelFolder());
                b.putString("xml",dataset.getXml());
                b.putFloat("scale",dataset.getScale());
                b.putSerializable("markers",dataset.getMarkers());

                }catch(Exception e){}
                return b;
            }

            @Override
            public void onClick(View view) {
                if(curFileName.length()>0){
                    Unzip.unzip(new File(path+"/"+curFileName),new File("/sdcard/Download/tmpardata"));
                    Bundle bund=setBundle();
                    Intent mavoar = new Intent(MenuActivity.this,MAVOAR.class);
                    mavoar.putExtras(bund);
                    startActivity(mavoar);
                    finish();
                }
            }
        });
    }

    public ArrayList<String> ExtractDatasetNames(){
        ArrayList<String> names= new ArrayList<>();
        for(SimpleDatasetInfo s: datasets){
            names.add(s.getName());
        }

        return names;
    }

    public void getfile(View view){ 
    	Intent intent1 = new Intent(this, com.mavoar.fileexplorer.FileChooser.class);
        startActivityForResult(intent1,REQUEST_PATH);
    }

     // Listen for results.
    protected void onActivityResult(int requestCode, int resultCode, Intent data){
        // See which child activity is calling us back.
    	if (requestCode == REQUEST_PATH){
    		if (resultCode == RESULT_OK) { 
    			curFileName = data.getStringExtra("GetFileName"); 
                path = data.getStringExtra("GetPath");
            	edittext.setText(curFileName);
    		}
    	 }
    }

}
