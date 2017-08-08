/*
 *    Copyright 2016 Anand Muralidhar
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <renderer/myShader.h>
#include <renderer/modelAssimp.h>


#include <assimp/Importer.hpp>
#include <renderer/myJNIHelper.h>

/**
 * Class constructor
 */
ModelAssimp::ModelAssimp() {

    MyLOGD("ModelAssimp::ModelAssimp");
    initsDone = false;

    modelObject = NULL;
}

ModelAssimp::~ModelAssimp() {

    MyLOGD("ModelAssimp::ModelAssimpssimp");

    if (modelObject) {
        delete modelObject;
    }
}

/**
 * Perform inits and load the triangle's vertices/colors to GLES
 */
void ModelAssimp::PerformGLInits(const char* obj,const char* mtl,const char* folder) {

    MyLOGD("ModelAssimp::PerformGLInits");

    MyLOGD("FOLDER: %s",folder);
    MyLOGD("OBJ FILE: %s",obj);
    MyLOGD("MTL FILE: %s",mtl);

    MyGLInits();

    modelObject = new AssimpLoader();

    modelObject->Load3DModel(obj,folder);

    CheckGLError("ModelAssimp::PerformGLInits");
    initsDone = true;
}

ThreeDModel* ModelAssimp::getThreeDModel(){
    return modelObject->getThreeDModel();
}


std::vector<MeshInfo*> ModelAssimp::getMeshes(){
    return modelObject->getMeshes();
}

std::vector<cv::Mat> ModelAssimp::getTextures(){
    return modelObject->getTextures();
}


