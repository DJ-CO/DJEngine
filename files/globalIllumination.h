#ifndef GLOBALILLUMINATION_H_INCLUDED
#define GLOBALILLUMINATION_H_INCLUDED

/***********
This header is holds functions that control the process of generating Global Illumination
************/

/* Function declarations */

void DrawQuad (void);

/**********************
DEVELOPMENT PLAN

1) Render the scene to retrieve/calculate the normal vector of each fragment,
   along with with the location of relevant light textures.
   a) We can do this a few ways. All ways will require

2) Render the scene as a 1x1 pixel using each normal vector as the view direction
   (the rotation of view around the normal vector doesn’t matter since the final result is a single pixel)

3) Paint each resulting pixel to its corresponding light texture.

4) Repeat steps 2 and 3 using each object’s light texture as lighting. Each
   repetition will increase the number of light bounces

5) Render the final version of the scene.


***********************/

/********************
DrawQuad: Draws a screen-filling quad
in: none
out: none
Post: A screen-filling quad is drawn
*********************/
void DrawQuad (void)
{
// initialize (if necessary)
    GLuint quadVAO = 0;

    if (quadVAO == 0)
    {
        float vertices[] = {
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVAO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, quadVAO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(quadVAO);
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
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
#endif // GLOBALILLUMINATION_H_INCLUDED
