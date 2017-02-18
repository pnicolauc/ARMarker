/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Copyright (c) 2012-2014 Qualcomm Connected Experiences, Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other 
countries.
===============================================================================*/

#ifndef _VUFORIA_CUBE_SHADERS_H_
#define _VUFORIA_CUBE_SHADERS_H_


static const char* cubeMeshVertexShader = " \
  \
attribute vec4 vertexPosition; \
attribute vec2 vertexTexCoord; \
 \
varying vec2 texCoord; \
 \
uniform mat4 modelViewProjectionMatrix; \
 \
void main() \
{ \
   gl_Position = modelViewProjectionMatrix * vertexPosition; \
   texCoord = vertexTexCoord; \
} \
";


static const char* cubeFragmentShader = " \
 \
precision mediump float; \
 \
varying vec2 texCoord; \
 \
uniform sampler2D texSampler2D; \
 \
void main() \
{ \
   gl_FragColor = texture2D(texSampler2D, texCoord); \
} \
";

#endif // _VUFORIA_CUBE_SHADERS_H_
