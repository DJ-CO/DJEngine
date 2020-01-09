#ifndef OBJECT_H_INCLUDED
#define OBJECT_H_INCLUDED

#include "model.h"
#include <iostream>
#include <string>
#include <SDL_opengl.h>
#include <glew.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
// Include everything needed for models?
// Include glm for vector math and stuff

using namespace std;

// Object Class
class Object
{
public:
    glm::vec3 location;// Location
    glm::vec3 rotation;// Rotation
    glm::vec3 scale;// Scale
    glm::vec3 velocity;// Velocity

    float mass;// Mass
    float elasticity;// Elasticity

    bool hidden;// Whether the object should be drawn

    GLchar * meshDir;// Mesh directory for the model

    Model model;// The object's model

    Object (glm::vec3 location, glm::vec3 rotation, glm::vec3 scale, GLchar * meshDir)
    {
        this->location = location;
        this->rotation = rotation;
        this->scale = scale;
        this->meshDir = meshDir;
    }
};

// Light
class Light
{
public:
    glm::vec3 location;// Location
    glm::vec3 diffuse;// RGB of diffuse
    glm::vec3 ambient;// RGB of ambient
    glm::vec3 specular;// RGE of specular
    glm::vec3 direction;// Components of direction
    float constant;// Amount of constant light
    float linear;// Amount of linear falloff
    float quadratic;// Amount of quadratic falloff
    float cutOff;
    float outerCutOff;
    int type;// The type of light: 0 for point light, 1 for directional light, 2 for spot light
    string index;//The "birth number" of the light

    GLchar * meshDir;// Mesh directory for the model

    Model model;// The object's model

    Light (glm::vec3 locaton, glm::vec3 diffuse, glm::vec3 ambient, glm::vec3 direction, float cutOff, float outerCutOff, int type, GLchar * meshDir)
    {
        this->location = locaton;
        this->diffuse = diffuse;
        this->ambient = ambient;
        this->specular = specular;
        this->direction = direction;
        this->cutOff = cutOff;
        this->outerCutOff = outerCutOff;
        this->type = type;
        this->meshDir = meshDir;
    }

    void Draw( Shader shader )// A function meant to be used in a loop to automate the process of passing all uniform information to the fragment shader
    {
        // OpenGL is weird. I need to specify the exact name of the uniform I want to find the location of, but in a GLchar
        // This means that (the way it is intended to be done) you have to explicity declare every single uniform affected
        // by every single light, individually. This is a problem if you have many lights because, even if they were identical
        // and you stored them neatly in an array, you would have to specifically type out the index of each array when activating the lights,
        // defeating the purpose of having them in an array.

        // To remedy this situation, I thought I could have a function (this function) that does the necessary uniform setting
        // for any given light in the light array, and cycle through this function with each different index of light. I thought
        // I could concatinate the string representing the name of the uniform with the index of the light (which is passed in as
        // string) automating the process, but alas, I needed a GLchar *.

        // GLchar *'s don't concatinate, and (while the entire array of characters can be changed) individual indices cannot

        // For instance:
        // GLchar * word = "light[i].position" ;//works
        // word = "light[0].position";// Still works
        // cout << word [6]; //Outputs "0" as expected
        // word [6] = "1";// Gives an error
        // word [6] = '1';// Gives no error *******BUT CRASHES AT RUNTIME*********
        // So simply changing the [index] part of the GLchar * to match the index of the light is impossible

        // However, through some witchcraft, I managed to concatinate some strings, then somehow convert them to a GLchar const * (it only works for const)

        // This is a lot of code, but it automates the process

        string startPath = "light[" + index;// A string for the beginning part of the uniform name (will be constant, except for the index)
        string endPath = "].position";// A string for the end part of the uniform name (will be changed)
        string wholePath = startPath + endPath;// Concatinating the strings into one
        GLchar const * whichUniform = wholePath.c_str();// Setting th GLchar const * to the name of the uniform (concatination cannot be done in this step)
        glUniform3f (glGetUniformLocation(shader.Program, whichUniform), location.x, location.y, location.z); // Setting the uniform based on the light's parameters

        endPath = "].ambient"; // Changing the end of the uniform name to affect the ambient light
        wholePath = startPath + endPath; // Re-concatinating
        whichUniform = wholePath.c_str(); // Changing the GLchar const * to the new re-concatinated variable
        glUniform3f (glGetUniformLocation(shader.Program, whichUniform), ambient.r, ambient.g, ambient.b);

        endPath = "].diffuse"; // The rest is all the same
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform3f (glGetUniformLocation(shader.Program, whichUniform), diffuse.r, diffuse.g, diffuse.b);

        endPath = "].specular";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform3f (glGetUniformLocation(shader.Program, whichUniform), specular.r, specular.g, specular.b );

        endPath = "].direction";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform3f (glGetUniformLocation(shader.Program, whichUniform), direction.x, direction.y, direction.z);


        endPath = "].constant";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform1f (glGetUniformLocation(shader.Program, whichUniform), constant);

        endPath = "].linear";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform1f (glGetUniformLocation(shader.Program, whichUniform), linear);

        endPath = "].quadratic";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform1f (glGetUniformLocation(shader.Program, whichUniform), quadratic);

        endPath = "].cutOff";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform1f (glGetUniformLocation (shader.Program, whichUniform), glm::cos(glm::radians(cutOff)));

        endPath = "].outerCutOff";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform1f (glGetUniformLocation (shader.Program, whichUniform), glm::cos(glm::radians(outerCutOff)));

        endPath = "].type";
        wholePath = startPath + endPath;
        whichUniform = wholePath.c_str();
        glUniform1f (glGetUniformLocation (shader.Program, whichUniform), type);

        // For reference; the non-automated version of the code would look like this
        // glUniform3f (glGetUniformLocation(shader.Program, "light[0].position"), location.x, location.y, location.z);
    }
};

void DrawAllLights(Shader shader, vector <Light> lights );
string name (string index, string parameter);

void DrawAllLights(Shader shader, vector<Light> lights )// A function meant to be used in a loop to automate the process of passing all uniform information to the fragment shader
{
    for (int i = 0; i < lights.size(); i++)
    {
        // Convert index to char pointer
        char * indexChar = new char [lights.size()];
        (itoa(i, indexChar, 10));
        // Convert char pointer to string
        string index = string(indexChar);
        delete [] indexChar;
        // Set uniforms
        glUniform3f(glGetUniformLocation(shader.Program, name(index, "position").c_str() ), lights[i].location.x, lights[i].location.y, lights[i].location.z);
        glUniform3f(glGetUniformLocation(shader.Program, name(index, "diffuse").c_str() ), lights[i].diffuse.r, lights[i].diffuse.g, lights[i].diffuse.b);
        glUniform3f(glGetUniformLocation(shader.Program, name(index, "ambient").c_str() ), lights[i].ambient.r, lights[i].ambient.g, lights[i].ambient.b);
        glUniform3f(glGetUniformLocation(shader.Program, name(index, "direction").c_str() ), lights[i].direction.x, lights[i].direction.y, lights[i].direction.z);
        glUniform1f(glGetUniformLocation(shader.Program, name(index, "cutOff").c_str() ), lights[i].cutOff);
        glUniform1f(glGetUniformLocation(shader.Program, name(index, "outerCutOff").c_str() ), lights[i].outerCutOff);
        glUniform1f(glGetUniformLocation(shader.Program, name(index, "type").c_str() ), lights[i].type);
    }
    // Tell the fragment shader how many lights there are
    glUniform1i (glGetUniformLocation(shader.Program, "LIGHT_AMOUNT" ), lights.size());
}

string name (string index, string parameter)
{
    return string("light[" + index + "]." + parameter);
}
/*
GLchar const * name (string index, string parameter)
{
    GLchar const * path = string("light[" + index + "]." + parameter).c_str();
    return path;
}
*/
#endif // OBJECT_H_INCLUDED
