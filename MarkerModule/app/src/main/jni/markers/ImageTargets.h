/*===============================================================================
Copyright (c) 2016 PTC Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

#ifndef IMAGETARGETSNATIVE_IMAGETARGETS_H
#define IMAGETARGETSNATIVE_IMAGETARGETS_H

// info used to render a mesh
struct Marker {
    const char* name;

    float rotation[4];
    float translation[3];
    float scale[3];
};

#ifdef __cplusplus
extern "C"
{
#endif

// Expose this function to SampleAppRenderer class so we can call it for each view, mono or stereo
// This function is defined globally in ImageTargets.cpp. We need the state to get the
// trackables results and projection matrix to render the augmentation correctly depending on
// the current view
void renderFrameForView(const Vuforia::State *state, Vuforia::Matrix44F &projectionMatrix);

#ifdef __cplusplus
}
#endif

#endif //IMAGETARGETSNATIVE_IMAGETARGETS_H
