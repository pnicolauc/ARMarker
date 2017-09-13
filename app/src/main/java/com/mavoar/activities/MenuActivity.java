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
import android.widget.ProgressBar;

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

    Spinner modelSpinner;


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


        modelSpinner= (Spinner) findViewById(R.id.modeltype);
        ArrayList<String> list2 = new ArrayList<String>();
        list2.add("Virtual Model");
        list2.add("Real Model");
        ArrayAdapter<String> dataAdapter2 = new ArrayAdapter<String>(this,
            android.R.layout.simple_spinner_item, list2);
        dataAdapter2.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        modelSpinner.setAdapter(dataAdapter2);

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
                if( modelSpinner.getSelectedItemPosition()==0){
                    b.putString("obj",dataset.getVirtual());
                }
                else b.putString("obj",dataset.getReal());

                b.putInt("mode",0);
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
                ProgressBar progressBar= (ProgressBar) findViewById(R.id.progbar);;

                if(curFileName.length()>0){
                    progressBar.setVisibility(View.VISIBLE);
                    Thread welcomeThread = new Thread() {
                                float fl;
                                @Override
                                public void run() {
                                Unzip.unzip(new File(path+"/"+curFileName),new File("/sdcard/Download/tmpardata"));
                                
                                Bundle bund=setBundle();
                                Intent mavoar = new Intent(MenuActivity.this,MAVOAR.class);
                                mavoar.putExtras(bund);
                                startActivity(mavoar);
                                finish();    
                        }
                            };
                    welcomeThread.start();
                    
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
