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

#include <renderer/myGLM.h>
#include <renderer/myGLFunctions.h>
#include <string>
#include <vector>

#include <Vuforia/Vuforia.h>
#include <Vuforia/Vectors.h>
#include <Vuforia/Matrices.h>

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
struct MaterialInfo
{
    std::string Name;
    Vuforia::Vec3F Ambient;
    Vuforia::Vec3F Diffuse;
    Vuforia::Vec3F Specular;
    float Shininess;

    bool isTexture;
    std::string textureName;
    unsigned int mTextureID;
    cv::Mat cvTexture;
};

struct LightInfo
{
    Vuforia::Vec4F Position;
    Vuforia::Vec3F Intensity;
};

struct Mesh
{
    std::string name;
    unsigned int indexOffset;
    MaterialInfo* material;

    float* vertex;
    float* uv;
    unsigned short* indices;
    unsigned int indexCount;

    bool isTexture;
    std::string textureName;

    unsigned int numUVChannels;
    bool hasTangentsAndBitangents;
    bool hasNormals;
    bool hasBones;
};
struct Node{
    std::string name;
    int id;
    Vuforia::Matrix44F transformation;
    std::vector<Mesh*> meshes;
    std::vector<Node> nodes;
};

struct ThreeDModel{
    std::vector<float> m_vertices;
    std::vector<float> m_normals;
    std::vector<unsigned int> m_indices;
    std::vector<std::vector<float/*texture mapping coords*/> > m_textureUV;
    std::vector<float> m_tangents;
    std::vector<float> m_bitangents;
    std::vector<unsigned int /*num components*/> m_numUVComponents; // m_numUVComponents[uvChannelIndex]

    std::vector<MaterialInfo*> m_materials;
    std::vector<Mesh*> m_meshes;
    Node* node;
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

    ThreeDModel* getThreeDModel();

private:
    MaterialInfo* processMaterial(aiMaterial *mater);
    Mesh* processMesh(aiMesh *mesh);
    void processNode(const aiScene *scene, aiNode *node, Node *parentNode, Node &newNode);

    void GenerateGLBuffers();
    bool LoadTexturesToGL(std::string modelFilename,const char* folder);

    std::vector<MeshInfo*> modelMeshes;       // contains one struct for every mesh in model
    std::vector<cv::Mat> texturesCV;       // Textures

    Assimp::Importer *importerPtr;
    const aiScene* scene;                           // assimp's output data structure
    bool isObjectLoaded;


    ThreeDModel* model;       

    std::map<std::string, GLuint> textureNameMap;   // (texture filename, texture name in GL)

    GLuint  vertexAttribute, vertexUVAttribute;     // attributes for shader variables
    GLuint  shaderProgramID;
    GLint   mvpLocation, textureSamplerLocation;    // location of MVP in the shader
};

#endif //ASSIMPLOADER_H
