#ifndef SKYBOX_H_INCLUDED
#define SKYBOX_H_INCLUDED

unsigned int LoadCubeMap (vector<std::string> faces);
void RenderSkybox (unsigned int cubemapTexture);

// Function to load a skybox based on six textures
unsigned int LoadCubeMap (vector<std::string> faces)                                                                        // A vector of the paths to the textures of the faces
{
    unsigned int textureID;                                                                                                 // Dummy texture ID to be used in the function
    glGenTextures(1, &textureID);                                                                                           // Generate the texture
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);                                                                          // Bind the texture as a cube map type

    int width, height, nrChannels;                                                                                          // Integers to hold the width, height, and number of channels in the image
    for (unsigned int i = 0; i < faces.size(); i++)                                                                           // Loop through the number of faces
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        //unsigned char *data = SOIL_load_image(faces[i].c_str(), &width, &height, &nrChannels, 0);                           // Create variable with the path and image parameters of the texutre
        if (data)                                                                                                           // If image loaded successfully
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);  // Cycle through the enumerations and load each side of the map
            stbi_image_free(data);
        }
        else                                                                                                                // If image failed to load, print error message
        {
        std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;                                   // Error message
        stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                                                 // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;                                                                                                       // Return the ID of the texture
}

void RenderSkybox (unsigned int cubemapTexture)                                                                             // Function to render the skybox
{
    /*
    glBindVertexArray(skyboxVAO);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthMask(GL_TRUE);
    */
}

// Function to render the skybox


#endif // SKYBOX_H_INCLUDED
