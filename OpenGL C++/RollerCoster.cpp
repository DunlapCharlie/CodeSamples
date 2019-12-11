/*
CSCI 420 Computer Graphics, USC
Assignment 2: Roller Coaster
C++ starter code

Student username: Charlied

designed for understanding not efficiency
run in release mode as debug mode take a solid minuite to load up

double rails
rails dont use a texture, kept the color based off
of the normal on the rail then sent to shader for phong 
sky
phong

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openGLHeader.h"
#include "imageIO.h"
#include "glutHeader.h"
#include "basicPipelineProgram.h"
#include "texturePipelineProgram.h"
#include "openGLMatrix.h"
#include <iostream>
#include <cstring>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

ImageIO * heightmapImage;

//data from slides

float zStudent = 3 + (2948071714 / 10000000000);

OpenGLMatrix openGLMatrix;

BasicPipelineProgram pipelineProgram;
GLint h_modelViewMatrix, h_projectionMatrix;
GLuint program;

int n = 0;
bool anim = false;

GLuint vbo;
GLuint vao;
GLuint buffer;
vector<float> pos;
vector<float> uvs;
vector<float> color;
vector<float> tangent;
vector<float> normal;
vector<float> binormal;
GLuint VBOpos;
GLuint VBOcolor;
GLuint VAOmilestone;


//triangles
vector<glm::vec3> posV3;
vector<glm::vec3> normV3;
vector<glm::vec3> binormV3;
vector<glm::vec3> tanV3;

std::vector<float> posTri;
std::vector<float> colorTri;
std::vector<float> normalTri;
GLuint VAOTri;
GLuint VBOTriPos;
GLuint VBOTriColor;
GLuint VBOTriNormal;

// represents one control point along the spline 
struct Point
{
	double x;
	double y;
	double z;
};

// spline struct 
// contains how many control points the spline has, and an array of control points 
struct Spline
{
	int numControlPoints;
	Point * points;
};

// the spline array 
Spline * splines;
// total number of splines 
int numSplines;

glm::mat4 CMRBasisM4x4;
glm::mat4x3 CMRControlM4x3;

glm::vec3 camPos;
glm::vec3 camCenter;
glm::vec3 camUp;
int index2 = 0;
int c = 0;

//sky
GLuint texHandleSky;
vector<float> posSky;
vector<float> uvSky;
GLuint VAOSky;
GLuint VBOSkyPos;
GLuint VBOSkyUV;
float ssize = 1024.0f;
int rep = 64;
int dis = 60;
const char* sfile = "redsky.jpg";
//texture ground
GLuint texHandle;
vector<float> posGround;
vector<float> uvGround;
texturePipelineProgram texturePipelineProg;
GLint t_modelViewMatrix, t_projectionMatrix;
GLuint t_program;
GLuint VAOGround;
GLuint VBOGroundPos;
GLuint VBOGroundUV;
float gsize = 40.96f;

const char* tfile = "water4k.jpg";

int camSpeed = 180;
vector<glm::vec3> railNormals;


//
//p(u) = [u^3 u^2 u 1] M C
//where M is the CatmullRom spline basis matrix 
//C is the control matrix
//parameter u varies on the interval[0, 1]
//You may use s = 1 / 2
//can brute force u by small value  0.001
//vector for complex geometry
// tangent at pi to
//s * (pi + 1 – pi - 1) 
//for i = 2, ..., n - 1, for some s(often s = 0.5)

void setTextureUnit(GLint unit)
{
	texturePipelineProg.Bind();
	glActiveTexture(unit); // select the active texture unit
	// get a handle to the “textureImage” shader variable
	GLint h_textureImage = glGetUniformLocation(t_program, "textureImage");
	// deem the shader variable “textureImage” to read from texture unit “unit”
	glUniform1i(h_textureImage, unit - GL_TEXTURE0);
}
void initGroundVAO()
{
	
	texturePipelineProg.Bind();

	glGenVertexArrays(1, &VAOGround);
	glBindVertexArray(VAOGround); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, VBOGroundPos);

	//position part of vbo into vao
	GLuint loc = glGetAttribLocation(t_program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	//uv part
	glBindBuffer(GL_ARRAY_BUFFER, VBOGroundUV);
	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(t_program, "texCoord");
	glEnableVertexAttribArray(loc); // enable the “uv” attribute
									// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 2, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO


}
void initSkyVAO()
{
	texturePipelineProg.Bind();

	glGenVertexArrays(1, &VAOSky);
	glBindVertexArray(VAOSky); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, VBOSkyPos);

	//position part of vbo into vao
	GLuint loc = glGetAttribLocation(t_program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	//uv part
	glBindBuffer(GL_ARRAY_BUFFER, VBOSkyUV);
	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(t_program, "texCoord");
	glEnableVertexAttribArray(loc); // enable the “uv” attribute
									// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 2, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO
}
void createSky()
{
	for (int i = 0; i < rep; i++)
	{
		for (int j = 0; j < rep; j++)
		{
			//using a 1024 x 1024 image making a quad from 
			//2 triangles
			//   v1   v2   uv:: 01  11
			//   v3   v4        00  10
			glm::vec3 v1 = { -gsize + i * gsize * 2,  gsize + j * gsize * 2, dis };//01
			glm::vec3 v2 = {  gsize + i * gsize * 2,  gsize + j * gsize * 2, dis };//11
			glm::vec3 v3 = { -gsize + i * gsize * 2, -gsize + j * gsize * 2, dis };//00
			glm::vec3 v4 = {  gsize + i * gsize * 2, -gsize + j * gsize * 2, dis };//10
			//421 134

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
			//2
			posSky.push_back(v2.x);
			posSky.push_back(v2.y);
			posSky.push_back(v2.z);
			uvSky.push_back(1);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//3
			posSky.push_back(v3.x);
			posSky.push_back(v3.y);
			posSky.push_back(v3.z);
			uvSky.push_back(0);
			uvSky.push_back(0);

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
		}
	}
	for (int i = 0; i < rep; i++)
	{
		for (int j = 0; j < rep; j++)
		{
			//using a 1024 x 1024 image making a quad from 
			//2 triangles
			//   v1   v2   uv:: 01  11
			//   v3   v4        00  10
			glm::vec3 v1 = {dis, -gsize + i * gsize * 2,  gsize + j * gsize * 2};//01
			glm::vec3 v2 = {dis, gsize + i * gsize * 2,  gsize + j * gsize * 2 };//11
			glm::vec3 v3 = {dis, -gsize + i * gsize * 2, -gsize + j * gsize * 2 };//00
			glm::vec3 v4 = {dis, gsize + i * gsize * 2, -gsize + j * gsize * 2 };//10
			//421 134

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
			//2
			posSky.push_back(v2.x);
			posSky.push_back(v2.y);
			posSky.push_back(v2.z);
			uvSky.push_back(1);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//3
			posSky.push_back(v3.x);
			posSky.push_back(v3.y);
			posSky.push_back(v3.z);
			uvSky.push_back(0);
			uvSky.push_back(0);

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
		}
	}
	for (int i = 0; i < rep; i++)
	{
		for (int j = 0; j < rep; j++)
		{
			//using a 1024 x 1024 image making a quad from 
			//2 triangles
			//   v1   v2   uv:: 01  11
			//   v3   v4        00  10
			glm::vec3 v1 = {-dis, -gsize + i * gsize * 2,  gsize + j * gsize * 2 };//01
			glm::vec3 v2 = {-dis, gsize + i * gsize * 2,  gsize + j * gsize * 2 };//11
			glm::vec3 v3 = {-dis, -gsize + i * gsize * 2, -gsize + j * gsize * 2 };//00
			glm::vec3 v4 = {-dis, gsize + i * gsize * 2, -gsize + j * gsize * 2 };//10
			//421 134

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
			//2
			posSky.push_back(v2.x);
			posSky.push_back(v2.y);
			posSky.push_back(v2.z);
			uvSky.push_back(1);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//3
			posSky.push_back(v3.x);
			posSky.push_back(v3.y);
			posSky.push_back(v3.z);
			uvSky.push_back(0);
			uvSky.push_back(0);

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
		}
	}
	for (int i = 0; i < rep; i++)
	{
		for (int j = 0; j < rep; j++)
		{
			//using a 1024 x 1024 image making a quad from 
			//2 triangles
			//   v1   v2   uv:: 01  11
			//   v3   v4        00  10
			glm::vec3 v1 = { -gsize + i * gsize * 2, dis,  gsize + j * gsize * 2 };//01
			glm::vec3 v2 = { gsize + i * gsize * 2, dis,  gsize + j * gsize * 2 };//11
			glm::vec3 v3 = { -gsize + i * gsize * 2, dis, -gsize + j * gsize * 2 };//00
			glm::vec3 v4 = { gsize + i * gsize * 2, dis, -gsize + j * gsize * 2 };//10
			//421 134

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
			//2
			posSky.push_back(v2.x);
			posSky.push_back(v2.y);
			posSky.push_back(v2.z);
			uvSky.push_back(1);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//3
			posSky.push_back(v3.x);
			posSky.push_back(v3.y);
			posSky.push_back(v3.z);
			uvSky.push_back(0);
			uvSky.push_back(0);

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
		}
	}
	for (int i = 0; i < rep; i++)
	{
		for (int j = 0; j < rep; j++)
		{
			//using a 1024 x 1024 image making a quad from 
			//2 triangles
			//   v1   v2   uv:: 01  11
			//   v3   v4        00  10
			glm::vec3 v1 = { -gsize + i * gsize * 2, -dis,  gsize + j * gsize * 2 };//01
			glm::vec3 v2 = { gsize + i * gsize * 2, -dis,  gsize + j * gsize * 2 };//11
			glm::vec3 v3 = { -gsize + i * gsize * 2, -dis, -gsize + j * gsize * 2 };//00
			glm::vec3 v4 = { gsize + i * gsize * 2, -dis, -gsize + j * gsize * 2 };//10
			//421 134

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
			//2
			posSky.push_back(v2.x);
			posSky.push_back(v2.y);
			posSky.push_back(v2.z);
			uvSky.push_back(1);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//1
			posSky.push_back(v1.x);
			posSky.push_back(v1.y);
			posSky.push_back(v1.z);
			uvSky.push_back(0);
			uvSky.push_back(1);
			//3
			posSky.push_back(v3.x);
			posSky.push_back(v3.y);
			posSky.push_back(v3.z);
			uvSky.push_back(0);
			uvSky.push_back(0);

			//4
			posSky.push_back(v4.x);
			posSky.push_back(v4.y);
			posSky.push_back(v4.z);
			uvSky.push_back(1);
			uvSky.push_back(0);
		}
	}
}
void createGround()
{
	for (int i =0; i<128;i++)
	{
		for (int j = 0; j < 128; j++) 
		{
			//using a 1024 x 1024 image making a quad from 
			//2 triangles
			//   v1   v2   uv:: 01  11
			//   v3   v4        00  10
			glm::vec3 v1 = { -gsize + i * gsize*2,gsize + j * gsize *2,-10 };//01
			glm::vec3 v2 = { gsize + i * gsize*2,gsize + j * gsize*2 ,-10 };//11
			glm::vec3 v3 = { -gsize + i * gsize*2,-gsize + j * gsize*2,-10 };//00
			glm::vec3 v4 = { gsize + i * gsize*2,-gsize + j * gsize*2,-10 };//10
			//421 134

			//4
			posGround.push_back(v4.x);
			posGround.push_back(v4.y);
			posGround.push_back(v4.z);
			uvGround.push_back(1);
			uvGround.push_back(0);
			//2
			posGround.push_back(v2.x);
			posGround.push_back(v2.y);
			posGround.push_back(v2.z);
			uvGround.push_back(1);
			uvGround.push_back(1);
			//1
			posGround.push_back(v1.x);
			posGround.push_back(v1.y);
			posGround.push_back(v1.z);
			uvGround.push_back(0);
			uvGround.push_back(1);
			//1
			posGround.push_back(v1.x);
			posGround.push_back(v1.y);
			posGround.push_back(v1.z);
			uvGround.push_back(0);
			uvGround.push_back(1);
			//3
			posGround.push_back(v3.x);
			posGround.push_back(v3.y);
			posGround.push_back(v3.z);
			uvGround.push_back(0);
			uvGround.push_back(0);

			//4
			posGround.push_back(v4.x);
			posGround.push_back(v4.y);
			posGround.push_back(v4.z);
			uvGround.push_back(1);
			uvGround.push_back(0);
		}
	}

}
//VAO from pos and color VBOs
void initVAO(GLuint &vao, GLuint &vboP, GLuint &vboC, GLuint &vboN)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, vboP);

	//position part of vbo into vao
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
	
	//color part
	glBindBuffer(GL_ARRAY_BUFFER, vboC);
	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
									// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO
	
	//normal part
	glBindBuffer(GL_ARRAY_BUFFER, vboN);
	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(program, "normal");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
									// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO


}
void initVAO(GLuint &vao, GLuint &vboP, GLuint &vboC)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, vboP);

	//position part of vbo into vao
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	//color part
	glBindBuffer(GL_ARRAY_BUFFER, vboC);
	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
									// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO


}
//vbo for pos and color from vector of pos and vector of color
void initVBO(GLuint &vboP, GLuint &vboC, GLuint &vboN, std::vector<GLfloat> &pos, std::vector<GLfloat> &color, std::vector<GLfloat> &normal)
{
	glGenBuffers(1, &vboP);
	glBindBuffer(GL_ARRAY_BUFFER, vboP);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*pos.size(),
		&pos[0], GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it

	glGenBuffers(1, &vboC);
	glBindBuffer(GL_ARRAY_BUFFER, vboC);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*color.size(),
		&color[0], GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it
	
	glGenBuffers(1, &vboN);
	glBindBuffer(GL_ARRAY_BUFFER, vboN);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*normal.size(),
		&color[0], GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it

}
void initVBO(GLuint &vboP, GLuint &vboC, std::vector<GLfloat> &pos, std::vector<GLfloat> &color)
{
	glGenBuffers(1, &vboP);
	glBindBuffer(GL_ARRAY_BUFFER, vboP);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*pos.size(),
		&pos[0], GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it

	glGenBuffers(1, &vboC);
	glBindBuffer(GL_ARRAY_BUFFER, vboC);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)*color.size(),
		&color[0], GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it

	
}

//cam values 
void addCamCords(glm::vec3 in)
{
	//starting set of axes at point P0, given T0. 
	//Pick an arbitrary vector V. 
	//N0 = unit(T0 x V) and 
	//B0 = unit(T0 x N0), 
	//where unit(u) is the normalization of vector u, or u/|u|.
	//You now have a starting set of coordinates.

	//Now you simply need a function to calculate a set of coordinate vectors given the previous set.
	//Given a previous set at P0 and the desired new set at P1, 
	//a good way to do this is to let N1 = unit(B0 x T1) and B1 = unit(T1 x N1).
	
	glm::vec3 t;
	glm::vec3 no;
	glm::vec3 bi;
	glm::vec3 v;
	if (c == 0)
	{
		t = glm::normalize(in);
		v.x = 0; v.y = 1; v.z = 0;
		no = glm::cross(t, v);
		no = glm::normalize(no);
		bi = glm::cross(t, no);
		bi = glm::normalize(bi);
	}
	else 
	{
		t = glm::normalize(in);
		no = glm::cross(binormV3[c - 1], t);
		no = glm::normalize(no);
		bi = glm::cross(t, no);
		bi = glm::normalize(bi);
	}
	tangent.push_back(t.x);
	tangent.push_back(t.y);
	tangent.push_back(t.z);
	normal.push_back(no.x);
	normal.push_back(no.y);
	normal.push_back(no.z);
	binormal.push_back(bi.x);
	binormal.push_back(bi.y);
	binormal.push_back(bi.z);
	c++;
	normV3.push_back(no);
	binormV3.push_back(bi);
	tanV3.push_back(t);
}
//pos x y z, color r g b a used for spline line render
void addVert(glm::vec3 in)
{
	pos.push_back(in.x);
	pos.push_back(in.y);
	pos.push_back(in.z);
	//color temp for milestone
	color.push_back(1.0f);//r
	color.push_back(0.0f);//g
	color.push_back(1.0f);//b
	color.push_back(1.0f);//a
}
//vector4 * matrix4x4 * matrix 4x3
glm::vec3 createCMRVec3(float u, glm::mat4x3 controlM4x3)
{
	//p(u) = [u^3 u^2 u 1] M C,  
	glm::vec4 CMRuV4 = glm::vec4(pow(u, 3), pow(u, 2), u, 1.0);
	//glm == column matrix, v4*m4x4*m4x3
	//need reverse order 
	//M * u
	//glm::vec4 tempU = (CMRBasisM4x4*CMRuV4);
	// C * (M * u)
	//glm::vec3 r = controlM4x3 * tempU;
	//return r;
	
	//(C*M)*u
	return ((controlM4x3*CMRBasisM4x4)*CMRuV4);
}
glm::vec3 createCMRTanVec3(float u, glm::mat4x3 controlM4x3)
{
	//similar to createCMRVec3
	//t(u) = p'(u) = [3u^2 2u 1 0] M C
	glm::vec4 CMRuTanV4 = glm::vec4(3 * pow(u, 2), 2 * u, 1.0, 0.0);
	glm::normalize(CMRuTanV4);
	//need reverse order 
	//M * u
	//glm::vec4 tempTan = (CMRBasisM4x4*CMRuTanV4);
	// C * (M * u)
	//glm::vec3 temp = controlM4x3 * tempTan;
	//glm::normalize(temp);
	
	//return temp;
	
	//(C*M)*u
	return (glm::normalize((controlM4x3*CMRBasisM4x4)*CMRuTanV4));
}
//recursive line division, fills pos vector 
void subdivide(float u0, float u1, float maxLineLength ,glm::mat4x3 controlM4x3)
{
	/*
	Subdivide(u0,u1,maxlinelength)
	umid = (u0 + u1)/2
	x0 = F(u0)
	x1 = F(u1)
	if |x1 - x0| > maxlinelength
	Subdivide(u0,umid,maxlinelength)
	Subdivide(umid,u1,maxlinelength)
	else drawline(x0,x1)
	*/
	float umid = (u0 + u1) / 2;
	glm::vec3 x0 = createCMRVec3(u0, controlM4x3);
	glm::vec3 x1 = createCMRVec3(u1, controlM4x3);
	
	glm::vec3 abs = glm::abs(x1 - x0);
	if (glm::length(abs) > maxLineLength)
	{
		subdivide(u0, umid, maxLineLength, controlM4x3);
		subdivide(umid, u1, maxLineLength, controlM4x3);
	}
	else
	{
		glm::vec3 tan0 = createCMRTanVec3(u0, controlM4x3);
		glm::vec3 tan1 = createCMRTanVec3(u1, controlM4x3);

		addVert(x0);
		posV3.push_back(x0);
		addVert(x1);
		posV3.push_back(x1);

		addCamCords(tan0);
		addCamCords(tan1);
	}
}
//control matrix m4x3 for CMR spline, calculate per point
void createCMRControl()
{
	for (int i = 1; i < splines->numControlPoints - 2; i++)
	{
		//new m4x3 control matrix for each control point
		//curve between piand pi + 1 is completely determined by pi - 1, pi, pi + 1, pi + 2 .
		CMRControlM4x3 = glm::mat4x3(1.0);
		CMRControlM4x3[0] = 
			glm::vec3(splines->points[i - 1].x, 
					  splines->points[i - 1].y,
					  splines->points[i - 1].z);
		CMRControlM4x3[1] = 
			glm::vec3(splines->points[i].x,
				      splines->points[i].y,
				      splines->points[i].z);
		CMRControlM4x3[2] = 
			glm::vec3(splines->points[i + 1].x,
					  splines->points[i + 1].y, 
					  splines->points[i + 1].z);
		CMRControlM4x3[3] = 
			glm::vec3(splines->points[i + 2].x,
				      splines->points[i + 2].y,
					  splines->points[i + 2].z);

		subdivide(0.0f, 1.0f, 0.001, CMRControlM4x3);
	}
}
//basis m4x4 for CMR spline, calculate once
void createCMRBasis()
{
	float s = 0.5f;
	CMRBasisM4x4 = glm::mat4(1.0);
	CMRBasisM4x4[0] = glm::vec4(-s, 2 - s, s - 2, s);
	CMRBasisM4x4[1] = glm::vec4(2 * s, s - 3, 3 - 2 * s, -s);
	CMRBasisM4x4[2] = glm::vec4(-s, 0, s, 0);
	CMRBasisM4x4[3] = glm::vec4(0, 1, 0, 0);
}
//send verticies for rails
void makeRails()
{
	// (r,g,b) color of each vertex to the normal of the triangle.
	//That is, if the normal is n=(nx, ny, nz), set r = nx, g = ny, b = nz
	// make a quad for rail

	//quads from 2 triangles == 4 points 
	//v0 v1 v2 v3 , v4 v5 v6 v7
	glm::vec3 p0;
	glm::vec3 p1;
	glm::vec3 n0;
	glm::vec3 n1;
	glm::vec3 b0;
	glm::vec3 b1;

	glm::vec3 v0;
	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 v3;
	glm::vec3 v4;
	glm::vec3 v5;
	glm::vec3 v6;
	glm::vec3 v7;
	glm::vec3 faceNormal0, faceNormal1, faceNormal2, faceNormal3;
	glm::vec3 faceNormal4, faceNormal5, faceNormal6, faceNormal7;

	int p = posV3.size();
	float b = 0.15f;
	float a = 0.05f;
	for (int i = 0; i < p - 1; i++)
	{
		{
			p0 = posV3[i];
			p1 = posV3[i + 1];
			n0 = normV3[i];
			n1 = normV3[i + 1];
			b0 = binormV3[i];
			b1 = binormV3[i + 1];

			
		}

		//need 4 faces off the 8 vertices
		//each face is a set of 6 vertices
		//
		//first 4
		//v2  v1        v2------v1
		//v3  v0		|		|	
		//next 4		v3------v0
		//v6  v5
		//v7  v4			v6------v5
		//					|		|
		//					v7------v4

		{

			v0 = (-n0 + b0);
			v0.x *= a;
			v0.y *= a;
			v0.z *= a;
			v0 += p0;

			v1 = (n0 + b0);
			v1.x *= a;
			v1.y *= a;
			v1.z *= a;
			v1 += p0;

			v2 = (n0 - b0);
			v2.x *= a;
			v2.y *= a;
			v2.z *= a;
			v2 += p0;

			v3 = (-n0 - b0);
			v3.x *= a;
			v3.y *= a;
			v3.z *= a;
			v3 += p0;

			//next 4
			//v6  v5
			//v7  v4
			v4 = (-n1 + b1);
			v4.x *= a;
			v4.y *= a;
			v4.z *= a;
			v4 += p1;

			v5 = (n1 + b1);
			v5.x *= a;
			v5.y *= a;
			v5.z *= a;
			v5 += p1;

			v6 = (n1 - b1);
			v6.x *= a;
			v6.y *= a;
			v6.z *= a;
			v6 += p1;

			v7 = (-n1 - b1);
			v7.x *= a;
			v7.y *= a;
			v7.z *= a;
			v7 += p1;
		}
		
		{
			//normals of each triangle face for phong
			//014 ,415 125, 526  763, 362 734, 430
			//n0 = (p1 – p0) x(p2 – p0)

			//1-0 4-0
			faceNormal0 = glm::normalize(glm::cross((v1-v0),(v4-v0)));
			//1-4 5-4
			faceNormal1 = glm::normalize(glm::cross((v1 - v4), (v5 - v4)));
			//2-1 5-1
			faceNormal2 = glm::normalize(glm::cross((v2 - v1), (v5 - v1)));
			//526  2-5 6-5
			faceNormal3 = glm::normalize(glm::cross((v2 - v5), (v6 - v5)));
			//763  6-7 3-7
			faceNormal4 = glm::normalize(glm::cross((v6 - v7), (v3 - v7)));
			//362 6-3 2-3
			faceNormal5 = glm::normalize(glm::cross((v6 - v3), (v2 - v3)));
			//734 3-7 4-7
			faceNormal6 = glm::normalize(glm::cross((v3 - v7), (v4 - v7)));
			//430 3-4 0-4
			faceNormal7 = glm::normalize(glm::cross((v3 - v4), (v0 - v4)));
			
			//rail 1 semi cheated no binormal offsets
			railNormals.push_back(faceNormal0);
			railNormals.push_back(faceNormal1);
			railNormals.push_back(faceNormal2);
			railNormals.push_back(faceNormal3);
			railNormals.push_back(faceNormal4);
			railNormals.push_back(faceNormal5);
			railNormals.push_back(faceNormal6);
			railNormals.push_back(faceNormal7);
			//rail2
			railNormals.push_back(faceNormal0);
			railNormals.push_back(faceNormal1);
			railNormals.push_back(faceNormal2);
			railNormals.push_back(faceNormal3);
			railNormals.push_back(faceNormal4);
			railNormals.push_back(faceNormal5);
			railNormals.push_back(faceNormal6);
			railNormals.push_back(faceNormal7);


		}
		{
			//rail 1
		
			//014 ,415
			
			//0
			posTri.push_back(v0.x + b0.x*b);
			posTri.push_back(v0.y + b0.y*b);
			posTri.push_back(v0.z + b0.z*b);

			//1
			posTri.push_back(v1.x + b0.x*b);
			posTri.push_back(v1.y + b0.y*b);
			posTri.push_back(v1.z + b0.z*b);

			//4
			posTri.push_back(v4.x + b1.x*b);
			posTri.push_back(v4.y + b1.y*b);
			posTri.push_back(v4.z + b1.z*b);

			//4
			posTri.push_back(v4.x + b1.x*b);
			posTri.push_back(v4.y + b1.y*b);
			posTri.push_back(v4.z + b1.z*b);

			//1
			posTri.push_back(v1.x + b0.x*b);
			posTri.push_back(v1.y + b0.y*b);
			posTri.push_back(v1.z + b0.z*b);

			//5
			posTri.push_back(v5.x + b1.x*b);
			posTri.push_back(v5.y + b1.y*b);
			posTri.push_back(v5.z + b1.z*b);

			//125, 526
			//1
			posTri.push_back(v1.x + b0.x*b);
			posTri.push_back(v1.y + b0.y*b);
			posTri.push_back(v1.z + b0.z*b);

			//2
			posTri.push_back(v2.x + b0.x*b);
			posTri.push_back(v2.y + b0.y*b);
			posTri.push_back(v2.z + b0.z*b);

			//5
			posTri.push_back(v5.x + b1.x*b);
			posTri.push_back(v5.y + b1.y*b);
			posTri.push_back(v5.z + b1.z*b);

			//5
			posTri.push_back(v5.x + b1.x*b);
			posTri.push_back(v5.y + b1.y*b);
			posTri.push_back(v5.z + b1.z*b);

			//2
			posTri.push_back(v2.x + b0.x*b);
			posTri.push_back(v2.y + b0.y*b);
			posTri.push_back(v2.z + b0.z*b);

			//6
			posTri.push_back(v6.x + b1.x*b);
			posTri.push_back(v6.y + b1.y*b);
			posTri.push_back(v6.z + b1.z*b);

			//763, 362
			//7
			posTri.push_back(v7.x + b1.x*b);
			posTri.push_back(v7.y + b1.y*b);
			posTri.push_back(v7.z + b1.z*b);

			//6
			posTri.push_back(v6.x + b1.x*b);
			posTri.push_back(v6.y + b1.y*b);
			posTri.push_back(v6.z + b1.z*b);

			//3
			posTri.push_back(v3.x + b0.x*b);
			posTri.push_back(v3.y + b0.y*b);
			posTri.push_back(v3.z + b0.z*b);

			//3
			posTri.push_back(v3.x + b0.x*b);
			posTri.push_back(v3.y + b0.y*b);
			posTri.push_back(v3.z + b0.z*b);

			//6
			posTri.push_back(v6.x + b1.x*b);
			posTri.push_back(v6.y + b1.y*b);
			posTri.push_back(v6.z + b1.z*b);

			//2
			posTri.push_back(v2.x + b0.x*b);
			posTri.push_back(v2.y + b0.y*b);
			posTri.push_back(v2.z + b0.z*b);

			//734, 430
			//7
			posTri.push_back(v7.x + b1.x*b);
			posTri.push_back(v7.y + b1.y*b);
			posTri.push_back(v7.z + b1.z*b);

			//3
			posTri.push_back(v3.x + b0.x*b);
			posTri.push_back(v3.y + b0.y*b);
			posTri.push_back(v3.z + b0.z*b);

			//4
			posTri.push_back(v4.x + b1.x*b);
			posTri.push_back(v4.y + b1.y*b);
			posTri.push_back(v4.z + b1.z*b);

			//4
			posTri.push_back(v4.x + b1.x*b);
			posTri.push_back(v4.y + b1.y*b);
			posTri.push_back(v4.z + b1.z*b);

			//3
			posTri.push_back(v3.x + b0.x*b);
			posTri.push_back(v3.y + b0.y*b);
			posTri.push_back(v3.z + b0.z*b);

			//0
			posTri.push_back(v0.x + b0.x*b);
			posTri.push_back(v0.y + b0.y*b);
			posTri.push_back(v0.z + b0.z*b);

		}
		{
			{
				//2nd rail
				//014 ,415
				//0
				posTri.push_back(v0.x - b0.x*b);
				posTri.push_back(v0.y - b0.y*b);
				posTri.push_back(v0.z - b0.z*b);
				//1
				posTri.push_back(v1.x - b0.x*b);
				posTri.push_back(v1.y - b0.y*b);
				posTri.push_back(v1.z - b0.z*b);
				//4
				posTri.push_back(v4.x - b1.x*b);
				posTri.push_back(v4.y - b1.y*b);
				posTri.push_back(v4.z - b1.z*b);

				//4
				posTri.push_back(v4.x - b1.x*b);
				posTri.push_back(v4.y - b1.y*b);
				posTri.push_back(v4.z - b1.z*b);

				//1
				posTri.push_back(v1.x - b0.x*b);
				posTri.push_back(v1.y - b0.y*b);
				posTri.push_back(v1.z - b0.z*b);

				//5
				posTri.push_back(v5.x - b1.x*b);
				posTri.push_back(v5.y - b1.y*b);
				posTri.push_back(v5.z - b1.z*b);

				//125, 526
				//1
				posTri.push_back(v1.x - b0.x*b);
				posTri.push_back(v1.y - b0.y*b);
				posTri.push_back(v1.z - b0.z*b);

				//2
				posTri.push_back(v2.x - b0.x*b);
				posTri.push_back(v2.y - b0.y*b);
				posTri.push_back(v2.z - b0.z*b);

				//5
				posTri.push_back(v5.x - b1.x*b);
				posTri.push_back(v5.y - b1.y*b);
				posTri.push_back(v5.z - b1.z*b);

				//5
				posTri.push_back(v5.x - b1.x*b);
				posTri.push_back(v5.y - b1.y*b);
				posTri.push_back(v5.z - b1.z*b);

				//2
				posTri.push_back(v2.x - b0.x*b);
				posTri.push_back(v2.y - b0.y*b);
				posTri.push_back(v2.z - b0.z*b);

				//6
				posTri.push_back(v6.x - b1.x*b);
				posTri.push_back(v6.y - b1.y*b);
				posTri.push_back(v6.z - b1.z*b);

				//763, 362
				//7
				posTri.push_back(v7.x - b1.x*b);
				posTri.push_back(v7.y - b1.y*b);
				posTri.push_back(v7.z - b1.z*b);

				//6
				posTri.push_back(v6.x - b1.x*b);
				posTri.push_back(v6.y - b1.y*b);
				posTri.push_back(v6.z - b1.z*b);

				//3
				posTri.push_back(v3.x - b0.x*b);
				posTri.push_back(v3.y - b0.y*b);
				posTri.push_back(v3.z - b0.z*b);

				//3
				posTri.push_back(v3.x - b0.x*b);
				posTri.push_back(v3.y - b0.y*b);
				posTri.push_back(v3.z - b0.z*b);

				//6
				posTri.push_back(v6.x - b1.x*b);
				posTri.push_back(v6.y - b1.y*b);
				posTri.push_back(v6.z - b1.z*b);

				//2
				posTri.push_back(v2.x - b0.x*b);
				posTri.push_back(v2.y - b0.y*b);
				posTri.push_back(v2.z - b0.z*b);

				//734, 430
				//7
				posTri.push_back(v7.x - b1.x*b);
				posTri.push_back(v7.y - b1.y*b);
				posTri.push_back(v7.z - b1.z*b);

				//3
				posTri.push_back(v3.x - b0.x*b);
				posTri.push_back(v3.y - b0.y*b);
				posTri.push_back(v3.z - b0.z*b);

				//4
				posTri.push_back(v4.x - b1.x*b);
				posTri.push_back(v4.y - b1.y*b);
				posTri.push_back(v4.z - b1.z*b);

				//4
				posTri.push_back(v4.x - b1.x*b);
				posTri.push_back(v4.y - b1.y*b);
				posTri.push_back(v4.z - b1.z*b);

				//3
				posTri.push_back(v3.x - b0.x*b);
				posTri.push_back(v3.y - b0.y*b);
				posTri.push_back(v3.z - b0.z*b);

				//0
				posTri.push_back(v0.x - b0.x*b);
				posTri.push_back(v0.y - b0.y*b);
				posTri.push_back(v0.z - b0.z*b);


			}
		}
		{
			{
			
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);

				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n1.x);
				colorTri.push_back(n1.y);
				colorTri.push_back(n1.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
				colorTri.push_back(n0.x);
				colorTri.push_back(n0.y);
				colorTri.push_back(n0.z);
				colorTri.push_back(1);
			
			}
		}
		
	}
	for (int t = 0; t < railNormals.size(); t++)
	{
		normalTri.push_back(railNormals[t].x);
		normalTri.push_back(railNormals[t].y);
		normalTri.push_back(railNormals[t].z);

	}
}
void doRide()
{
	
	GLint first = 0;
	{
		GLsizei numTri = posTri.size() / 3;//num tri point x y z
		glBindVertexArray(VAOTri);
		glDrawArrays(GL_TRIANGLES, first, numTri);
		glBindVertexArray(0); // unbind the VAO
	}
}
void doGround()
{
	
	setTextureUnit(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	GLint first = 0;
	{
		GLsizei num = posGround.size() / 3;//num/ point x y z
		glBindVertexArray(VAOGround);
		glDrawArrays(GL_TRIANGLES, first, num);
		glBindVertexArray(0); // unbind the VAO
	}
}
void doSky()
{

	setTextureUnit(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texHandleSky);
	GLint first = 0;
	{
		GLsizei num = posSky.size() / 3;//num/ point x y z
		glBindVertexArray(VAOSky);
		glDrawArrays(GL_TRIANGLES, first, num);
		glBindVertexArray(0); // unbind the VAO
	}
}

glm::vec4 calcView(float view[16])
{
	//0  1  2  3       0
	//4  5  6  7   \/  1
	//8  9  10 11  /\  0
	//12 13 14 15      0
	return(glm::vec4{ view[1],view[5],view[9],view[13] });
}
void doViews()
{	

	float p[16]; // column-major
	openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	openGLMatrix.GetMatrix(p);
	const GLboolean isRowMajor = GL_FALSE;

	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.LoadIdentity();

	//camera
	//size of vector holding the points
	int tsize = tangent.size();
	if (index2 >= tsize)
	{
		index2 = 0;
	}
	//x
	camPos.x = pos[index2] + normal[index2] * 0.30;
	camCenter.x = camPos.x + tangent[index2];
	camUp.x = normal[index2];
	index2++;
	//y
	camPos.y = pos[index2] + normal[index2] * 0.30;
	camCenter.y = camPos.y + tangent[index2];
	camUp.y = normal[index2];
	index2++;
	//z
	camPos.z = pos[index2] + normal[index2] * 0.30;
	camCenter.z = camPos.z + tangent[index2];
	camUp.z = normal[index2];
	index2++;
	index2 += camSpeed;

	openGLMatrix.LookAt(camPos.x, camPos.y, camPos.z, camCenter.x, camCenter.y, camCenter.z, camUp.x, camUp.y, camUp.z);

	//translate, rotate, scale. rotate x y z
	openGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
	openGLMatrix.Rotate(landRotate[0], 1, 0, 0);//w
	openGLMatrix.Rotate(landRotate[1], 0, 1, 0);//v
	openGLMatrix.Rotate(landRotate[2], 0, 0, 1);//n
	openGLMatrix.Scale(landScale[0], landScale[1], landScale[2]);
	
	float m[16]; // column-major
	openGLMatrix.GetMatrix(m);
	
	//send to shader
	//ground
	texturePipelineProg.Bind();
	t_projectionMatrix =
		glGetUniformLocation(t_program, "projectionMatrix");
	t_modelViewMatrix =
		glGetUniformLocation(t_program, "modelViewMatrix");
	glUniformMatrix4fv(t_projectionMatrix, 1, isRowMajor, p);
	glUniformMatrix4fv(t_modelViewMatrix, 1, isRowMajor, m);
	doGround();
	doSky();
	//ride
	pipelineProgram.Bind();
	
	//
	float view[16];
	openGLMatrix.GetMatrix(view);
	GLint h_viewLightDirection =
		glGetUniformLocation(program, "viewLightDirection");	
	//float lightDirection[3] = { 0, 1, 0 }; // the “Sun” at noon
	//below is a hard coded result of viewmat * 0,0,1,0 vector
	float viewLightDirection[3] = {view[2],view[6],view[10]}; // light direction in the view space
	//viewLightDirection = (view * float4(lightDirection, 0.0)).xyz;
	// upload viewLightDirection to the GPU
	glUniform3fv(h_viewLightDirection, 1, viewLightDirection);
	//
	GLint h_normalMatrix =
		glGetUniformLocation(program, "normalMatrix");
	float n[16];
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.GetNormalMatrix(n); // get normal matrix
	// upload n to the GPU
	glUniformMatrix4fv(h_normalMatrix, 1, isRowMajor, n);
	//
	
	h_projectionMatrix =
		glGetUniformLocation(program, "projectionMatrix");
	h_modelViewMatrix =
		glGetUniformLocation(program, "modelViewMatrix");

	glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
	doRide();


	
}
void displayFunc()
{
	//clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	doViews();
	
	glutSwapBuffers();
}

int loadSplines(char * argv)
{
	char * cName = (char *)malloc(128 * sizeof(char));
	FILE * fileList;
	FILE * fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL)
	{
		printf("can't open file\n");
		exit(1);
	}

	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*)malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++)
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL)
		{
			printf("can't open file\n");
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point *)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf",
			&splines[j].points[i].x,
			&splines[j].points[i].y,
			&splines[j].points[i].z) != EOF)
		{
			i++;
		}
	}

	free(cName);

	return 0;
}
int initTexture(const char * imageFilename, GLuint textureHandle)
{
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK)
	{
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4)
	{
		printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

																		// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++)
		{
			// assign some default byte values (for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

													   // set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0)
	{
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}

	// de-allocate the pixel array -- it is no longer needed
	delete[] pixelsRGBA;

	return 0;
}
void saveScreenshot(const char * filename)
{
	unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
		cout << "File " << filename << " saved successfully." << endl;
	else cout << "Failed to save file " << filename << '.' << endl;

	delete[] screenshotData;
}
int shots = 0;
void idleFunc()
{
	// do some stuff... 

	// for example, here, you can save the screenshots to disk (to make the animation)
	if (anim && n %5==0)
	{
		string s = "SS/screenshot";
		string ss = s + std::to_string(shots) + ".jpg";
		char* c = new char[ss.length() + 1];
		strcpy(c, ss.c_str());
		saveScreenshot(c);
		shots++;
	}
	if (shots >= 750)
	{
		anim = false;
		n = 0;
		shots = 0;
	}
	n++;
	// make the screen update keep at end
	glutPostRedisplay();
}
void mouseMotionDragFunc(int x, int y)
{
	// mouse has moved and one of the mouse buttons is pressed (dragging)

	// the change in mouse position since the last invocation of this function
	int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

	switch (controlState)
	{
		// translate the landscape
	case TRANSLATE:
		if (leftMouseButton)
		{
			// control x,y translation via the left mouse button
			landTranslate[0] += mousePosDelta[0] * 0.01f;
			landTranslate[1] -= mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z translation via the middle mouse button
			landTranslate[2] += mousePosDelta[1] * 0.01f;
		}
		break;

		// rotate the landscape
	case ROTATE:
		if (leftMouseButton)
		{
			// control x,y rotation via the left mouse button
			landRotate[0] += mousePosDelta[1];
			landRotate[1] += mousePosDelta[0];
		}
		if (middleMouseButton)
		{
			// control z rotation via the middle mouse button
			landRotate[2] += mousePosDelta[1];
		}
		break;

		// scale the landscape
	case SCALE:
		if (leftMouseButton)
		{
			// control x,y scaling via the left mouse button
			landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
			landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		if (middleMouseButton)
		{
			// control z scaling via the middle mouse button
			landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
		}
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}
void mouseMotionFunc(int x, int y)
{
	// mouse has moved
	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}
void mouseButtonFunc(int button, int state, int x, int y)
{
	// a mouse button has has been pressed or depressed

	// keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		leftMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_MIDDLE_BUTTON:
		middleMouseButton = (state == GLUT_DOWN);
		break;

	case GLUT_RIGHT_BUTTON:
		rightMouseButton = (state == GLUT_DOWN);
		break;
	}

	// keep track of whether CTRL and SHIFT keys are pressed
	switch (glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		controlState = TRANSLATE;
		break;

	case GLUT_ACTIVE_SHIFT:
		controlState = SCALE;
		break;

		// if CTRL and SHIFT are not pressed, we are in rotate mode
	default:
		controlState = ROTATE;
		break;
	}

	// store the new mouse position
	mousePos[0] = x;
	mousePos[1] = y;
}
void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 't':
		//cycle modes
		//dPoints = false;
		//dLines = false;
		//dTri = true;
		break;
	case 'l':
		//cycle modes
		//dPoints = false;
		//dTri = false;
		//dLines = true;
		break;
	case 'p':
		//cycle modes
		//dTri = false;
		//dLines = false;
		//dPoints = true;
		break;

	case 27: // ESC key
		exit(0); // exit the program
		break;

	case ' ':
		cout << "You pressed the spacebar." << endl;
		break;

	case 'a':
		anim = true;
		break;

	case 'x':
		// take a screenshot

		saveScreenshot("screenshot.jpg");

		break;


	}
}

void initScene()
{
	//set background to black, set depth test
	glClearColor(255.0f, 255.0f, 255.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	pipelineProgram.Init("../openGLHelper-starterCode");
	//pipelineProgram.Bind();
	program = pipelineProgram.GetProgramHandle();
	// get a handle to the projectionMatrix shader variable
	texturePipelineProg.Init("../openGLHelper-starterCode");
	//texturePipelineProg.Bind();
	t_program = texturePipelineProg.GetProgramHandle();
	// get a handle to the projectionMatrix shader variable

	glGenTextures(1, &texHandle);
	int code = initTexture(tfile,texHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
	
	glGenTextures(1, &texHandleSky);
	int code2 = initTexture(sfile, texHandleSky);
	if (code2 != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}
}
void reshapeFunc(int x, int y)
{
	glViewport(0, 0, x, y);
	openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	openGLMatrix.LoadIdentity();
	openGLMatrix.Perspective(45.0, 1.0 * x / y, 0.01, 2500.0);
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
}
int main(int argc, char ** argv)
{

	{
		if (argc < 2)
		{
			printf("usage: %s <trackfile>\n", argv[0]);
			exit(0);
		}


		cout << "Initializing GLUT..." << endl;
		glutInit(&argc, argv);

		cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
		glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

		glutInitWindowSize(windowWidth, windowHeight);
		glutInitWindowPosition(0, 0);
		glutCreateWindow(windowTitle);

		cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
		cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
		cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

		// tells glut to use a particular display function to redraw 
		glutDisplayFunc(displayFunc);
		// perform animation inside idleFunc
		glutIdleFunc(idleFunc);
		// callback for mouse drags
		glutMotionFunc(mouseMotionDragFunc);
		// callback for idle mouse movement
		glutPassiveMotionFunc(mouseMotionFunc);
		// callback for mouse button changes
		glutMouseFunc(mouseButtonFunc);
		// callback for resizing the window
		glutReshapeFunc(reshapeFunc);
		// callback for pressing the keys on the keyboard
		glutKeyboardFunc(keyboardFunc);

		// init glew
#ifdef __APPLE__
	// nothing is needed on Apple
#else
	// Windows, Linux
		GLint result = glewInit();
		if (result != GLEW_OK)
		{
			cout << "error: " << glewGetErrorString(result) << endl;
			exit(EXIT_FAILURE);
		}
#endif
}
	// do initialization
	initScene();
	// load the splines from the provided filename
	
	loadSplines(argv[1]);

	//create CMR Basis
	createCMRBasis();
	//create CMR Control and Fill pos vector
	createCMRControl();
	
	//just spline line
	//create vbo
	//initVBO(VBOpos,VBOcolor,pos,color);
	//create vao
	//initVAO(VAOmilestone, VBOpos, VBOcolor);
	
	//double rails 
	makeRails();
	//triangles for double rails
	//pipelineProgram.Bind();
	initVBO(VBOTriPos,VBOTriColor, VBOTriNormal, posTri,colorTri, normalTri);
	initVAO(VAOTri, VBOTriPos,VBOTriColor, VBOTriNormal);
	
	//texturePipelineProg.Bind();
	createGround();
	initVBO(VBOGroundPos, VBOGroundUV, posGround, uvGround);
	initGroundVAO();

	createSky();
	initVBO(VBOSkyPos, VBOSkyUV, posSky, uvSky);
	initSkyVAO();
	// sink forever into the glut loop
	glutMainLoop();
	
}