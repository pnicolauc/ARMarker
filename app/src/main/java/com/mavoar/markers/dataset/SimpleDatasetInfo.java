package com.mavoar.markers.dataset;

/**
 * Created by al on 05-03-2017.
 */

public class SimpleDatasetInfo {
    private String name;
    private String folder;

    public SimpleDatasetInfo(String name, String folder) {
        this.name = name;
        this.folder = folder;
    }

    public String getName() {
        return name;
    }

    public String getFolder() {
        return folder;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setFolder(String folder){this.folder=folder;}
}

