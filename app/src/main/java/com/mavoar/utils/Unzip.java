package com.mavoar.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class Unzip
{
    /**
     * Unzip it
     * @param zipFile input zip file
     * @param output zip file output folder
     */
    private static void delete(File f)  {
        try{
            if (f.isDirectory()) {
                for (File c : f.listFiles())
                delete(c);
            }
        f.delete();
        }
        catch(Exception e){}
    }
    public static void unzip(File zipFile, File targetDirectory) {
        try {

    delete(targetDirectory);
    ZipInputStream zis = new ZipInputStream(
            new BufferedInputStream(new FileInputStream(zipFile)));
        ZipEntry ze;
        int count;
        byte[] buffer = new byte[8192];
        while ((ze = zis.getNextEntry()) != null) {
            File file = new File(targetDirectory, ze.getName());
            File dir = ze.isDirectory() ? file : file.getParentFile();
            if (!dir.isDirectory() && !dir.mkdirs()){}
            if (ze.isDirectory())
                continue;
        
            FileOutputStream fout = new FileOutputStream(file);
            
                while ((count = zis.read(buffer)) != -1)
                    fout.write(buffer, 0, count);

                    fout.close();
           
            /* if time should be restored as well
            long time = ze.getTime();
            if (time > 0)
                file.setLastModified(time);
            */
    } 
        zis.close();
  
     } catch(Exception e){}
}
}