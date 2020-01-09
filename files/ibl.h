#ifndef IBL_H_INCLUDED
#define IBL_H_INCLUDED

#include <iostream>
#include <fstream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <stdio.h>
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define DIRECTORY "resources/hdr/"

using namespace std;

void GetEnvAndIrrCubemap (unsigned int &envCubemap, unsigned int &irradianceMap, unsigned int &prefilterMap, unsigned int &brdfLUTTexture, string name);
void renderACube(void);
void renderAQuad(void);
bool SetUpCubeMap (string mapType, string name, int width, int height);
void ExportNewTextures (string mapType, string name, int width, int height, unsigned int &textureRef, unsigned int level);
bool AlreadyExists (string pathToImage);

void GetEnvAndIrrCubemap (unsigned int &envCubemap, unsigned int &irradianceMap, unsigned int &prefilterMap, unsigned int &brdfLUTTexture, string name)
{
    string pathToHDR = DIRECTORY + name + "/" + name + ".hdr";
    // Create shaders
    Shader equirectangularToCubemapShader ("resources/shaders/cubemap.vs", "resources/shaders/equirectangular_to_cubemap.frag");
    Shader irradianceShader("resources/shaders/cubemap.vs", "resources/shaders/irradiance_convolution.frag");
    Shader prefilterShader("resources/shaders/cubemap.vs", "resources/shaders/prefilter.frag");
    Shader brdfShader("resources/shaders/brdf.vs", "resources/shaders/brdf.frag");

    // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // pbr: load the HDR environment map
    // ---------------------------------
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(pathToHDR.c_str(), &width, &height, &nrComponents, 0);
    stbi_flip_vertically_on_write(true);

    unsigned int hdrTexture;
    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." <<  std::endl;
    }


    // pbr: setup cubemap to render to and attach to framebuffer
    // ---------------------------------------------------------
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    bool skip = true;

    skip = SetUpCubeMap ("CUBEMAP", name, 512, 512);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    if (!skip)
    {
        // pbr: convert HDR equirectangular environment map to cubemap equivalent
        // ----------------------------------------------------------------------

        equirectangularToCubemapShader.Use();
        glUniform1i (glGetUniformLocation(equirectangularToCubemapShader.Program, "equirectangularMap"), 0);
        glUniformMatrix4fv (glGetUniformLocation(equirectangularToCubemapShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);


        glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        // glBindFramebuffer(GL_READ_FRAMEBUFFER, captureFBO);
        // glReadBuffer(GL_COLOR_ATTACHMENT0);
        for (unsigned int i = 0; i < 6; i++)
        {
            glUniformMatrix4fv (glGetUniformLocation(equirectangularToCubemapShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderACube();
        }

        ExportNewTextures ("CUBEMAP", name, 512, 512, envCubemap, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

// then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

// pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
// --------------------------------------------------------------------------------
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    // If it already exists, set to skip
    skip =  SetUpCubeMap ("IRRMAP", name, 32, 32);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    if (!skip)
    {
// pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
// -----------------------------------------------------------------------------
        irradianceShader.Use();
        glUniform1i (glGetUniformLocation(irradianceShader.Program, "environmentMap"), 0);
        glUniformMatrix4fv (glGetUniformLocation(irradianceShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv (glGetUniformLocation(irradianceShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderACube();
        }

        ExportNewTextures("IRRMAP", name, 32, 32, irradianceMap, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

// pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
// --------------------------------------------------------------------------------

    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

    // If it already exists, set to skip
    skip =  SetUpCubeMap ("PREMAP_0", name, 128, 128);


    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minifcation filter to mip_linear
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);



// pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
// ----------------------------------------------------------------------------------------------------
    if (!skip)
    {
        prefilterShader.Use();
        glUniform1i (glGetUniformLocation(prefilterShader.Program, "environmentMap"), 0);
        glUniformMatrix4fv (glGetUniformLocation(prefilterShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth  = 128 * std::pow(0.5, mip);
            unsigned int mipHeight = 128 * std::pow(0.5, mip);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            glUniform1f(glGetUniformLocation(prefilterShader.Program, "roughness"), roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                glUniformMatrix4fv (glGetUniformLocation(prefilterShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                renderACube();
            }
        }

        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            char * numberChar = new char [6];
            (itoa(mip, numberChar, 10));
            string number = string(numberChar);
            delete [] numberChar;
            ExportNewTextures("PREMAP_" + number, name, 128 * std::pow(0.5, mip), 128 * std::pow(0.5, mip), prefilterMap, mip);
        }

    }
    else
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        // Load mip maps
        for (int i = 0; i < 6; i++)
        {
            for (int level = 1; level < 5; level++)
            {
                data = NULL;
                // Set up image reading
                char * numberChar = new char [6];
                (itoa(level, numberChar, 10));
                string levelNumber = string(numberChar);
                (itoa(i, numberChar, 10));
                string number = string(numberChar);
                delete [] numberChar;

                int nrComponents;
                // Try to read the image into the texture
                data = stbi_loadf(string(DIRECTORY + name + "/" + name + "_PREMAP_" + levelNumber + "_" + number + ".hdr").c_str(), &width, &height, &nrComponents, 0);
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, GL_RGB16, width, height, 0, GL_RGB, GL_FLOAT, data);
                if (data) stbi_image_free(data);
            }
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
// pbr: generate a 2D LUT from the BRDF equations used.
// ----------------------------------------------------
    glGenTextures(1, &brdfLUTTexture);

// pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    if (!AlreadyExists( string (string(DIRECTORY) + "BRDF_LUT.hdr")))
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

        glViewport(0, 0, 512, 512);
        brdfShader.Use();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderAQuad();

        width = 512;
        height = 512;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        GLfloat * imageData = new GLfloat [width*height*3];
        //write
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, imageData);
        // imageData = stbi_load( string(DIRECTORY + name + "/" + name + "_preview.jpg").c_str(), &width, &height, &nrComponents, 0);
        stbi_write_hdr(string(string (DIRECTORY) + "BRDF_LUT.hdr").c_str(), width, height, 3, imageData);
    }
    else
    {
        int nrComponents;
        data = stbi_loadf(string(string(DIRECTORY) + "BRDF_LUT.hdr").c_str(), &width, &height, &nrComponents, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, data);
        if (data) stbi_image_free(data);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderACube()
{
    unsigned int cubeVAO = 0;
    unsigned int cubeVBO = 0;
    // initialize (if necessary)
    if (true)
    {
        float vertices[] =
        {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
            1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
            1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            1.0f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,  // bottom-right
            1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
            1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderAQuad()
{
    unsigned int quadVAO = 0;
    unsigned int quadVBO;
    if (true)
    {
        float quadVertices[] =
        {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// Check if the desired texture already exists
bool SetUpCubeMap (string mapType, string name, int width, int height)
{
    bool skip = true;
    // If it already exists, set to skip
    for (unsigned int i = 0; i < 6; i++)
    {
        char * numberChar = new char [6];
        (itoa(i, numberChar, 10));
        string number = string(numberChar);
        delete [] numberChar;
        if (!AlreadyExists(DIRECTORY + name + "/" + name + "_" + mapType + "_" + number + ".hdr"))
        {
            skip = false;
            break;
        }
    }

    float *data = NULL;

    for (unsigned int i = 0; i < 6; i++)
    {
        data = NULL;
        // Set up image reading
        char * numberChar = new char [6];
        (itoa(i, numberChar, 10));
        string number = string(numberChar);
        delete [] numberChar;

        int nrComponents;
        // Try to read the image into the texture
        if (skip)
        {
            data = stbi_loadf(string(DIRECTORY + name + "/" + name + "_" + mapType + "_" + number + ".hdr").c_str(), &width, &height, &nrComponents, 0);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16, width, height, 0, GL_RGB, GL_FLOAT, data);
            if (!data) cout << "There was a problem loading images" << endl;
        }
        else
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16, width, height, 0, GL_RGB, GL_BYTE, data);
        }

        if (data) stbi_image_free(data);
    }

    return skip;
}

void ExportNewTextures (string mapType, string name, int width, int height, unsigned int &textureRef, unsigned int level)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureRef);
    for (unsigned int i = 0; i < 6; i++)
    {
        char * numberChar = new char [6];
        (itoa(i, numberChar, 10));
        string number = string(numberChar);
        delete [] numberChar;
        // Image Writing
        GLfloat * imageData = new GLfloat [width*height*3];
        //write
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, GL_RGB, GL_FLOAT, imageData);
        // imageData = stbi_load( string(DIRECTORY + name + "/" + name + "_preview.jpg").c_str(), &width, &height, &nrComponents, 0);
        stbi_write_hdr(string(DIRECTORY + name + "/" + name + "_" + mapType + "_" + number + ".hdr").c_str(), width, height, 3, imageData);

        delete [] imageData;
    }
}

// Check if the desired texture already exists
bool AlreadyExists (string pathToImage)
{
    ifstream fin;
    fin.open (pathToImage.c_str());
    if (!fin.is_open())
    {
        //fin.close();
        return false;
    }
    else
    {
        fin.close();
        return true;
    }
}

// If it doesn't already exist, make it
// Once it exists, bind it to the cubemap texture

// Load image if it exists
#endif // IBL_H_INCLUDED
