
/* Using the standard output for fprintf */
#include <stdio.h>
#include <stdlib.h>

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/freeglut.h>

#include "shader_utils.h"

#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SOIL.h>

#include<iostream>
using std::cerr;
using std::cout;

#include "MyClasses.h"

/* ADD GLOBAL VARIABLES HERE LATER */
GLuint texture_id;
GLuint program;

xDModel *xdModel = NULL;

int screen_width = 800, screen_height = 600;

int last_mx = 0, last_my = 0, cur_mx = 0, cur_my = 0;
int arcball_on = false;

struct FPSCount
{
    int frame;
    int timebase;
    int time;
} fpsCount = {0, 0, 0};

struct CameraProperty
{
    glm::vec3 eye;
    glm::vec3 at;
    glm::vec3 up;

    GLfloat fov;
} cameraProp;

struct ShaderAttriLoc
{
    GLuint vObjPos;
    GLuint vObjNormal;

    GLuint vTex;
} attriLoc;

struct ShaderUniformLoc
{
    GLuint modelMatrix;
    GLuint viewMatrix;
    GLuint projMatrix;
    GLuint normalMatrix;

    GLuint lightModelMatrix;
    GLuint lightPosition;
    GLuint lightDiffuse;
    GLuint lightAmbient;

    GLuint mat_diffuse;
    GLuint mat_ambient;

    GLuint sampler;
} uniformLoc;

struct Buffers
{
    GLuint vertexBuffer;
    GLuint normalBuffer;
    GLuint textureBuffer;

    GLuint elementBuffer;
} buffers;

struct PointLight
{
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
} plight =
{
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(0.3f, 0.3f, 0.3f, 1.0f),
    glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
};

xDMaterial modelMat =
{
    glm::vec4(0.5f,0.5f,0.5f,1.0f),
    glm::vec4(0.5f,0.5f,0.5f,1.0f),
    glm::vec4(0.5f,0.5f,0.5f,1.0f)
};

void loadTexture()
{

    int width, height, channels;
    unsigned char *ht_map = SOIL_load_image
                            (
                                "fiber.jpg",
                                &width, &height, &channels,
                                SOIL_LOAD_AUTO
                            );
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, ht_map);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

}

void calCameraSet()
{
    cameraProp.up = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraProp.at = xdModel->center;

    cameraProp.fov = 30.0f;

    float distanceToCamera = xdModel->boundingShpereRadius / tanf(M_PI * cameraProp.fov / 360.0f);
    cameraProp.eye = xdModel->center;
    cameraProp.eye.z = distanceToCamera;
}

/*
Function: init_resources
Receives: void
Returns: int
This function creates all GLSL related stuff
explained in this example.
Returns 1 when all is ok, 0 with a displayed error
*/
int init_resources(void)
{
    //loading model
    xdModel = new xDModel("teapot2.obj");
    //
    //setting shader
    GLint link_ok = GL_FALSE;

    GLuint vs, fs;
    if ((vs = create_shader("triangle.vert", GL_VERTEX_SHADER))   == 0) return 0;
    if ((fs = create_shader("triangle.frag", GL_FRAGMENT_SHADER)) == 0) return 0;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok)
    {
        fprintf(stderr, "glLinkProgram:");
        print_log(program);
        return 0;
    }


    ////attributes
    if(!getAttributeLoc(program, "vObjPos", attriLoc.vObjPos)
            || !getAttributeLoc(program, "vObjNormal", attriLoc.vObjNormal)
            || !getAttributeLoc(program, "vTex", attriLoc.vTex)
      )
        return 0;

    //uniforms
    if(
        !getUniformLoc(program, "modelMatrix", uniformLoc.modelMatrix)
        || !getUniformLoc(program, "projMatrix", uniformLoc.projMatrix)
        || !getUniformLoc(program, "viewMatrix", uniformLoc.viewMatrix)
        || !getUniformLoc(program, "normalMatrix", uniformLoc.normalMatrix)
        || !getUniformLoc(program, "light_diffuse", uniformLoc.lightDiffuse)
        || !getUniformLoc(program, "lightModelMatrix", uniformLoc.lightModelMatrix)
        || !getUniformLoc(program, "lightPosition", uniformLoc.lightPosition)
        || !getUniformLoc(program, "mat_diffuse", uniformLoc.mat_diffuse)
        || !getUniformLoc(program, "mat_ambient", uniformLoc.mat_ambient)
        || !getUniformLoc(program, "light_ambient", uniformLoc.lightAmbient)
        || !getUniformLoc(program, "sampler", uniformLoc.sampler)
    )
        return 0;

    loadTexture();
    calCameraSet();

    return 1;
}

glm::mat4 moveToCenter;
glm::mat4 teapotMatrix(1.0f);
glm::mat4 smallTeapotMatrix;

glm::mat4 viewMatrix;
glm::mat4 projectionMatrix;

glm::mat4 lightMatrix;
glm::mat3 normalMatrix;

void onDisplay()
{

    /* Clear the background as white */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);


    glUseProgram(program);

    glEnableVertexAttribArray(attriLoc.vObjPos);
    glEnableVertexAttribArray(attriLoc.vObjNormal);
    glEnableVertexAttribArray(attriLoc.vTex);


    glUniform4fv(uniformLoc.lightDiffuse, 1, glm::value_ptr(plight.diffuse));
    glUniform4fv(uniformLoc.lightAmbient, 1, glm::value_ptr(plight.ambient));
    glUniform4fv(uniformLoc.lightPosition, 1, glm::value_ptr(plight.position));
    glUniform4fv(uniformLoc.mat_diffuse, 1, glm::value_ptr(modelMat.diffuse));
    glUniform4fv(uniformLoc.mat_ambient, 1, glm::value_ptr(modelMat.ambient));

    glUniformMatrix4fv(uniformLoc.lightModelMatrix, 1, GL_FALSE, glm::value_ptr(lightMatrix));

    glUniformMatrix4fv(uniformLoc.modelMatrix, 1, GL_FALSE, glm::value_ptr(teapotMatrix * moveToCenter));
    glUniformMatrix4fv(uniformLoc.viewMatrix, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(uniformLoc.projMatrix, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix3fv(uniformLoc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    GLMmodel *m = xdModel->glmModel;


    //draw the big teapot
    glBegin(GL_TRIANGLES);
    GLMgroup *gg = m->groups;
    while(gg != NULL)
    {
        for(int i = 0; i < gg->numtriangles; i++)
        {
            GLuint triIdx = gg->triangles[i];
            for(int j = 0; j < 3; j++)
            {
                GLuint vIdx = m->triangles[triIdx].vindices[j] * 3;
                GLuint nIdx = m->triangles[triIdx].nindices[j] * 3;
                GLuint tIdx = m->triangles[triIdx].tindices[j] * 2;

                glVertexAttrib3fv(attriLoc.vObjNormal, &m->normals[nIdx]);
                glVertexAttrib3fv(attriLoc.vTex, &m->texcoords[tIdx]);

                glVertexAttrib3fv(attriLoc.vObjPos, &m->vertices[vIdx]);
            }
        }
        gg = gg->next;
    }
    glEnd();


    glUniformMatrix4fv(uniformLoc.modelMatrix, 1, GL_FALSE, glm::value_ptr(smallTeapotMatrix));
    //draw the small teapot
    glBegin(GL_TRIANGLES);
    gg = m->groups;
    while(gg != NULL)
    {
        for(int i = 0; i < gg->numtriangles; i++)
        {
            GLuint triIdx = gg->triangles[i];
            for(int j = 0; j < 3; j++)
            {
                GLuint vIdx = m->triangles[triIdx].vindices[j] * 3;
                GLuint nIdx = m->triangles[triIdx].nindices[j] * 3;
                GLuint tIdx = m->triangles[triIdx].tindices[j] * 2;

                glVertexAttrib3fv(attriLoc.vObjNormal, &m->normals[nIdx]);
                glVertexAttrib3fv(attriLoc.vTex, &m->texcoords[tIdx]);

                glVertexAttrib3fv(attriLoc.vObjPos, &m->vertices[vIdx]);
            }
        }
        gg = gg->next;
    }
    glEnd();

    glDisableVertexAttribArray(attriLoc.vObjPos);
    glDisableVertexAttribArray(attriLoc.vObjNormal);
    glDisableVertexAttribArray(attriLoc.vTex);

    glUseProgram(0);


    /* Display the result */
    glutSwapBuffers();
}

void free_resources()
{
    glDeleteProgram(program);

    if(xdModel)
        delete xdModel;
}

void onReshape(int width, int height)
{
    screen_width = width;
    screen_height = height;
    glViewport(0, 0, screen_width, screen_height);
}

/**
 * Get a normalized vector from the center of the virtual ball O to a
 * point P on the virtual ball surface, such that P is aligned on
 * screen's (X,Y) coordinates.  If (X,Y) is too far away from the
 * sphere, return the nearest point on the virtual ball surface.
 */
glm::vec3 get_arcball_vector(int x, int y)
{
    glm::vec3 P = glm::vec3(1.0*x/screen_width*2 - 1.0,
                            1.0*y/screen_height*2 - 1.0,
                            0);
    P.y = -P.y;
    float OP_squared = P.x * P.x + P.y * P.y;
    if (OP_squared <= 1*1)
        P.z = sqrt(1*1 - OP_squared);  // Pythagore
    else
        P = glm::normalize(P);  // nearest point
    return P;
}

void onIdle()
{

    fpsCount.frame++;
    fpsCount.time = glutGet(GLUT_ELAPSED_TIME);

    if (fpsCount.time - fpsCount.timebase > 1000)
    {
        double fps = fpsCount.frame * 1000.0 / (fpsCount.time - fpsCount.timebase);
        fpsCount.timebase = fpsCount.time;
        fpsCount.frame = 0;
        printf("fps: %f\n", fps);
    }


    moveToCenter = glm::translate(glm::mat4(1.0f), -xdModel->center);

    //translate and rotate camera position
    viewMatrix = glm::lookAt(cameraProp.eye, cameraProp.at, cameraProp.up);

    //projection matrix
    projectionMatrix = glm::perspective(cameraProp.fov, 1.0f*screen_width/screen_height, 0.1f, 1000.0f);

    if (cur_mx != last_mx || cur_my != last_my)
    {
        glm::vec3 va = get_arcball_vector(last_mx, last_my);
        glm::vec3 vb = get_arcball_vector( cur_mx,  cur_my);
        float angle = acos(glm::min(1.0f, glm::dot(va, vb)));
        glm::vec3 axis_in_camera_coord = glm::cross(va, vb);
        glm::mat3 camera2object = glm::mat3(glm::inverse(viewMatrix)) * glm::mat3(teapotMatrix);
        glm::vec3 axis_in_object_coord = camera2object * axis_in_camera_coord;

        teapotMatrix = glm::rotate(teapotMatrix, glm::degrees(angle), axis_in_object_coord);
        last_mx = cur_mx;
        last_my = cur_my;
    }

    //normal matrix
    normalMatrix = glm::mat3(glm::transpose(glm::inverse(viewMatrix * teapotMatrix)));

    //for small teapot, which will rotate around the big teapot
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)) * glm::translate(glm::mat4(1.0f), -xdModel->center);
    glm::mat4 translateNearBig = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    float angle3 = glutGet(GLUT_ELAPSED_TIME) / 1000.0f * 25.0f;
    glm::mat4 rotate3 = glm::rotate(glm::mat4(1.0f), angle3, glm::vec3(0.0f, 1.0f, 0.0f));
    smallTeapotMatrix = rotate3 * translateNearBig * scale;

    //light transformation
    lightMatrix = rotate3 * translateNearBig;

    glutPostRedisplay();
}

void onMouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        arcball_on = true;
        last_mx = cur_mx = x;
        last_my = cur_my = y;
    }
    else
    {
        arcball_on = false;
    }
}

void onMotion(int x, int y)
{
    if (arcball_on)    // if left button is pressed
    {
        cur_mx = x;
        cur_my = y;
    }
}

int main(int argc, char* argv[])
{
    /* Glut-related initialising functions */
    glutInit(&argc, argv);
    glutInitContextVersion(2,0);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_ALPHA);
    glutInitWindowSize(screen_width, screen_height);
    glutCreateWindow("ZZZZZZZZ");

    /* Extension wrangler initialising */
    GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
        return EXIT_FAILURE;
    }

    /* When all init functions run without errors,
    the program can initialise the resources */
    if (1 == init_resources())
    {
        /* We can display it if everything goes OK */
        glutDisplayFunc(onDisplay);
        glutIdleFunc(onIdle);
        glutReshapeFunc(onReshape);

        glutMouseFunc(onMouse);
        glutMotionFunc(onMotion);

        glutMainLoop();
    }

    /* If the program exits in the usual way,
    free resources and exit with a success */
    free_resources();
    return EXIT_SUCCESS;
}