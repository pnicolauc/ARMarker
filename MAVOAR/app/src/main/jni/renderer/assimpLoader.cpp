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

#include <renderer/assimpLoader.h>
#include <renderer/myShader.h>
#include <utils/misc.h>
#include <opencv2/opencv.hpp>

#include <renderer/myJNIHelper.h>


/**
 * Class constructor, loads shaders & gets locations of variables in them
 */
AssimpLoader::AssimpLoader() {
    importerPtr = new Assimp::Importer;
    scene = NULL;
    isObjectLoaded = false;



    CheckGLError("AssimpLoader::AssimpLoader");
}

/**
 * Class destructor, deletes Assimp importer pointer and removes 3D model from GL
 */
AssimpLoader::~AssimpLoader() {
    Delete3DModel();
    if(importerPtr) {
        delete importerPtr;
        importerPtr = NULL;
    }
    scene = NULL; // gets deleted along with importerPtr
}

/**
 * Generate buffers for vertex positions, texture coordinates, faces -- and load data into them
 */
void AssimpLoader::GenerateGLBuffers() {



    MeshInfo* newMeshInfo= new MeshInfo; // this struct is updated for each mesh in the model
    GLuint buffer;

    // For every mesh -- load face indices, vertex positions, vertex texture coords
    // also copy texture index for mesh into newMeshInfo.textureIndex
    for (unsigned int n = 0; n < scene->mNumMeshes; ++n) {

        const aiMesh *mesh = scene->mMeshes[n]; // read the n-th mesh

        // create array with faces
        // convert from Assimp's format to array for GLES
        newMeshInfo->indices = new unsigned short[mesh->mNumFaces * 3];
        unsigned int faceIndex = 0;

        for (unsigned int t = 0; t < mesh->mNumFaces; ++t) {

            // read a face from assimp's mesh and copy it into faceArray
            const aiFace *face = &mesh->mFaces[t];
            newMeshInfo->indices[faceIndex]=(unsigned short)face->mIndices[0];
            newMeshInfo->indices[faceIndex+1]=(unsigned short)face->mIndices[1];
            newMeshInfo->indices[faceIndex+2]=(unsigned short)face->mIndices[2];


            faceIndex += 3;
        }

        newMeshInfo->numberOfFaces = mesh->mNumFaces;
        newMeshInfo->nIndices = mesh->mNumFaces*3;

        // buffer for vertex positions
        if (mesh->HasPositions()) {
            newMeshInfo->nVertices=mesh->mNumVertices;
            newMeshInfo->vertices=(float*)malloc(mesh->mNumVertices*3*sizeof(float));

            for(unsigned int t = 0; t < mesh->mNumVertices; ++t){
                   newMeshInfo->vertices[t*3]=mesh->mVertices[t][0];
                   newMeshInfo->vertices[(t*3)+1]=mesh->mVertices[t][1];
                   newMeshInfo->vertices[(t*3)+2]=mesh->mVertices[t][2];
               }

        }

        // buffer for vertex texture coordinates
        // ***ASSUMPTION*** -- handle only one texture for each mesh
        if (mesh->HasTextureCoords(0)) {
            newMeshInfo->texCoords = new float[2 * mesh->mNumVertices];
            for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
                newMeshInfo->texCoords[k * 2] = mesh->mTextureCoords[0][k].x;
                newMeshInfo->texCoords[k * 2 + 1] = mesh->mTextureCoords[0][k].y;
            }
        }

        /* unbind buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);*/

        // copy texture index (= texture name in GL) for the mesh from textureNameMap
        aiMaterial *mtl = scene->mMaterials[mesh->mMaterialIndex];
        aiString texturePath;	//contains filename of texture
        if (AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath)) {
            unsigned int textureId = textureNameMap[texturePath.data];
            newMeshInfo->textureIndex = textureId;
        } else {
            newMeshInfo->textureIndex = 0;
        }

        modelMeshes.push_back(newMeshInfo);
    }
}

/**
 * Read textures associated with all materials and load images to GL
 */
bool AssimpLoader::LoadTexturesToGL(std::string modelFilename,const char* folder) {

    // read names of textures associated with all materials
    textureNameMap.clear();

    for (unsigned int m = 0; m < scene->mNumMaterials; ++m) {

        int textureIndex = 0;
        aiString textureFilename;
        aiReturn isTexturePresent = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE,
                                                                     textureIndex,
                                                                     &textureFilename);

        while (isTexturePresent == AI_SUCCESS) {
            //fill map with textures, OpenGL image ids set to 0
            textureNameMap.insert(std::pair<std::string, GLuint>(textureFilename.data, 0));

            // more textures? more than one texture could be associated with a material
            textureIndex++;
            isTexturePresent = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE,
                                                                textureIndex, &textureFilename);
        }
    }

    int numTextures = (int) textureNameMap.size();
    MyLOGI("Total number of textures is %d ", numTextures);

    // create and fill array with texture names in GL
    GLuint * textureGLNames = new GLuint[numTextures];

    // Extract the directory part from the file name
    // will be used to read the texture
    std::string modelDirectoryName = GetDirectoryName(modelFilename);

    // iterate over the textures, read them using OpenCV, load into GL
    std::map<std::string, GLuint>::iterator textureIterator = textureNameMap.begin();
    int i = 0;
    for (; textureIterator != textureNameMap.end(); ++i, ++textureIterator) {

        std::string textureFilename = (*textureIterator).first;  // get filename
        std::string test;

        std::string fld(folder);
        gHelperObject->ExtractAssetReturnFilename(fld+"/"+textureFilename, test);

        std::string textureFullPath = modelDirectoryName + "/" + textureFilename;
        (*textureIterator).second = textureGLNames[i];	  // save texture id for filename in map

        // load the texture using OpenCV
        MyLOGI("Loading texture %s", textureFullPath.c_str());
        cv::Mat textureImage = cv::imread(textureFullPath);
        if (!textureImage.empty()) {

            // opencv reads textures in BGR format, change to RGB for GL
            cv::cvtColor(textureImage, textureImage, CV_BGR2RGB);
            // opencv reads image from top-left, while GL expects it from bottom-left
            // vertically flip the image
            cv::flip(textureImage, textureImage, 0);

            texturesCV.push_back(textureImage);

            CheckGLError("AssimpLoader::loadGLTexGen");

        } else {

            MyLOGE("Couldn't load texture %s", textureFilename.c_str());

            //Cleanup and return
            delete[] textureGLNames;
            return false;

        }
    }

    //Cleanup and return
    delete[] textureGLNames;
    return true;
}

std::vector<cv::Mat> AssimpLoader::getTextures(){
    return texturesCV;
}
/**
 * Loads a general OBJ with many meshes -- assumes texture is associated with each mesh
 * does not handle material properties (like diffuse, specular, etc.)
 */
bool AssimpLoader::Load3DModel(std::string modelFilename,const char* folder) {

    MyLOGI("Scene will be imported now");
    scene = importerPtr->ReadFile(modelFilename, aiProcessPreset_TargetRealtime_Quality);

    // Check if import failed
    if (!scene) {
        std::string errorString = importerPtr->GetErrorString();
        MyLOGE("Scene import failed: %s", errorString.c_str());
        return false;
    }
    MyLOGI("Imported %s successfully.", modelFilename.c_str());

    if(!LoadTexturesToGL(modelFilename,folder)) {
        MyLOGE("Unable to load textures");
        return false;
    }
    MyLOGI("Loaded textures successfully");

    GenerateGLBuffers();
    MyLOGI("Loaded vertices and texture coords successfully");

    isObjectLoaded = true;
    return true;
}

/**
 * Clears memory associated with the 3D model
 */
void AssimpLoader::Delete3DModel() {
    if (isObjectLoaded) {
        // clear modelMeshes stuff
        for (unsigned int i = 0; i < modelMeshes.size(); ++i) {
            glDeleteTextures(1, &(modelMeshes[i]->textureIndex));
        }
        modelMeshes.clear();

        MyLOGI("Deleted Assimp object");
        isObjectLoaded = false;
    }
}

std::vector<MeshInfo*> AssimpLoader::getMeshes(){
    return modelMeshes;
}
/**
 * Renders the 3D model by rendering every mesh in the object
 */
void AssimpLoader::Render3DModel(GLfloat* mvpMat) {

    if (!isObjectLoaded) {
        return;
    }

    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    //glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, (GLfloat *) mvpMat);

    glActiveTexture(GL_TEXTURE0);
    glUniform1i(textureSamplerLocation, 0);

    unsigned int numberOfLoadedMeshes = modelMeshes.size();

    // render all meshes
    for (unsigned int n = 0; n < numberOfLoadedMeshes; ++n) {
        MyLOGI("MESH");

        // Texture
        if (modelMeshes[n]->textureIndex) {
            glBindTexture( GL_TEXTURE_2D, modelMeshes[n]->textureIndex);
        }

        // Faces
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelMeshes[n]->faceBuffer);

        // Vertices
        glBindBuffer(GL_ARRAY_BUFFER, modelMeshes[n]->vertexBuffer);
        glEnableVertexAttribArray(vertexAttribute);
        glVertexAttribPointer(vertexAttribute, 3, GL_FLOAT, 0, 0, 0);

        // Texture coords
        glBindBuffer(GL_ARRAY_BUFFER, modelMeshes[n]->textureCoordBuffer);
        glEnableVertexAttribArray(vertexUVAttribute);
        glVertexAttribPointer(vertexUVAttribute, 2, GL_FLOAT, 0, 0, 0);

        glDrawElements(GL_TRIANGLES, modelMeshes[n]->numberOfFaces * 3, GL_UNSIGNED_SHORT, 0);

        // unbind buffers
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    }

    CheckGLError("AssimpLoader::renderObject() ");

}
