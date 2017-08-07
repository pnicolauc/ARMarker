package com.mavoar.markers.dataset;

import java.util.ArrayList;

/**
 * Created by al on 05-03-2017.
 */

public class Dataset {
    private String key;
    private String real;
    private String modelFolder;
    private float scale;
    private String virtual;
    private String xml;

    private ArrayList<Marker> markers;

    public Dataset(){
    }

    public Dataset(String key, String real, String modelFolder, float scale, String virtual, String xml, ArrayList<Marker> markers) {
        this.key = key;
        this.real = real;
        this.modelFolder = modelFolder;
        this.scale = scale;
        this.virtual = virtual;
        this.xml = xml;
        this.markers = markers;
    }

    public void setFolder(String folder){
        this.real=folder+"/"+modelFolder + "/" +real;
        this.virtual=folder+"/"+modelFolder + "/" +virtual;
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

    public String getReal() {
        return real;
    }

    public void setReal(String real) {
        this.real = real;
    }

    public String getVirtual() {
        return virtual;
    }

    public void setVirtual(String virtual) {
        this.virtual = virtual;
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

    public void reverseMarkersTransforms(){
        ArrayList<Marker> newMarkerList= new ArrayList<>();
        for(Marker m: this.markers){
            m.reverseTransformations();
            newMarkerList.add(m);
        }

        this.markers=newMarkerList;
    }
}
