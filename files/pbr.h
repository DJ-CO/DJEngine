#ifndef PBR_H_INCLUDED
#define PBR_H_INCLUDED

#include <string>
#include <iostream>
#include <glew.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <Importer.hpp>
#include <scene.h>
#include <postprocess.h>

using namespace std;

struct Texture
{
    GLint id = -1;
    string type;
    aiString path;
};

class Material
{
private:
    // The authorable vec3 components of the material (for testing only)
    glm::vec3 albedoHolder = glm::vec3(0.5f,0.5f,0.5f);
    float specularHolder = 1;
    glm::vec3 normalHolder = glm::vec3(0.0f,0.0f,0.0f);
    float metallicHolder = 0;
    float roughnessHolder = 0.5;
    float AOHolder = 1;

    // The authorable texture components of the material
    Texture albedoTexture;
    Texture specularTexture;
    Texture normalTexture;
    Texture metallicTexture;
    Texture roughnessTexture;
    Texture AOTexture;

    // A function to author a material
public:
    // Set the textures
    void SetText (Texture &texture, string type)
    {
        if (type == "texture_albedo") albedoTexture = texture;
        if (type == "texture_specular") specularTexture = texture;
        if (type == "texture_normal") normalTexture = texture;
        if (type == "texture_metallic") metallicTexture = texture;
        if (type == "texture_roughness") roughnessTexture = texture;
        if (type == "texture_AO") AOTexture = texture;
    }

    void SetMMaterial (glm::vec3 albedo, float specular, glm::vec3 normal, float metallic, float roughness, float AO)
    {
        this->albedoHolder = albedo;
        this->specularHolder = specular;
        this->normalHolder = normal;
        this->metallicHolder = metallic;
        this->roughnessHolder = roughness;
        this->AOHolder = AO;
    }

    void Draw (Shader &shader)
    {
        glActiveTexture( GL_TEXTURE0 ); // Active proper texture unit before binding
        glUniform1i(glGetUniformLocation(shader.Program, "material.texture_albedo"), 0);
        glBindTexture( GL_TEXTURE_2D, albedoTexture.id );

        glActiveTexture( GL_TEXTURE0 + 1 ); // Active proper texture unit before binding
        glUniform1i(glGetUniformLocation(shader.Program, "material.texture_specular"), 1);
        glBindTexture( GL_TEXTURE_2D, specularTexture.id );

        glActiveTexture( GL_TEXTURE0 + 2 ); // Active proper texture unit before binding
        glUniform1i(glGetUniformLocation(shader.Program, "material.texture_normal"), 2);
        glBindTexture( GL_TEXTURE_2D, normalTexture.id );

        glActiveTexture( GL_TEXTURE0 + 3 ); // Active proper texture unit before binding
        glUniform1i(glGetUniformLocation(shader.Program, "material.texture_metallic"), 3);
        glBindTexture( GL_TEXTURE_2D, metallicTexture.id );

        glActiveTexture( GL_TEXTURE0 + 4 ); // Active proper texture unit before binding
        glUniform1i(glGetUniformLocation(shader.Program, "material.texture_roughness"), 4);
        glBindTexture( GL_TEXTURE_2D, roughnessTexture.id );

        glActiveTexture( GL_TEXTURE0 + 5 ); // Active proper texture unit before binding
        glUniform1i(glGetUniformLocation(shader.Program, "material.texture_AO"), 5);
        glBindTexture( GL_TEXTURE_2D, AOTexture.id );

        // Send info about which textures are missing
        glUniform1i(glGetUniformLocation(shader.Program, "material.hasAL"), albedoTexture.id + 1);
        glUniform1i(glGetUniformLocation(shader.Program, "material.hasSP"), specularTexture.id + 1);
        glUniform1i(glGetUniformLocation(shader.Program, "material.hasNO"), normalTexture.id + 1);
        glUniform1i(glGetUniformLocation(shader.Program, "material.hasME"), metallicTexture.id + 1);
        glUniform1i(glGetUniformLocation(shader.Program, "material.hasRO"), roughnessTexture.id + 1);
        glUniform1i(glGetUniformLocation(shader.Program, "material.hasAO"), AOTexture.id + 1);

    int hasAL;
    int hasSP;
    int hasNO;
    int hasME;
    int hasRO;
    int hasAO;

        glUniform3f(glGetUniformLocation(shader.Program, "material.albedoHolder"), albedoHolder.r, albedoHolder.g, albedoHolder.b);
        glUniform1f(glGetUniformLocation(shader.Program, "material.specularHolder"), specularHolder);
        glUniform3f(glGetUniformLocation(shader.Program, "material.normalHolder"), normalHolder.r, normalHolder.g, normalHolder.b);
        glUniform1f(glGetUniformLocation(shader.Program, "material.metallicHolder"), metallicHolder);
        glUniform1f(glGetUniformLocation(shader.Program, "material.roughnessHolder"), roughnessHolder);
        glUniform1f(glGetUniformLocation(shader.Program, "material.AOHolder"), AOHolder);
    }
};

#endif // PBR_H_INCLUDED
