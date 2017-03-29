package com.mavoar.vo;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.util.Log;

public class SensorListener implements SensorEventListener {
	//Most recent sensor data and its timestamp
	
	static final float ALPHA = 0.25f; // if ALPHA = 1 OR 0, no filter applies.

	String Tag= "SensorMAVOAR";	
	float[] dAcc=new float[3];	
	float[] finalAcc=new float[3];
	float[] samplesAcc={0,0,0};	
	float[] prevAcc=new float[3];
	
	float deltaTime=0;
	float previoustime=0;

	private final float N2S=1000000000f;

	//Camera Pos/Att
	private static float[] Pos={0,0,0};
	private float[] Vel={0,0,0};
	
	private float[] Cal={0,0,0};

	//Rotation
	private static float[] Rot={0,0,0};
	private static float[] rotationMatrix = new float[9];

	int calibration = 1;
	
	int samples=1;
	
	private SensorManager mSMan;
		
	public SensorListener(Context mContext) {
		//Get a reference to sensor manager
        mSMan = (SensorManager)mContext.getSystemService(Context.SENSOR_SERVICE);
        
      //Sensors
        Sensor sAcc = mSMan.getDefaultSensor(Sensor.TYPE_LINEAR_ACCELERATION);
		Sensor sRot = mSMan.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);

		int rate=SensorManager.SENSOR_DELAY_GAME;
        
        mSMan.registerListener(this, sAcc, rate);
		mSMan.registerListener(this, sRot, rate);

	}
	
    protected void pause() {
        mSMan.unregisterListener(this);
    }
        
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    	//Do nothing
    }
    
    public static synchronized double getScale(){
    	double scale= /*Math.abs(*/Math.sqrt( Math.pow(Pos[0],2) + Math.pow(Pos[1],2) + Math.pow(Pos[2],2) )/*)*/;

		Pos[0] =0;
		Pos[1] =0;
		Pos[2] =0;

    	return scale;
    }

	public static synchronized float[] getRot(){
		return Rot;
	}

	public static synchronized float[] getRotMat(){
		return rotationMatrix;
	}
    
    public double getScaleUnfilteredAcc(){
    	double scale= Math.sqrt( Math.pow(dAcc[0],2) + Math.pow(dAcc[1],2) + Math.pow(dAcc[2],2) );
    	
    	return scale;
    }
    
    public String getAccelerationString(){
    	String acc= finalAcc[0] + "\t" +  finalAcc[1]  + "\t"+ finalAcc[2];
    	
    	return acc;
    }
    
    public String getPositionString(){
    	String pos= Pos[0] + "\t" +  Pos[1]  + "\t"+ Pos[2];
    	
    	return pos;
    }
    
    public String getDeltaT(){   	   	
    	return deltaTime + "";
    }

    public void onSensorChanged(SensorEvent event) {
    	int etype = event.sensor.getType();
    	long etime = event.timestamp;
    	float dt=0;
    	
    	
    	//Recod the value and time
    	if (etype ==Sensor.TYPE_LINEAR_ACCELERATION ) {
    		dAcc[0]=event.values[0];
    		dAcc[1]=event.values[1];
    		dAcc[2]=event.values[2];
    		
    		/* if(calibration <=10 && (dAcc[0]!= 0.0f | dAcc[1]!= 0.0f | dAcc[2]!= 0.0f)){
    			Cal[0] += dAcc[0];
    			Cal[1] += dAcc[1];
    			Cal[2] += dAcc[2];
    			calibration++;
    		}
    		else if(calibration ==11){
    			Cal[0] /= 10.0;
    			Cal[1] /= 10.0;
    			Cal[2] /= 10.0;
    			
    			calibration++;
    		}*/
    		        //else{
    			if(samples <= 3){
    				samplesAcc[0] += dAcc[0] - Cal[0];
    				samplesAcc[1] += dAcc[1] - Cal[1];
    				samplesAcc[2] += dAcc[2] - Cal[2];
    				
    				deltaTime+= (etime - previoustime) /N2S;
    				
    				previoustime = etime;
    				
    				samples++;
    			}
    			else{
    				
    				/*samplesAcc[0] /= 3.0;
    				samplesAcc[1] /= 3.0;
    				samplesAcc[2] /= 3.0;*/
    				
    				
    				finalAcc[0] = samplesAcc[0] - Cal[0];
    				finalAcc[1] = samplesAcc[1] - Cal[1];
    				finalAcc[2] = samplesAcc[2] - Cal[2];
    				
                    if((finalAcc[0]>=-0.2 && finalAcc[0]<=0.2) && (finalAcc[1]>=-0.2 && finalAcc[1]<=0.2) && (finalAcc[2]>=-0.2 && finalAcc[2]<=0.2)){
                    	finalAcc[0]=0;
                    	finalAcc[1]=0;
                    	finalAcc[2]=0;                      
                    }
                    
    				Vel[0]= (finalAcc[0] + prevAcc[0])/2.0f + deltaTime ;
    				Vel[1]= (finalAcc[1] + prevAcc[1])/2.0f + deltaTime ;
    				Vel[2]= (finalAcc[2] + prevAcc[2])/2.0f + deltaTime ;

    				Pos[0] += Vel[0] * deltaTime;
    				Pos[1] += Vel[1] * deltaTime;
    				Pos[2] += Vel[2] * deltaTime;
    				   				
    				     				
    				samplesAcc[0]=0;
    				samplesAcc[1]=0;
    				samplesAcc[2]=0;
    				samples=1;
    				deltaTime=0;
				}
    		//}    		   		
    	}
    	else{
			Rot[0]=event.values[0];
			Rot[1]=event.values[1];
			Rot[2]=event.values[2];

			mSMan.getRotationMatrixFromVector(rotationMatrix,Rot);
		}

    }
    
    protected float[] lowPass( float[] input, float[] output ) { 
    	
    	if ( output == null ) 
    		return input; 
    	for ( int i=0; i<input.length; i++ ) 
    	{ 
    		output[i] = output[i] + ALPHA * (input[i] - output[i]); 
		} 
    	return output; 
	}

	private native void scale(double scale);
	private native void rotationVector(float[] rot);



}
    

