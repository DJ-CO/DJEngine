#include <iostream>

#include <stdio.h>
#include <glew.h>
#include <SDL.h>
//#include <SDL2/SDL_mixer.h>
#include <SDL_mixer.h>
#include <SDL_image.h>
#include <SDL_opengl.h>
#include <time.h>

// TRANSFORMATIONS
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
// Custom Shaders
#include "files/shader.h"
// Include camera class
#include "files/camera.h"
// PBR things
#include "files/pbr.h"
#include "files/ibl.h"
#include "mesh.h"
#include "files/model.h"
#include "files/object.h"
#include "files/skybox.h"
#include "files/globalIllumination.h"



// Textures
#define STB_IMAGE_IMPLEMENTATION
#include "files/stb_image.h"

#define PI 3.14159265359// A PI constant because I think glm works in radians
#define NUMBER_OF_OBJECTS 4//I don't want to just have a magic number, so I'm defining the number of objects here.
// In a real game engine, they'd have hundreds of models and would probably just use a vector instead of manually counting
#define NUMBER_OF_LIGHTS 2// The number of lights
#define POINT 0// Defines for the types of lights
#define DIRECTIONAL 1
#define SPOT 2

using namespace std;

const int WIDTH = 800, HEIGHT = 600;

int SCREEN_WIDTH = WIDTH, SCREEN_HEIGHT = HEIGHT;
// Function Prototypes
// Get keyboard input
//void KeyCallback(SDL_Window *window, int key, int scancode, int action, int mode);
// Function to control camera movement
void DoMovement (SDL_Event event);
// GEt keys
void GetKeys (SDL_Event event);
void renderCube(void);

//create camera
Camera camera(glm::vec3 (0.0f, 0.0f, 3.0f));
// Lastx, relative to middle of the screen
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// For reading keyboard
const Uint8 *keys = SDL_GetKeyboardState(NULL);
// Position of light
glm::vec3 lightPos (1.2f, 1.0f, 2.0f);

int main(int argc, char *argv[])
{
    //**********************************************************************************************//
    // INITIALIZE SDL                                                                               //
    //**********************************************************************************************//
    SDL_Init(SDL_INIT_VIDEO);                                                                       // Initializes the specific part of SDL that can use the opengl window
    //
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);                  // Makes code forward compatable???
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);                                           //
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);                                           //
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);                                                    //
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    //
    SDL_Window* window = SDL_CreateWindow("OpenGL", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);    // Create a window variable and stencil buffer
    //
    SDL_GLContext context = SDL_GL_CreateContext(window);                                           // Create context. Must be deleted at the end.
    //
    SDL_ShowCursor(SDL_DISABLE);                                                                    // Hide Cursor
    //
    glewExperimental = GL_TRUE;                                                                     //
    glewInit();                                                                                     //
    //
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)															// Initialize everything, and print an error if it doesn't initialize correctly
    {
        //
        cout << "SDL could not initialize! SDL error: " << SDL_GetError() << endl;                  //
    }                                                                                               //
    //
    if (NULL == window)																				// Print error if window hasn't been created correctly
    {
        //
        cout << "SDL could not create window! SDL error: " << SDL_GetError() << endl;               //
        return -1;                                                                                  //
    }                                                                                               //
    //
    SDL_Event windowEvent;                                                                          // Create SDL window event
    //
    //**********************************************************************************************//
    // END INITIALIZE SDL                                                                           //
    //**********************************************************************************************//

    // Setup OpenGL options
    glEnable(GL_MULTISAMPLE); // Enabled by default on some drivers, but not all so always enable to make sure
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //******************************************************************************************//
    // SHADERS                                                                                  //
    //******************************************************************************************//
    // Enable depth testing
    //
    //
    //
    Shader shader ("resources/shaders/reflection.vs", "resources/shaders/reflection.frag");	    // Create variable for main shader
    Shader skyboxShader ("resources/shaders/skybox.vs", "resources/shaders/skybox.frag");       // Create variable for skybox shader
    Shader PBR_Shader ("resources/shaders/pbr.vs", "resources/shaders/pbr.frag");
    Shader backgroundShader("resources/shaders/background.vs", "resources/shaders/background.frag");

    vector<string> faces;                                                                       // Create vector of the cube map face textures
    faces.push_back("resources/images/skybox/right.jpg");                                       //
    faces.push_back("resources/images/skybox/left.jpg");                                        //
    faces.push_back("resources/images/skybox/top.jpg");                                         //
    faces.push_back("resources/images/skybox/bottom.jpg");                                      //
    faces.push_back("resources/images/skybox/front.jpg");                                       //
    faces.push_back("resources/images/skybox/back.jpg");                                        //
    //
    //unsigned int cubemapTexture = LoadCubeMap(faces);                                           // Create a variable that holds the cubemap texture
    //******************************************************************************************//
    // END SHADERS                                                                              //
    //******************************************************************************************//

    // Set up both shaders
    PBR_Shader.Use();
    glUniform1i(glGetUniformLocation (PBR_Shader.Program, "irradianceMap"), 6);
    glUniform1i(glGetUniformLocation (PBR_Shader.Program, "prefilterMap"), 7);
    glUniform1i(glGetUniformLocation (PBR_Shader.Program, "brdfLUT"), 8);

    backgroundShader.Use();
    glUniform1i(glGetUniformLocation (backgroundShader.Program, "environmentMap"), 0);

    // Test getting HDR cubemap
    unsigned int envCubemap;
    unsigned int irradianceMap;
    unsigned int prefilterMap;
    unsigned int brdfLUTTexture;
    GetEnvAndIrrCubemap(envCubemap, irradianceMap, prefilterMap, brdfLUTTexture, "Newport_Loft");

    // Set up object vector
    vector <Object> objects;
    objects.push_back(Object(glm::vec3 (5.0f,0.0f,0.0f), glm::vec3 (0.0f,0.0f,0.0f), glm::vec3 (1.0f,1.0f,1.0f), "resources/models/Cube2/Cube.obj"));
    objects.push_back(Object(glm::vec3 (0.0f,0.0f,0.0f), glm::vec3 (PI/4,0.0f,0.0f), glm::vec3 (1.0f,1.0f,1.0f), "resources/models/Cube2/Cube.obj"));
    objects.push_back(Object(glm::vec3 (0.0f,-1.0f,0.0f), glm::vec3 (0.0f,0.0f,0.0f), glm::vec3 (1.0f,1.0f,1.0f), "resources/models/Floor/TestScene.obj"));
    objects.push_back(Object(glm::vec3 (0.0f,0.0f,5.0f), glm::vec3 (0.0f,0.0f,0.0f), glm::vec3 (1.0f,1.0f,1.0f), "resources/models/TestModel/TestModel.obj"));

    // Load object all models
    for (int i = 0; i < objects.size(); i++)
    {
        objects[i].model.LoadModel(objects[i].meshDir);
    }

    vector <Light> lights;// LOL

    lights.push_back(Light( glm::vec3(0.5f, 0.0f, 5.0f), glm::vec3(100.0f, 100.0f, 100.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, 1.0f, POINT, "resources/models/MatTestSphere/MatTestSphere.obj"));
    lights.push_back(Light( glm::vec3(-0.5f, 0.0f, 5.0f), glm::vec3(100.0f, 100.0f, 100.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, 1.0f, POINT,"resources/models/MatTestSphere/MatTestSphere.obj"));
    lights.push_back(Light( glm::vec3(0.0f, 0.0f, 6.5f), glm::vec3(100.0f, 100.0f, 100.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, 1.0f, POINT,"resources/models/MatTestSphere/MatTestSphere.obj"));
    lights.push_back(Light( glm::vec3(0.0f, 0.0f, 4.5f), glm::vec3(100.0f, 100.0f, 100.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 1.0f, 1.0f, POINT,"resources/models/MatTestSphere/MatTestSphere.obj"));

    // Load all light models
    for (int i = 0; i < lights.size(); i++)
    {
        lights[i].model.LoadModel(lights[i].meshDir);
    }


    float skyboxVertices[] =
    {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };

    /*
    // SKYBOX VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    */

    // Projection type      //          // Projection Type//Field of view//Aspect ratio        // Near clip // Far clip
    glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH/(GLfloat)SCREEN_HEIGHT, 0.1f, 1000.0f);

    PBR_Shader.Use();
    glUniformMatrix4fv (glGetUniformLocation(PBR_Shader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    backgroundShader.Use();
    glUniformMatrix4fv (glGetUniformLocation(backgroundShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // then before rendering, configure the viewport to the original framebuffer's screen dimensions
    glViewport(0, 0, WIDTH, HEIGHT);

    //glViewport(0, 0, 1, 1);

    GLfloat fps = 0;
    clock_t t = clock();
    int frames = 0;

    //**********************************************************************************************************************//
    // MAIN LOOP                                                                                                            //
    //**********************************************************************************************************************//
    while (true)																											// Loop forever
    {
        GLfloat currentFrame = SDL_GetTicks();                                                                              // Set up frame independent time
        deltaTime = currentFrame - lastFrame;                                                                               //
        lastFrame = currentFrame;                                                                                           //
        //
        if (SDL_PollEvent(&windowEvent))																	                // Check if something is happening with the window
        {
            //
            if (SDL_QUIT == windowEvent.type)																				// Check if what's happening with the window is the quit event
            {
                //
                break;																										// If the quit event has been triggered, break the loop, thus exiting the program
            }

            if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
            {
                break;
            }
        }

        /* Get FPS */
        if (frames < 1000)
        {
            frames ++;
        }
        else
        {
            t = clock() - t;
            printf("<%f>\n", (frames / ( (float) t / CLOCKS_PER_SEC) ) );
            frames = 0;
            t = clock();
        }
        /*
        <280.190530>
        <295.159386>
        <289.184500>
        <303.030303>
        <297.973778>
        <297.176820>

        <826.446281>
        <1116.071429>
        <1119.820829>
        <1082.251082>
        <1153.402537>
        <1158.748552>
        <1142.857143>
        <1142.857143>
        <1145.475372>
        <1014.198783>
        <1138.952164>
        <1137.656428>
        <1168.224299>
        <1113.585746>
        <1095.290252>
        <1123.595506>
        <1142.857143>
        <1144.164760>
        <1145.475372>
        <1142.857143>
        <1072.961373>
        <1124.859393>
        <1124.859393>
        <1133.786848>
        <1123.595506>
        <1069.518717>
        */

        //cout << "FPS = " << 1/(deltaTime/1000) << endl;
        // Handle the movement of the camera
        DoMovement(windowEvent);

        // RENDER
        //

        // Clear screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader and set up some ititial values
        PBR_Shader.Use();
        GLint viewPosLoc = glGetUniformLocation( PBR_Shader.Program, "viewPos");
        glUniform3f (viewPosLoc, camera.GetPosition( ).x, camera.GetPosition( ).y, camera.GetPosition().z );
        //glUniform1f(glGetUniformLocation(PBR_Shader.Program, "material.shininess"), 32.0f);
        glUniform1i(glGetUniformLocation(PBR_Shader.Program, "NUMBER_OF_LIGHTS"), NUMBER_OF_LIGHTS);

        // Create camera transformation
        glm::mat4 view;
        view = camera.GetViewMatrix();
        GLint modelLoc = glGetUniformLocation ( PBR_Shader.Program, "model");
        GLint viewLoc = glGetUniformLocation ( PBR_Shader.Program, "view");
        GLint projLoc = glGetUniformLocation ( PBR_Shader.Program, "projection");

        glUniformMatrix4fv ( viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv ( projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // bind pre-computed IBL data
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);

        DrawAllLights(PBR_Shader, lights);

        // For loop to draw all light meshes
        // For loop to set all objects

        for (int i = 0; i < objects.size(); i++)
        {
            glm::mat4 model; // Prepare to apply all transformations to all models
            model = glm::translate(model, lights[i].location); // Apply translations
            model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f)); // Apply dilation
            glUniformMatrix4fv (glGetUniformLocation (PBR_Shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model)); // Apply all transformations
            // still need to draw the model
            lights[i].model.Draw(PBR_Shader);
        }


        // For loop to set all objects
        for (int j = 0; j < 1; j++)
        {

        for (int i = 0; i < objects.size(); i++)
        {
            glm::mat4 model; // Prepare to apply all transformations to all models

            model = glm::translate(model, objects[i].location); // Apply translations
            model = glm::scale(model, objects[i].scale); // Apply dilation
            model = glm::rotate(model, objects[i].rotation.z, glm::vec3(0.0f,0.0f,1.0f)); // Rotate on z axis
            model = glm::rotate(model, objects[i].rotation.y, glm::vec3(0.0f,1.0f,0.0f)); // Rotate on y axis
            model = glm::rotate(model, objects[i].rotation.x, glm::vec3(1.0f,0.0f,0.0f)); // Rotate on x axis

            glUniformMatrix4fv (glGetUniformLocation (PBR_Shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model)); // Apply all transformations

            // still need to draw the model
            objects[i].model.Draw(PBR_Shader);
        }

        }

        viewLoc = glGetUniformLocation ( skyboxShader.Program, "view");
        projLoc = glGetUniformLocation ( skyboxShader.Program, "projection");

        // render skybox (render as last to prevent overdraw)
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        backgroundShader.Use();
        glUniformMatrix4fv (glGetUniformLocation(backgroundShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        renderCube();
        glDepthFunc(GL_LESS); // set depth function back to default


        // Swap screen buffers
        SDL_GL_SwapWindow(window);
    }
    //**********************************************************************************************************************//
    // END MAIN LOOP                                                                                                        //
    //**********************************************************************************************************************//


    //**********************************************************************************************************************//
    // CLEAN UP                                                                                                             //
    //**********************************************************************************************************************//
    SDL_DestroyWindow(window);																								// Destroy the window before exiting
    SDL_GL_DeleteContext(context);                                                                                          //
    SDL_Quit();																												// Quit SDL
    return 0;                                                                                                               //
    //**********************************************************************************************************************//
    // END CLEAN UP                                                                                                         //
    //**********************************************************************************************************************//																												// Quit program
}


// FUNCTIONS
void DoMovement(SDL_Event event)
{
    //--------------------------------------------------------------------------------------------------------------------------------------------
    // IDEA: Make global variables for key presses, that way this function can set the variables, while other functions can use them independently
    //--------------------------------------------------------------------------------------------------------------------------------------------
    // RESET MOUSE POSITION
    SDL_WarpMouseInWindow(NULL, WIDTH/2, HEIGHT/2);

    // IF key was up or w
    if ( (keys [SDL_SCANCODE_UP] ) || (keys [SDL_SCANCODE_W]) )
    {
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    // IF key was right or d
    if ( (keys [SDL_SCANCODE_RIGHT] ) || (keys [SDL_SCANCODE_D]) )
    {
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }
    // IF key was down or s
    if ( (keys [SDL_SCANCODE_DOWN] ) || (keys [SDL_SCANCODE_S]) )
    {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    // IF key was left or a
    if ( (keys [SDL_SCANCODE_LEFT] ) || (keys [SDL_SCANCODE_A]) )
    {
        camera.ProcessKeyboard(LEFT, deltaTime);
    }


// Check Mouse stuff
    if (event.type == SDL_MOUSEMOTION)
    {
        GLfloat xOffset = 0, yOffset = 0;

        // Find offset relative to the mouse's rest position at the centre of the screen
        xOffset = event.motion.x - WIDTH/2;
        yOffset = HEIGHT/2 - event.motion.y ;

        camera.ProcessMouseMovement(xOffset, yOffset);
    }

}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
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
