/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields
  C++ starter code

  Student username: Charlied 
  milestone 1 code left for my referance 
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

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
char windowTitle[512] = "CSCI 420 homework I";

ImageIO * heightmapImage;

//data from slides

float zStudent= 3+(2948071714/10000000000);

OpenGLMatrix openGLMatrix;

BasicPipelineProgram pipelineProgram;
GLint h_modelViewMatrix, h_projectionMatrix;
GLuint program;
// milestone triangle //
GLuint vbo;
GLuint vao;
//GLuint buffer;
float positions[] = { 0, 0, -1,
1, 0, -1,
0, 1, -1 };
float colors[] = { 1, 0, 0, 1,
0, 1, 0, 1,
0, 0, 1, 1 };
// end milestone triangle //
//points
GLsizei numV;
GLfloat *pos;
GLfloat *color;
GLuint VAOPoints;
GLuint VBOPoints;
//lines
std::vector<GLfloat> posLine;
std::vector<GLfloat> colorLine;
GLuint VAOLines;
GLuint VBOLinesPos;
GLuint VBOLinesColor;

//triangles
std::vector<GLfloat> posTri;
std::vector<GLfloat> colorTri;
GLuint VAOTri;
GLuint VBOTriPos;
GLuint VBOTriColor;
int n=0;
bool anim = false;
int H, W;
bool dPoints = false;
bool dLines = false;
bool dTri = true;
//used for milestone triangle
void initVBO()
{
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(colors),
		nullptr, GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it
								  // upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), positions);
	// upload color data
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(colors), colors);
}
//model off triangle for image VBO for points
void initVBO(GLuint &vbo, int numPos, int numCol)
{
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*numPos + sizeof(GLfloat)*numCol,
		nullptr, GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it
								  // upload position data
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*numPos, pos);
	// upload color data
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat)*numPos, sizeof(GLfloat)*numCol, color);
}
//for lines and triangles
void initVBO(GLuint &vboP, GLuint &vboC, std::vector<GLfloat> &pos, std::vector<GLfloat> &color)
{
	glGenBuffers(1, &vboP);
	glBindBuffer(GL_ARRAY_BUFFER, vboP);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*pos.size(),
		&pos[0], GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it

	glGenBuffers(1, &vboC);
	glBindBuffer(GL_ARRAY_BUFFER, vboC);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*color.size(),
		&color[0], GL_STATIC_DRAW); // init VBO’s size, but don’t assign any data to it

}
//used for triangle
void initVAO() 
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // bind the VAO
							// bind the VBO “buffer” (must be previously created)
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// get location index of the “position” shader variable
	//position part of vbo into vao
	GLuint loc = glGetAttribLocation(program, "position");//"position" is the shader name
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
	//color part
	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
	//color comes after positions so offset by positions
	offset = (const void*) sizeof(positions);
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO
}
//model off triangle for image VAO points
void initVAO(GLuint &vao, GLuint &vbo, int numPos)
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao); // bind the VAO
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	// get location index of the “position” shader variable
	//position part of vbo into vao
	GLuint loc = glGetAttribLocation(program, "position");
	glEnableVertexAttribArray(loc); // enable the “position” attribute
	const void * offset = (const void*)0;
	GLsizei stride = 0;
	GLboolean normalized = GL_FALSE;
	// set the layout of the “position” attribute data
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
	//color part
	// get the location index of the “color” shader variable
	loc = glGetAttribLocation(program, "color");
	glEnableVertexAttribArray(loc); // enable the “color” attribute
	int posOffset = sizeof(GLfloat)*numPos;
	offset = (const void*) posOffset;
	stride = 0;
	normalized = GL_FALSE;
	// set the layout of the “color” attribute data
	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
	glBindVertexArray(0); // unbind the VAO

}
//for triangles and lines
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
void displayFunc()
{
	//clear
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	//start projection/////////////////////////////////////////
	float p[16]; // column-major
	openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	openGLMatrix.GetMatrix(p);
	const GLboolean isRowMajor = GL_FALSE;
	glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);
	//end projection//////////////////////////////////////////

	//start set model view matrix///////////////////////////////////////

	//Set mode then identity
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
	openGLMatrix.LoadIdentity();
	//camera
	//openGLMatrix.LookAt(0, 0, zStudent, 1, 0, 0, 0, 1, 0); // core profile
	openGLMatrix.LookAt(W - 100, W + 100, W+50, 120, 100, 0,  0, 1, 0); // core profile

	//translate, rotate, scale. rotate x y z
	openGLMatrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
	openGLMatrix.Rotate(landRotate[0], 1, 0, 0);//w
	openGLMatrix.Rotate(landRotate[1], 0, 1, 0);//v
	openGLMatrix.Rotate(landRotate[2], 0, 0, 1);//n
	openGLMatrix.Scale(landScale[0], landScale[1], landScale[2]);
	//Upload m to the GPU :
	//only one shader pipeline and it was bound in initscene
	//pipelineProgram->Bind();
	float m[16]; // column-major
	openGLMatrix.GetMatrix(m);
	//send to shader
	glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
	//end set model view matrixs///////////////////////////////

	GLint first = 0;
	GLsizei count = 3;

	//miles stone 1 left for referance 
	//bind vao and draw for milestone triangle
	//glBindVertexArray(vao); // bind the VAO
	//glDrawArrays(GL_TRIANGLES, first, count);
	//glBindVertexArray(0); // unbind the VAO
	
	//triangles
	if (dTri)
	{
		GLsizei numTri = posTri.size() / 3;//num tri point x y z
		glBindVertexArray(VAOTri);
		glDrawArrays(GL_TRIANGLES, first, numTri);
		glBindVertexArray(0); // unbind the VAO
	}
	//lines
	if (dLines)
	{
		GLsizei numLine = posLine.size() / 3 ;//num lines point x y z
		glBindVertexArray(VAOLines);
		glDrawArrays(GL_LINES, first, numLine);
		glBindVertexArray(0); // unbind the VAO
	}
	//points
	if (dPoints)
	{
		glBindVertexArray(VAOPoints);
		glDrawArrays(GL_POINTS, first, numV);
		glBindVertexArray(0); // unbind the VAO
	}
	glutSwapBuffers();
}

void initScene(int argc, char *argv[])
{
	// load the image from a jpeg disk file to main memory
	heightmapImage = new ImageIO();
	if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
	{
		cout << "Error reading image " << argv[1] << "." << endl;
		exit(EXIT_FAILURE);
	}
	// do additional initialization here...
	//set background to black, set depth test
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	// Image Data	////////////////////////////////////////	
	float height;
	GLuint iHeight, iWidth;
	GLuint iNumVert;
	iHeight = heightmapImage->getHeight();
	iWidth = heightmapImage->getWidth();
	iNumVert = iHeight * iWidth;
	H = heightmapImage->getHeight();
	W = heightmapImage->getWidth();
	//sizei for draw command
	numV = iHeight * iWidth;
	//	Points	//////////////////////////////////////////////
	// Points are stright forward 

	int p = 3 * iNumVert;
	pos = new GLfloat[p];
	int c = 4 * iNumVert ;
	color = new GLfloat[c];

	// set vertices and color
	//height = scale * heightmapImage->getPixel(i, j, 0);
	//Pixel ->Vertex
	//(i,j) -> (i, height, -j)
	int m;
	for (int i = 0; i < iHeight; i++)
	{
		for (int j = 0; j < iWidth; j++)
		{
			height = heightmapImage->getPixel(i, j, 0);
			//pos (x,y,z)
			m = (i*iWidth + j);
			pos[    3 * m] = i;
			pos[1 + 3 * m] = height * 0.1f;
			pos[2 + 3 * m] = j * -1;
			//color (r,g,b,a)
			color[    4 * m] = height / 255.0f;
			color[1 + 4 * m] = height / 255.0f;
			color[2 + 4 * m] = height / 255.0f;
			color[3 + 4 * m] = 1;
		}
	}

	//End Points	///////////////////////////////////////////
	
	// tri	and lines ///////////////////////////////////////////////////
	GLfloat corner1[3];
	GLfloat corner2[3];
	GLfloat corner3[3];
	GLfloat corner4[3];
	GLfloat cornerColor1[4];
	GLfloat cornerColor2[4];
	GLfloat cornerColor3[4];
	GLfloat cornerColor4[4];
	//quad from 2 triangles == 4 points 

	for (int k = 0; k < iHeight; k++)
	{
		for (int l = 0; l < iWidth; l++)
		{
			//position corners 
			height = heightmapImage->getPixel(k, l, 0);
			corner1[0] = k;
			corner1[1] = height * 0.1f;
			corner1[2] = -1*l;
			//color
			cornerColor1[0] = height / 255.0f;
			cornerColor1[1] = height / 255.0f;
			cornerColor1[2] = height / 255.0f;
			cornerColor1[3] = 1;
			//pos
			height = heightmapImage->getPixel(k + 1, l, 0);
			corner2[0] = k + 1;
			corner2[1] = height * 0.1f;
			corner2[2] = -1*l;
			//color
			cornerColor2[0] = height / 255.0f;
			cornerColor2[1] = height / 255.0f;
			cornerColor2[2] = height / 255.0f;
			cornerColor2[3] = 1;
			//pos
			height = heightmapImage->getPixel(k, l + 1, 0);
			corner3[0] = k;
			corner3[1] = height * 0.1f;
			corner3[2] = (l + 1)*-1;
			//color
			cornerColor3[0] = height / 255.0f;
			cornerColor3[1] = height / 255.0f;
			cornerColor3[2] = height / 255.0f;
			cornerColor3[3] = 1;
			//pos
			height = heightmapImage->getPixel(k + 1, l + 1, 0);
			corner4[0] = k + 1;
			corner4[1] = height * 0.1f;
			corner4[2] = (l + 1)*-1;
			//color
			cornerColor4[0] = height / 255.0f;
			cornerColor4[1] = height / 255.0f;
			cornerColor4[2] = height / 255.0f;
			cornerColor4[3] = 1;

			// 2 4     2    2 4
			// 1 3     1 3    3 
			//tri inidcies(123) 
			//1
			posTri.push_back(corner1[0]);//x
			posTri.push_back(corner1[1]);//y
			posTri.push_back(corner1[2]);//z

			colorTri.push_back(cornerColor1[0]);//r
			colorTri.push_back(cornerColor1[1]);//g
			colorTri.push_back(cornerColor1[2]);//b
			colorTri.push_back(cornerColor1[3]);//a
			//2
			posTri.push_back(corner2[0]);//x
			posTri.push_back(corner2[1]);//y
			posTri.push_back(corner2[2]);//z

			colorTri.push_back(cornerColor2[0]);//r
			colorTri.push_back(cornerColor2[1]);//g
			colorTri.push_back(cornerColor2[2]);//b
			colorTri.push_back(cornerColor2[3]);//a
			//3
			posTri.push_back(corner3[0]);//x
			posTri.push_back(corner3[1]);//y
			posTri.push_back(corner3[2]);//z

			colorTri.push_back(cornerColor3[0]);//r
			colorTri.push_back(cornerColor3[1]);//g
			colorTri.push_back(cornerColor3[2]);//b
			colorTri.push_back(cornerColor3[3]);//a

			//+ tri indines (324)
			//3
			posTri.push_back(corner3[0]);//x
			posTri.push_back(corner3[1]);//y
			posTri.push_back(corner3[2]);//z

			colorTri.push_back(cornerColor3[0]);//r
			colorTri.push_back(cornerColor3[1]);//g
			colorTri.push_back(cornerColor3[2]);//b
			colorTri.push_back(cornerColor3[3]);//a
			//2
			posTri.push_back(corner2[0]);//x
			posTri.push_back(corner2[1]);//y
			posTri.push_back(corner2[2]);//z

			colorTri.push_back(cornerColor2[0]);//r
			colorTri.push_back(cornerColor2[1]);//g
			colorTri.push_back(cornerColor2[2]);//b
			colorTri.push_back(cornerColor2[3]);//a
			//4
			posTri.push_back(corner4[0]);//x
			posTri.push_back(corner4[1]);//y
			posTri.push_back(corner4[2]);//z

			colorTri.push_back(cornerColor4[0]);//r
			colorTri.push_back(cornerColor4[1]);//g
			colorTri.push_back(cornerColor4[2]);//b
			colorTri.push_back(cornerColor4[3]);//a

			//tri angles done now lines
			//  2 4
			//  1 3
			// 1:2 , 2:4 , 4:3 , 3:1 , 1:4
			//1
			posLine.push_back(corner1[0]);//x
			posLine.push_back(corner1[1]);//y
			posLine.push_back(corner1[2]);//z

			colorLine.push_back(cornerColor1[0]);//r
			colorLine.push_back(cornerColor1[1]);//g
			colorLine.push_back(cornerColor1[2]);//b
			colorLine.push_back(cornerColor1[3]);//a
			//2
			posLine.push_back(corner2[0]);//x
			posLine.push_back(corner2[1]);//y
			posLine.push_back(corner2[2]);//z

			colorLine.push_back(cornerColor2[0]);//r
			colorLine.push_back(cornerColor2[1]);//g
			colorLine.push_back(cornerColor2[2]);//b
			colorLine.push_back(cornerColor2[3]);//a
			
			//2
			posLine.push_back(corner2[0]);//x
			posLine.push_back(corner2[1]);//y
			posLine.push_back(corner2[2]);//z

			colorLine.push_back(cornerColor2[0]);//r
			colorLine.push_back(cornerColor2[1]);//g
			colorLine.push_back(cornerColor2[2]);//b
			colorLine.push_back(cornerColor2[3]);//a
			//4
			posLine.push_back(corner4[0]);//x
			posLine.push_back(corner4[1]);//y
			posLine.push_back(corner4[2]);//z

			colorLine.push_back(cornerColor4[0]);//r
			colorLine.push_back(cornerColor4[1]);//g
			colorLine.push_back(cornerColor4[2]);//b
			colorLine.push_back(cornerColor4[3]);//a

			//4
			posLine.push_back(corner4[0]);//x
			posLine.push_back(corner4[1]);//y
			posLine.push_back(corner4[2]);//z

			colorLine.push_back(cornerColor4[0]);//r
			colorLine.push_back(cornerColor4[1]);//g
			colorLine.push_back(cornerColor4[2]);//b
			colorLine.push_back(cornerColor4[3]);//a
			//3
			posLine.push_back(corner3[0]);//x
			posLine.push_back(corner3[1]);//y
			posLine.push_back(corner3[2]);//z

			colorLine.push_back(cornerColor3[0]);//r
			colorLine.push_back(cornerColor3[1]);//g
			colorLine.push_back(cornerColor3[2]);//b
			colorLine.push_back(cornerColor3[3]);//a

			//3
			posLine.push_back(corner3[0]);//x
			posLine.push_back(corner3[1]);//y
			posLine.push_back(corner3[2]);//z

			colorLine.push_back(cornerColor3[0]);//r
			colorLine.push_back(cornerColor3[1]);//g
			colorLine.push_back(cornerColor3[2]);//b
			colorLine.push_back(cornerColor3[3]);//a
			//1
			posLine.push_back(corner1[0]);//x
			posLine.push_back(corner1[1]);//y
			posLine.push_back(corner1[2]);//z

			colorLine.push_back(cornerColor1[0]);//r
			colorLine.push_back(cornerColor1[1]);//g
			colorLine.push_back(cornerColor1[2]);//b
			colorLine.push_back(cornerColor1[3]);//a


			//1
			posLine.push_back(corner1[0]);//x
			posLine.push_back(corner1[1]);//y
			posLine.push_back(corner1[2]);//z

			colorLine.push_back(cornerColor1[0]);//r
			colorLine.push_back(cornerColor1[1]);//g
			colorLine.push_back(cornerColor1[2]);//b
			colorLine.push_back(cornerColor1[3]);//a
			//4
			posLine.push_back(corner4[0]);//x
			posLine.push_back(corner4[1]);//y
			posLine.push_back(corner4[2]);//z

			colorLine.push_back(cornerColor4[0]);//r
			colorLine.push_back(cornerColor4[1]);//g
			colorLine.push_back(cornerColor4[2]);//b
			colorLine.push_back(cornerColor4[3]);//a
		}
	}

	//End tri and lines  ////////////////////////////////////////////////
	
	//Init and bind the pipeline program :
	//pipelineProgram = new BasicPipelineProgram(); //new not needed for triangle 
	pipelineProgram.Init("../openGLHelper-starterCode");
	pipelineProgram.Bind();

	program = pipelineProgram.GetProgramHandle();
	// get a handle to the projectionMatrix shader variable
	h_projectionMatrix =
		glGetUniformLocation(program, "projectionMatrix");
	h_modelViewMatrix =
		glGetUniformLocation(program, "modelViewMatrix");

	//Milestone Triangle calls vbo first then vao // 
	//initVBO();
	//initVAO();
	//end triangle //

	// VAO//VBO  //
	
	initVBO(VBOPoints, p, c);
	initVBO(VBOTriPos,VBOTriColor,posTri,colorTri);
	initVBO(VBOLinesPos, VBOLinesColor, posLine, colorLine);

	initVAO(VAOPoints, VBOPoints, p);
	initVAO(VAOTri,VBOTriPos,VBOTriColor);
	initVAO(VAOLines, VBOLinesPos, VBOLinesColor);

	//End VAO//VBO  //

	delete pos;
	delete color;
}



// write a screenshot to the specified filename
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

void idleFunc()
{
  // do some stuff... 

  // for example, here, you can save the screenshots to disk (to make the animation)
	if (anim && n < 300)
	{
			string s = "SS/screenshot";
			string ss = s + std::to_string(n) + ".jpg";
			char* c = new char[ss.length() + 1];
			strcpy(c, ss.c_str());
			saveScreenshot(c);
			n++;	
	}
	if (n >= 300)
	{
		anim = false;
		n = 0;
	}
  // make the screen update keep at end
  glutPostRedisplay();
}

void reshapeFunc(int x, int y)
{
	glViewport(0, 0, x, y);
	openGLMatrix.SetMatrixMode(OpenGLMatrix::Projection);
	openGLMatrix.LoadIdentity();
	openGLMatrix.Perspective(45.0, 1.0 * x / y, 0.01, 2500.0);
	openGLMatrix.SetMatrixMode(OpenGLMatrix::ModelView);
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
	  dPoints = false;
	  dLines = false;
	  dTri = true;
	  break;
  case 'l':
	  //cycle modes
	  dPoints = false;
	  dTri = false;
	  dLines = true;
	  break;
  case 'p':
	  //cycle modes
	  dTri = false;
	  dLines = false;
	  dPoints = true;
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

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cout << "The arguments are incorrect." << endl;
    cout << "usage: ./hw1 <heightmap file>" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

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

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}