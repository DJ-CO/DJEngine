#ifndef MODEL_H_INCLUDED
#define MODEL_H_INCLUDED

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include "dirent.h"

#include <glew.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "stb_image.h"
#include <Importer.hpp>
#include <scene.h>
#include <postprocess.h>
#include "mesh.h"

using namespace std;

GLint TextureFromFile( const char *path, string directory );
void CreateFileList (string directory);


class Model
{
public:
    /*  Functions   */
    // Constructor, expects a filepath to a 3D model.
    void LoadModel( GLchar *path )
    {
        this->loadModel( path );
    }

    // Draws the model, and thus all its meshes
    void Draw( Shader shader )
    {
        for ( GLuint i = 0; i < this->meshes.size( ); i++ )
        {
            this->meshes[i].Draw( shader );
        }
    }
    void SetMeshMaterial (Material &mmaterial, int i)
    {
        this->meshes[i].material = mmaterial;
    }

    Material & GetMeshMaterial (int i)
    {
        return this->meshes[i].material;
    }
private:
    /*  Model Data  */
    vector<Mesh> meshes;
    string directory;
    vector<Texture> textures_loaded;	// Stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.

    /*  Functions   */
    // Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel( string path )
    {
        // Read file via ASSIMP
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile( path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace );

        // Check for errors
        if( !scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode ) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString( ) << endl;
            return;
        }
        // Retrieve the directory path of the filepath
        this->directory = path.substr( 0, path.find_last_of( '/' ) );

        // Process ASSIMP's root node recursively
        this->processNode( scene->mRootNode, scene );
    }

    // Processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode( aiNode* node, const aiScene* scene )
    {
        // Process each mesh located at the current node
        for ( GLuint i = 0; i < node->mNumMeshes; i++ )
        {
            // The node object only contains indices to index the actual objects in the scene.
            // The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            this->meshes.push_back( this->processMesh( mesh, scene ) );
        }

        // After we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for ( GLuint i = 0; i < node->mNumChildren; i++ )
        {
            this->processNode( node->mChildren[i], scene );
        }
    }

    Mesh processMesh( aiMesh *mesh, const aiScene *scene )
    {
        // Data to fill
        vector<Vertex> vertices;
        vector<GLuint> indices;
        vector<Texture> textures;

        // Walk through each of the mesh's vertices
        for ( GLuint i = 0; i < mesh->mNumVertices; i++ )
        {
            Vertex vertex;
            glm::vec3 vector; // We declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.

            // Positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;

            // Normals
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;

            // Tangents
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;

            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;

            // Texture Coordinates
            if( mesh->mTextureCoords[0] ) // Does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
            {
                vertex.TexCoords = glm::vec2( 0.0f, 0.0f );
            }

            vertices.push_back( vertex );
        }

        // Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for ( GLuint i = 0; i < mesh->mNumFaces; i++ )
        {
            aiFace face = mesh->mFaces[i];
            // Retrieve all indices of the face and store them in the indices vector
            for ( GLuint j = 0; j < face.mNumIndices; j++ )
            {
                indices.push_back( face.mIndices[j] );
            }
        }

        // Process materials
        if( mesh->mMaterialIndex >= 0 )
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            // We assume a convention for sampler names in the shaders. Each diffuse texture should be named
            // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
            // Same applies to other texture as the following list summarizes:
            // Diffuse: texture_diffuseN
            // Specular: texture_specularN
            // Normal: texture_normalN

            /*
            // 1. Diffuse maps (ALBEDO)
            vector<Texture> diffuseMaps = this->loadMaterialTextures( material, aiTextureType_DIFFUSE, "texture_diffuse" ); //texture_diffuse
            textures.insert( textures.end( ), diffuseMaps.begin( ), diffuseMaps.end( ) );

            // 2. Specular maps (SPECULAR)
            vector<Texture> specularMaps = this->loadMaterialTextures( material, aiTextureType_SPECULAR, "texture_specular" );//texture_specular
            textures.insert( textures.end( ), specularMaps.begin( ), specularMaps.end( ) );

            // 3. Reflection maps (METALLIC)
            vector<Texture> reflectionMaps = this->loadMaterialTextures( material, aiTextureType_SHININESS, "texture_reflection" );//texture_reflection
            textures.insert( textures.end( ), reflectionMaps.begin( ), reflectionMaps.end( ) );
            */

            //TextFromDir(textures, material);
            TexFromFileList(textures, material);
        }

        // Return a mesh object created from the extracted mesh data
        return Mesh( vertices, indices, textures );
    }

    void TexFromFileList (vector<Texture> &textures, aiMaterial *mat)
    {
        CreateFileList(directory);
        // Make input file
        ifstream fin;
        fin.open( string(directory + "/FileList.txt").c_str() );
        if (!fin.is_open())
        {
            cout << "FileList.txt failed to open!" << endl;
            return;
        }
        // Read from FileList.txt and put each file in the string
        vector <string> strVec;
        do
        {
            string strHold;
            // Read line
            getline(fin,strHold);
            // Check each file and see if it has a texture prefix
            if (strHold[0]=='T' && strHold[1]=='_') strVec.push_back(strHold);
        }
        while (!fin.eof());
        // Check each file and see if it has a texture prefix
        fin.close();

        for (int i = 0; i < strVec.size(); i++)
        {
            string str = strVec[i];
            GLboolean skip = false;
            for ( GLuint j = 0; j < textures_loaded.size( ); j++ )
            {
                if( textures_loaded[j].path == aiString(str) )
                {
                    textures.push_back( textures_loaded[j] );
                    skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)

                    break;
                }
            }

            if( !skip )
            {
                // If texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile( str.c_str( ), this->directory );
                texture.path = str;
                if ( str[2] == 'A' && str[3] == 'L' && str[4] == '_' )
                {
                    texture.type = "texture_albedo";
                    textures.push_back( texture );
                }
                if ( str[2] == 'S' && str[3] == 'P' && str[4] == '_')
                {
                    texture.type = "texture_specular";
                    textures.push_back( texture );
                }
                if ( str[2] == 'N' && str[3] == 'O' && str[4] == '_')
                {
                    texture.type = "texture_normal";
                    textures.push_back( texture );
                }
                if ( str[2] == 'M' && str[3] == 'E' && str[4] == '_')
                {
                    texture.type = "texture_metallic";
                    textures.push_back( texture );
                }
                if ( str[2] == 'R' && str[3] == 'O' && str[4] == '_')
                {
                    texture.type = "texture_roughness";
                    textures.push_back( texture );
                }
                if ( str[2] == 'O' && str[3] == 'P' && str[4] == '_')
                {
                    texture.type = "texture_opacity";
                    textures.push_back( texture );
                }
                if ( str[2] == 'A' && str[3] == 'O' && str[4] == '_')
                {
                    texture.type = "texture_AO";
                    textures.push_back( texture );
                }
                if ( str[2] == 'S' && str[3] == 'C' && str[4] == '_')
                {
                    texture.type = "texture_SSColour";
                    textures.push_back( texture );
                }
                if ( str[2] == 'S' && str[3] == 'S' && str[4] == '_')
                {
                    texture.type = "texture_SSS";
                    textures.push_back( texture );
                }
                this->textures_loaded.push_back( texture );  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
    }
};

GLint TextureFromFile( const char *path, string directory )
{
    //Generate texture ID and load texture data
    string filename = string( path );
    filename = directory + '/' + filename;
    GLuint textureID;
    glGenTextures( 1, &textureID );

    int width, height, nrComponents;
    unsigned char *image = stbi_load(filename.c_str( ), &width, &height, &nrComponents, STBI_rgb );
    //unsigned char *image = SOIL_load_image( filename.c_str( ), &width, &height, 0, SOIL_LOAD_RGB );

    // Assign texture to ID
    glBindTexture( GL_TEXTURE_2D, textureID );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image );
    glGenerateMipmap( GL_TEXTURE_2D );

    // Parameters
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture( GL_TEXTURE_2D, 0 );
    stbi_image_free(image);

    return textureID;
}

// If a list doesn't already exist, make a list of the files in the object's directory
void CreateFileList (string directory)
{
    // Check if a file list already exists
    ifstream fin;
    fin.open( string(directory + "/FileList.txt").c_str() );
    // If the file can't open (doesn't exist) then make one
    if (!fin.is_open())
    {
        DIR *dir;
        struct dirent *ent;
        if (((dir = opendir (directory.c_str()))) != NULL)
        {
            ofstream fout;
            fout.open (string(directory + "/FileList.txt").c_str(),ios_base::out|ios_base::app);
            while ((ent = readdir (dir)) != NULL)
            {
                fout << ent->d_name <<endl;
            }
            closedir (dir);
            fout.close();
        }
        else
        {
            /* could not open directory */
            perror ("");
        }
    }
    fin.close();
}

#endif // MODEL_H_INCLUDED
