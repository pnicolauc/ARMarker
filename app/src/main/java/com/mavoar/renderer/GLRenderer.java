/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/

package com.mavoar.renderer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.view.animation.AccelerateInterpolator;

import com.mavoar.markers.ImageTargets;
import com.mavoar.utils.DebugLog;
import com.mavoar.vo.SensorListener;
import com.vuforia.Vuforia;


/** The renderer class for the ImageTargets sample. */
public class GLRenderer implements GLSurfaceView.Renderer
{
    public boolean mIsActive = false;
    
    /** Reference to main activity **/
    public ImageTargets mActivity;
    
    
    /** Native function for initializing the renderer. */
    public native void initRendering();
    
    
    /** Native function to update the renderer. */
    public native void updateRendering(int width, int height);


    /** Native method for updating rendering primitive */
    public native void updateRenderingPrimitives();


    /** Called when the surface is created or recreated. */
    public void onSurfaceCreated(GL10 gl, EGLConfig config)
    {
        DebugLog.LOGD("GLRenderer::onSurfaceCreated");
        
        // Call native function to initialize rendering:
        initRendering();
        
        // Call Vuforia function to (re)initialize rendering after first use
        // or after OpenGL ES context was lost (e.g. after onPause/onResume):
        Vuforia.onSurfaceCreated();
    }
    
    
    /** Called when the surface changed size. */
    public void onSurfaceChanged(GL10 gl, int width, int height)
    {
        DebugLog.LOGD("GLRenderer::onSurfaceChanged");
        
        // Call native function to update rendering when render surface
        // parameters have changed:
        updateRendering(width, height);
        
        // Call Vuforia function to handle render surface size changes:
        Vuforia.onSurfaceChanged(width, height);

        updateRenderingPrimitives();
    }
    

    /** The native render function. */
    public native void renderFrame(double scale,float x,float y,float z,
    float x1,float x2,float x3,
float y1,float y2,float y3,
float z1,float z2,float z3);
    
    
    /** Called to draw the current frame. */
    public void onDrawFrame(GL10 gl)
    {
        if (!mIsActive)
            return;
        
        // Update render view (projection matrix and viewport) if needed:
        mActivity.updateRenderView();
        
        // Call our native function to render content
        double scale= SensorListener.getScale();
        float[] rot= SensorListener.getRot();
        float[] rotmat = SensorListener.getRotMat();

        renderFrame(scale,rot[0],rot[1],rot[2],rotmat[0],rotmat[1],rotmat[2],
        rotmat[3],rotmat[4],rotmat[5],
        rotmat[6],rotmat[7],rotmat[8]);
    }
}
