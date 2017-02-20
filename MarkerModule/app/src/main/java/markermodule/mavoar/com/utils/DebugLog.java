/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/

package markermodule.mavoar.com.utils;

import android.util.Log;


/**
 * DebugLog is a support class for the Vuforia samples applications.
 * 
 * Exposes functionality for logging.
 * 
 * */

public class DebugLog
{
    private static final String LOGTAG = "Vuforia";
    
    
    /** Logging functions to generate ADB logcat messages. */
    
    public static final void LOGE(String nMessage)
    {
        Log.e(LOGTAG, nMessage);
    }
    
    
    public static final void LOGW(String nMessage)
    {
        Log.w(LOGTAG, nMessage);
    }
    
    
    public static final void LOGD(String nMessage)
    {
        Log.d(LOGTAG, nMessage);
    }
    
    
    public static final void LOGI(String nMessage)
    {
        Log.i(LOGTAG, nMessage);
    }
}
