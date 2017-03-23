package com.mavoar.markers.dataset;

import java.util.ArrayList;

/**
 * Created by al on 05-03-2017.
 */

public class Dataset {
    private String key;
    private String obj;
    private String modelFolder;
    private float scale;
    private String mtl;
    private String xml;

    private ArrayList<Marker> markers;

    public Dataset(){
    }

    public Dataset(String key, String obj, String modelFolder, float scale, String mtl, String xml, ArrayList<Marker> markers) {
        this.key = key;
        this.obj = obj;
        this.modelFolder = modelFolder;
        this.scale = scale;
        this.mtl = mtl;
        this.xml = xml;
        this.markers = markers;
    }

    public void setFolder(String folder){
        this.obj=folder+"/"+modelFolder + "/" +obj;
        this.mtl=folder+"/"+modelFolder + "/" +mtl;
        this.xml=folder + "/" +xml;
        this.modelFolder=folder+"/"+modelFolder;
    }



    public float getScale() {
        return scale;
    }

    public void setScale(float scale) {
        this.scale = scale;
    }
    public String getKey() {
        return key;
    }

    public void setKey(String key) {
        this.key = key;
    }

    public String getObj() {
        return obj;
    }

    public void setObj(String obj) {
        this.obj = obj;
    }

    public String getMtl() {
        return mtl;
    }

    public void setMtl(String mtl) {
        this.mtl = mtl;
    }

    public String getXml() {
        return xml;
    }

    public void setXml(String xml) {
        this.xml = xml;
    }

    public ArrayList<Marker> getMarkers() {
        return markers;
    }

    public void setMarkers(ArrayList<Marker> markers) {
        this.markers = markers;
    }


    public String getModelFolder() {
        return modelFolder;
    }

    public void setModelFolder(String modelFolder) {
        this.modelFolder = modelFolder;
    }
}
