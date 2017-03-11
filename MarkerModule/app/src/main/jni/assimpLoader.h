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

#ifndef ASSIMPLOADER_H
#define ASSIMPLOADER_H

#include <map>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <opencv2/opencv.hpp>

#include "myGLM.h"
#include "myGLFunctions.h"

// info used to render a mesh
struct MeshInfo {
    GLuint  textureIndex;
    int     numberOfFaces;
    GLuint  faceBuffer;
    GLuint  vertexBuffer;
    GLuint  textureCoordBuffer;

    unsigned int mTextureID;

    float* vertices;
    float* texCoords;

    unsigned short* indices;
    int nVertices;
    int nIndices;
};

class AssimpLoader {

public:
    AssimpLoader();
    ~AssimpLoader();

    void Render3DModel(GLfloat* mvpMat);
    bool Load3DModel(std::string modelFilename,const char* folder);
    void Delete3DModel();
    std::vector<MeshInfo*> getMeshes();
    std::vector<cv::Mat> getTextures();

private:
    void GenerateGLBuffers();
    bool LoadTexturesToGL(std::string modelFilename,const char* folder);

    std::vector<MeshInfo*> modelMeshes;       // contains one struct for every mesh in model
    std::vector<cv::Mat> texturesCV;       // Textures



    Assimp::Importer *importerPtr;
    const aiScene* scene;                           // assimp's output data structure
    bool isObjectLoaded;

    std::map<std::string, GLuint> textureNameMap;   // (texture filename, texture name in GL)

    GLuint  vertexAttribute, vertexUVAttribute;     // attributes for shader variables
    GLuint  shaderProgramID;
    GLint   mvpLocation, textureSamplerLocation;    // location of MVP in the shader
};

#endif //ASSIMPLOADER_H
