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

    model= new ThreeDModel();

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

    std::string modelDirectoryName = GetDirectoryName(modelFilename);

    // iterate over the textures, read them using OpenCV, load into GL
    for(int mat=0;mat<model->m_materials.size();mat++){
        if(model->m_materials[mat]->isTexture){
            std::string textureFullPath = modelDirectoryName + "/" + model->m_materials[mat]->textureName;
            MyLOGI("Loading texture %s", textureFullPath.c_str());
            cv::Mat textureImage = cv::imread(textureFullPath);
            if (!textureImage.empty()) {

                // opencv reads textures in BGR format, change to RGB for GL
                cv::cvtColor(textureImage, textureImage, CV_BGR2RGB);
                // opencv reads image from top-left, while GL expects it from bottom-left
                // vertically flip the image
                cv::flip(textureImage, textureImage, 0);

                model->m_materials[mat]->cvTexture= textureImage;

                CheckGLError("AssimpLoader::loadGLTexGen");

            } else {

                MyLOGE("Couldn't load texture %s", textureFullPath.c_str());
                model->m_materials[mat]->isTexture=false;
            }
        }
    }

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
    scene = importerPtr->ReadFile(modelFilename, 
    aiProcess_GenSmoothNormals      |
            aiProcess_CalcTangentSpace       |
            aiProcess_Triangulate       |
            aiProcess_JoinIdenticalVertices  |
            aiProcess_SortByPType);

    // Check if import failed
    if (!scene) {
        std::string errorString = importerPtr->GetErrorString();
        MyLOGE("Scene import failed: %s", errorString.c_str());
        return false;
    }
    MyLOGI("Imported %s successfully.", modelFilename.c_str());

    if(scene->HasMaterials())
    {
        for(unsigned int ii=0; ii<scene->mNumMaterials; ++ii)
        {
            MaterialInfo* mater = processMaterial(scene->mMaterials[ii]);
            model->m_materials.push_back(mater);
        }
    }

    if(scene->HasMeshes())
    {
        for(unsigned int ii=0; ii<scene->mNumMeshes; ++ii)
        {
            model->m_meshes.push_back(processMesh(scene->mMeshes[ii]));
        }
    }
    else
    {
        MyLOGE("Error: No meshes found");
        return false;
    }

    if(scene->HasLights())
    {
        MyLOGI( "Has Lights");
    }

    if(scene->mRootNode != NULL)
    {
        model->node= new Node();
        processNode(scene, scene->mRootNode, 0, *(model->node));
    }
    else
    {
        MyLOGE("Error loading model");
        return false;
    }

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

Mesh* AssimpLoader::processMesh(aiMesh *mesh){
    Mesh* newMesh(new Mesh);
    newMesh->name = mesh->mName.length != 0 ? mesh->mName.C_Str() : "";
    newMesh->indexOffset = model->m_indices.size();
    unsigned int indexCountBefore = model->m_indices.size();
    int vertindexoffset = model->m_vertices.size()/3;

    newMesh->numUVChannels = mesh->GetNumUVChannels();
    newMesh->hasTangentsAndBitangents = mesh->HasTangentsAndBitangents();
    newMesh->hasNormals = mesh->HasNormals();
    newMesh->hasBones = mesh->HasBones();

    // Get Vertices
    newMesh->vertex=(float*)malloc(mesh->mNumVertices*3*sizeof(float));

    if(mesh->mNumVertices > 0)
    {
        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mVertices[ii];

            newMesh->vertex[ii*3] =vec.x; 
            newMesh->vertex[ii*3+1] =vec.y; 
            newMesh->vertex[ii*3+2] =vec.z; 

            model->m_vertices.push_back(vec.x);
            model->m_vertices.push_back(vec.y);
            model->m_vertices.push_back(vec.z);


        }
    }

    // Get Normals
    if(mesh->HasNormals())
    {
        model->m_normals.resize(model->m_vertices.size());

        int nind = vertindexoffset * 3;

        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mNormals[ii];
            model->m_normals[nind] = vec.x;
            model->m_normals[nind+1] = vec.y;
            model->m_normals[nind+2] = vec.z;
            nind += 3;
        };
    }

    if (mesh->HasTextureCoords(0)) {
            newMesh->uv = new float[2 * mesh->mNumVertices];
            for (unsigned int k = 0; k < mesh->mNumVertices; ++k) {
                newMesh->uv[k * 2] = mesh->mTextureCoords[0][k].x;
                newMesh->uv[k * 2 + 1] = mesh->mTextureCoords[0][k].y;
            }
        }
    // Get Texture coordinates
    if(mesh->GetNumUVChannels() > 0)
    {
        if((unsigned int)model->m_textureUV.size() < mesh->GetNumUVChannels()) // Caution, assumes all meshes in this model have same number of uv channels
        {
            model->m_textureUV.resize(mesh->GetNumUVChannels());
            model->m_numUVComponents.resize(mesh->GetNumUVChannels());
        }

        for( unsigned int mchanInd = 0; mchanInd < mesh->GetNumUVChannels(); ++mchanInd)
        {
            //Q_ASSERT(mesh->mNumUVComponents[mchanInd] == 2 && "Error: Texture Mapping Component Count must be 2. Others not supported");

            model->m_numUVComponents[mchanInd] = mesh->mNumUVComponents[mchanInd];
            model->m_textureUV[mchanInd].resize((model->m_vertices.size()/3)*2);

            int uvind = vertindexoffset * model->m_numUVComponents[mchanInd];

            for(uint iind = 0; iind<mesh->mNumVertices; ++iind)
            {
                // U
                model->m_textureUV[mchanInd][uvind] = mesh->mTextureCoords[mchanInd][iind].x;
                if(mesh->mNumUVComponents[mchanInd] > 1)
                {
                    // V
                    model->m_textureUV[mchanInd][uvind+1] = mesh->mTextureCoords[mchanInd][iind].y;
                    if(mesh->mNumUVComponents[mchanInd] > 2)
                    {
                        // W
                        model->m_textureUV[mchanInd][uvind+2] = mesh->mTextureCoords[mchanInd][iind].z;
                    }
                }
                uvind += model->m_numUVComponents[mchanInd];
            }
        }
    }

    // Get Tangents and bitangents
    if(mesh->HasTangentsAndBitangents())
    {
        model->m_tangents.resize(model->m_vertices.size());
        model->m_bitangents.resize(model->m_vertices.size());

        int tind = vertindexoffset * 3;

        for(uint ii=0; ii<mesh->mNumVertices; ++ii)
        {
            aiVector3D &vec = mesh->mTangents[ii];
            model->m_tangents[tind] = vec.x;
            model->m_tangents[tind+1] = vec.y;
            model->m_tangents[tind+2] = vec.z;

            aiVector3D &vec2 = mesh->mBitangents[ii];
            model->m_bitangents[tind] = vec2.x;
            model->m_bitangents[tind+1] = vec2.y;
            model->m_bitangents[tind+2] = vec2.z;

            tind += 3;
        };
    }

    // Get mesh indexes
    newMesh->indices=(unsigned short*)malloc(mesh->mNumFaces*3*sizeof(unsigned short));

    for(uint t = 0; t<mesh->mNumFaces; ++t)
    {
        aiFace* face = &mesh->mFaces[t];
        if(face->mNumIndices != 3)
        {
            MyLOGI("Warning: Mesh face with not exactly 3 indices, ignoring this primitive. %d ",face->mNumIndices);
            continue;
        }
        
        newMesh->indices[t*3+0]=(unsigned short)face->mIndices[0];
        newMesh->indices[t*3+1]=(unsigned short)face->mIndices[1];
        newMesh->indices[t*3+2]=(unsigned short)face->mIndices[2];

        model->m_indices.push_back(face->mIndices[0]+vertindexoffset);
        model->m_indices.push_back(face->mIndices[1]+vertindexoffset);
        model->m_indices.push_back(face->mIndices[2]+vertindexoffset);


    }

    newMesh->indexCount = model->m_indices.size() - indexCountBefore;
    newMesh->material = model->m_materials.at(mesh->mMaterialIndex);

    MyLOGI("Mesh %s %d", newMesh->name.c_str(), newMesh->indexCount);

    return newMesh;
}


 MaterialInfo* AssimpLoader::processMaterial(aiMaterial *material){

    MaterialInfo* mater= new MaterialInfo();

    aiString mname;
    material->Get( AI_MATKEY_NAME, mname);
    if(mname.length > 0)
        mater->Name = mname.C_Str();
    mater->isTexture = false;

    int shadingModel;
    material->Get( AI_MATKEY_SHADING_MODEL, shadingModel );

    if(shadingModel != aiShadingMode_Phong && shadingModel != aiShadingMode_Gouraud)
    {
        MyLOGI( "This mesh's shading model is not implemented in this loader, setting to default material");
        mater->Name = "DefaultMaterial";
    }
    else
    {
        aiColor3D dif (0.f,0.f,0.f);
        aiColor3D amb (0.f,0.f,0.f);
        aiColor3D spec (0.f,0.f,0.f);
        float shine = 0.0;

        material->Get( AI_MATKEY_COLOR_AMBIENT, amb);
        material->Get( AI_MATKEY_COLOR_DIFFUSE, dif); //->Get(<material-key>,<where-to-store>))
        material->Get( AI_MATKEY_COLOR_SPECULAR, spec);
        material->Get( AI_MATKEY_SHININESS, shine);

        mater->Ambient = Vuforia::Vec3F(amb.r, amb.g, amb.b);
        mater->Diffuse = Vuforia::Vec3F(dif.r, dif.g, dif.b);
        mater->Specular = Vuforia::Vec3F(spec.r, spec.g, spec.b);
        mater->Shininess = shine;

        mater->Ambient.data[0] *= .2f;
        mater->Ambient.data[1] *= .2f;
        mater->Ambient.data[2] *= .2f;

        if( mater->Shininess == 0.0) mater->Shininess = 30;

        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            MyLOGI("Diffuse Texture(s) Found: %d: Material: %s", 
            material->GetTextureCount(aiTextureType_DIFFUSE),
            mater->Name.c_str());
            aiString texPath;

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                std::string texturePath = texPath.data;
                mater->isTexture = true;
                mater->textureName = texturePath;
                MyLOGI("Texture path: %s",  texturePath.c_str());
            }
            else
                MyLOGE( "  Failed to get texture for material");
        }
    }

    return mater;
 }

 void AssimpLoader::processNode(const aiScene *scene, aiNode *node, Node *parentNode, Node &newNode)
{
    static int nodeIndex = 0;


    newNode.name = node->mName.length != 0 ? node->mName.C_Str() : "";

    newNode.transformation = Vuforia::Matrix44F();

    for(int tx=0;tx<4;tx++){
        for(int ty=0;ty<4;ty++){
            newNode.transformation.data[tx+(ty*4)]=*node->mTransformation[(tx*4)+ty];
        }
    }

    newNode.meshes.resize(node->mNumMeshes);
    for(int imesh = 0; imesh < node->mNumMeshes; ++imesh)
    {
        Mesh* mesh = model->m_meshes[node->mMeshes[imesh]];

        MyLOGI( "Node %d", model->m_meshes.size()) ;

        newNode.meshes[imesh] = mesh;

       /* if (DEBUGOUTPUT_NORMALS(nodeIndex)) {
            MyLOGI( "Start Print Normals for nodeIndex: %d. MeshIndex: %d. NodeName %s",
            nodeIndex,imesh,newNode.name.c_str());

            for (uint32 ii=newNode.meshes[imesh]->indexOffset;
                 ii<newNode.meshes[imesh]->indexCount+newNode.meshes[imesh]->indexOffset;
                 ++ii) {
                int ind = m_indices[ii] * 3;
                MyLOGI( "%f %f %f",(m_normals[ind]) , (m_normals[ind]+1) , (m_normals[ind]+2));
            }
            MyLOGI( "End Print Normals for nodeIndex: %d. MeshIndex: %d. NodeName %s",
            nodeIndex,imesh,newNode.name.c_str());
        }*/
    }
    MyLOGI( "NodeName %s", newNode.name.c_str()) ;
    MyLOGI( " NodeIndex %d", nodeIndex) ;
    MyLOGI( " NumChildren %d", node->mNumChildren) ;
    MyLOGI( " NumMeshes %d", node->mNumChildren) ;


    for (int ii=0; ii<newNode.meshes.size(); ++ii) {
        MyLOGI( "    MeshName %s", newNode.meshes[ii]->name.c_str()) ;
        MyLOGI( "    MaterialName %s", newNode.meshes[ii]->material->Name.c_str()) ;
        MyLOGI( "    MeshVertices %d", newNode.meshes[ii]->indexCount) ;
        MyLOGI( "    numUVChannels %d", newNode.meshes[ii]->numUVChannels) ;
        MyLOGI( "    hasTangAndBit %d",  newNode.meshes[ii]->hasTangentsAndBitangents) ;
        MyLOGI( "    hasNormals %d", newNode.meshes[ii]->hasNormals) ;
        MyLOGI( "    hasBones %d", newNode.meshes[ii]->hasBones) ;
    }

    ++nodeIndex;

    for(uint ich = 0; ich < node->mNumChildren; ++ich)
    {
        newNode.nodes.push_back(Node());
        processNode(scene, node->mChildren[ich], parentNode, newNode.nodes[ich]);
    }
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

ThreeDModel* AssimpLoader::getThreeDModel(){
    return model;
}
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

