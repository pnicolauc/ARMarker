/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2014 Qualcomm Connected E xperiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/

#ifndef _VUFORIA_SAMPLEUTILS_H_
#define _VUFORIA_SAMPLEUTILS_H_

// Includes:
#include <stdio.h>
#include <android/log.h>

// Utility for logging:
#define LOG_TAG    "Vuforia"
#define LOG(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

/// A utility class used by the Vuforia SDK samples.
class SampleUtils
{
public:

    static void printVector(const float* mat);

    /// Prints a 4x4 matrix.
    static void printMatrix(const float* matrix);
    
    /// Prints a 3x3 matrix.
    static void printMatrix33(const float* mat);

    static void setIDMatrix(float x,float y,float z,float* mat);


    /// Prints GL error information.
    static void checkGlError(const char* operation);
    
    /// Set the rotation components of this 4x4 matrix.
    static void setRotationMatrix(float angle, float x, float y, float z, 
        float *nMatrix);
    
    /// Set the translation components of this 4x4 matrix.
    static void translatePoseMatrix(float x, float y, float z,
        float* nMatrix = NULL);

    static void multiplyMatrixForVector(float* mat1,float* mat2,float* out);
    
    static void
    setRotation33to44(float x1,float x2,float x3,
                                float y1,float y2,float y3,
                                float z1,float z2,float z3,
                                float* mat);

    static void setIdentity(float* mat);
    /// Applies a rotation.
    static void rotatePoseMatrix(float angle, float x, float y, float z, 
        float* nMatrix = NULL);

    static float* invertMatrix(float* m);

    static void printMatrix44(float* matrix);
    
    /// Applies a scaling transformation.
    static void scalePoseMatrix(float x, float y, float z, 
        float* nMatrix = NULL);

    /// transposes matrix
    static float* transposeMatrix(float* matrix);


    /// Multiplies the two matrices A and B and writes the result to C.
    static void multiplyMatrix(float *matrixA, float *matrixB, 
        float *matrixC);
    
    /// Initialize a shader.
    static unsigned int initShader(unsigned int shaderType, 
        const char* source);
    
    /// Create a shader program.
    static unsigned int createProgramFromBuffer(const char* vertexShaderBuffer,
        const char* fragmentShaderBuffer);
};

#endif // _VUFORIA_SAMPLEUTILS_H_
