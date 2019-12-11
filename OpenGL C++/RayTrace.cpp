/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: CharlieD
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100


#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <math.h>
#include <vector>

#define PI 3.14159265

char * filename = NULL;

//different display modes
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

//you may want to make these smaller for debugging purposes
#define WIDTH 640
#define HEIGHT 480

//the field of view of the camera
#define fov 60.0

unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);
glm::vec3 cop(0, 0, 0);


//every ray knows the pixel it was
//fired thru and the closest (Z value)
//to write to the correct pixel and Z buffer
struct Ray
{
	glm::vec3 Pos = cop;
	glm::vec3 Dir;
	glm::vec2 thruPixel;
	double closestZ = -1000;
	bool hasColor = false;
	glm::vec3 lightPos = cop;
};

struct vec3Pair
{
	glm::vec3 iPoint;
	glm::vec3 nDir;
	int inde = -1;
};

glm::vec3 PixelColor[WIDTH+1][HEIGHT+1];

glm::vec3 NormalsForTris[];
//glm::vec3 ImageCorners[4];
glm::vec3 sceneColor = glm::vec3(1.0, 1.0, 1.0);

std::vector<glm::vec3> pixels;
std::vector<Ray> rays;

vec3Pair DoSphere(Ray in)
{
	glm::vec3 rayP = in.Pos;
	glm::vec3 rayD = in.Dir;
	vec3Pair ret;
	ret.iPoint = cop;
	ret.nDir = cop;

	for (int i = 0; i < num_spheres; i++)
	{
		glm::vec3 sphereCenter = glm::vec3( spheres[i].position[0], spheres[i].position[1], spheres[i].position[2] );
		double radius = spheres[i].radius;
		double xd = rayD.x;
		double yd = rayD.y;
		double zd = rayD.z;

		double b = 2.0 * (  (xd * (rayP.x - sphereCenter.x))
						+ (yd * (rayP.y - sphereCenter.y))
						+ (zd * (rayP.z - sphereCenter.z)) ) ;

		double c = ((rayP.x - sphereCenter.x) * (rayP.x - sphereCenter.x))
				+ ((rayP.y - sphereCenter.y) * (rayP.y - sphereCenter.y))
				+ ((rayP.z - sphereCenter.z) * (rayP.z - sphereCenter.z) )
				- ( radius*radius ) ;

		if((b*b)-(4.0*c) < 0.0)
		{
			
		}
		else
		{
			double t0 = (-b + sqrt((b*b) - (4.0 * c))) / 2.0;
			double t1 = (-b - sqrt((b*b) - (4.0 * c))) / 2.0;

			if (t0 < 0 || t1 < 0)
			{
				
			}
			else
			{
				double firstT;
						
				if (t0 < t1)
				{
					firstT = t0;
				}
				else
				{
					firstT = t1;
				}
				
				glm::vec3 intersectionPoint = glm::vec3(xd * firstT, yd * firstT, zd * firstT);
				
				
				glm::vec3 normDir = intersectionPoint - sphereCenter;
				
				
				if (in.lightPos != cop)
				{
					glm::vec3 toLight = in.lightPos - rayP;
					glm::vec3 toCol = intersectionPoint - rayP;
					if (glm::length(toLight) < glm::length(toCol))
					{
						ret.iPoint = intersectionPoint;
						ret.inde = i;
						ret.nDir = normDir;
					}
				}
				else
				{

					ret.iPoint = intersectionPoint;

					ret.inde = i;

					ret.nDir = normDir;
				}
			}
		}
	}
	return ret;
}
vec3Pair DoTri(Ray in)
{
	vec3Pair ret;
	ret.iPoint = cop;
	ret.nDir = cop;

	glm::vec3 rayDir = in.Dir;
	glm::vec3 rayPos = in.Pos;
	for (int i = 0; i < num_triangles; i++)
	{
		glm::vec3 v0 = glm::vec3(triangles[i].v[0].position[0], 
								triangles[i].v[0].position[1], 
								triangles[i].v[0].position[2]);
	    glm::vec3 v1 = glm::vec3(triangles[i].v[1].position[0], 
								triangles[i].v[1].position[1],
								triangles[i].v[1].position[2]);
		glm::vec3 v2 = glm::vec3(triangles[i].v[2].position[0], 
								triangles[i].v[2].position[1], 
								triangles[i].v[2].position[2]);
		glm::vec3 TriNormal;
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		TriNormal = glm::cross(edge1, edge2);
		//printf("%d top \n", TriNormal.x);
		TriNormal = glm::normalize(TriNormal);
		
		float d = -glm::dot(TriNormal, v0);
		
		float top = glm::dot(TriNormal, rayPos);
		float bot = glm::dot(TriNormal, rayDir);

		if (bot == 0)
		{
		}
		else 
		{
			double t = -(top + d) / bot;
			//printf("%d time \n", t);

			if (t < DBL_EPSILON)
			{
			}
			else
			{
				//bool insec = false;
				glm::vec3 intersectionPoint = rayPos + (glm::vec3(rayDir.x * t, rayDir.y * t, rayDir.z * t));
				//test if in tri, winding order test
				glm::vec3 t0 = glm::cross(v1 - v0, intersectionPoint - v0);
				glm::vec3 t1 = glm::cross(v2 - v1, intersectionPoint - v1);
				glm::vec3 t2 = glm::cross(v0 - v2, intersectionPoint - v2);
				t0 = glm::normalize(t0);
				t1 = glm::normalize(t1);
				t2 = glm::normalize(t2);
				double a = glm::dot(TriNormal, t0);
				double b = glm::dot(TriNormal, t1);
				double c = glm::dot(TriNormal, t2);
				if (a >= 0.0 && b >= 0.0 && c >= 0.0)
				{
					if (in.lightPos != cop)
					{
						glm::vec3 toLight = in.lightPos - rayPos;
						glm::vec3 toCol = intersectionPoint - rayPos;
						if (glm::length(toLight) > glm::length(toCol))
						{
							ret.iPoint = intersectionPoint;
							ret.inde = i;
							ret.nDir = TriNormal;
						}
					}
					else 
					{	
						ret.iPoint = intersectionPoint;
						ret.inde = i;
						ret.nDir = TriNormal;
					}
				}
				else
				{
					//printf("fail");
				}
			}
		}
	}
	return ret;
}

glm::vec3 GetColorT(vec3Pair in)
{
	glm::vec3 retC;
	retC.x = ambient_light[0] * 255;
	retC.y = ambient_light[1] * 255;
	retC.z = ambient_light[2] * 255;
	for (int i = 0; i < num_lights; i++)
	{
		bool spass = false;
		bool tpass = false;
		//se if blocked, if not add color
		Ray out;
		out.Pos = in.iPoint;
		glm::vec3 lightP = glm::vec3(lights[i].position[0], lights[i].position[1], lights[i].position[2]);
		out.Dir = glm::normalize(lightP - in.iPoint);
		out.Pos.x += out.Dir.x*0.0000051;
		out.Pos.y += out.Dir.y*0.0000051;
		out.Pos.z += out.Dir.z*0.0000051;
		out.lightPos = lightP;
		vec3Pair r = DoSphere(out);
		glm::vec3 intPoint = r.iPoint;
		if (intPoint == cop)
		{
			//light is not blocked by sphere
			spass = true;
		}

		vec3Pair r1 = DoTri(out);
		glm::vec3 intPoint2 = r1.iPoint;
		if (intPoint2 == cop)
		{
			//light not blocked by triangle 
			tpass = true;
		}

		if (tpass && spass)
		{
		
			Triangle tri = triangles[in.inde];
			glm::vec3 v0 = glm::vec3(tri.v[0].position[0], tri.v[0].position[1], tri.v[0].position[3]);
			glm::vec3 v1 = glm::vec3(tri.v[1].position[0], tri.v[1].position[1], tri.v[1].position[3]);
			glm::vec3 v2 = glm::vec3(tri.v[2].position[0], tri.v[2].position[1], tri.v[2].position[3]);

			glm::vec3 triNor = in.nDir;
			glm::vec3 intPointT = in.iPoint;
			glm::vec3 c1 = glm::cross(v2 - v1, intPointT - v1);
			glm::vec3 c2 = glm::cross(v0 - v2, intPointT - v2);

			double a = glm::dot(c1,triNor) / glm::dot(triNor,triNor);
			double b = glm::dot(c2,triNor) / glm::dot(triNor,triNor);
			double g = 1.0 - a - b;
			
			//double test = a + b + g;

			glm::vec3 v0Nor = glm::vec3(tri.v[0].normal[0]*a, tri.v[0].normal[1]*a, tri.v[0].normal[2]*a);
			glm::vec3 v1Nor = glm::vec3(tri.v[1].normal[0]*b, tri.v[1].normal[1]*b, tri.v[1].normal[2]*b);
			glm::vec3 v2Nor = glm::vec3(tri.v[2].normal[0]*g, tri.v[2].normal[1]*g, tri.v[2].normal[2]*g);

			glm::vec3 v0Dif = glm::vec3(tri.v[0].color_diffuse[0]*a, tri.v[0].color_diffuse[1]*a, tri.v[0].color_diffuse[2]*a);
			glm::vec3 v1Dif = glm::vec3(tri.v[1].color_diffuse[0]*b, tri.v[1].color_diffuse[1]*b, tri.v[1].color_diffuse[2]*b);
			glm::vec3 v2Dif = glm::vec3(tri.v[2].color_diffuse[0]*g, tri.v[2].color_diffuse[1]*g, tri.v[2].color_diffuse[2]*g);

			glm::vec3 v0Spe = glm::vec3(tri.v[0].color_specular[0]*a, tri.v[0].color_specular[1]*a, tri.v[0].color_specular[2]*a);
			glm::vec3 v1Spe = glm::vec3(tri.v[1].color_specular[0]*b, tri.v[1].color_specular[1]*b, tri.v[1].color_specular[2]*b);
			glm::vec3 v2Spe = glm::vec3(tri.v[2].color_specular[0]*g, tri.v[2].color_specular[1]*g, tri.v[2].color_specular[2]*g);

			double shi =  tri.v[0].shininess*a + tri.v[1].shininess*b + tri.v[2].shininess*g ;

			

			glm::vec3 n = v0Nor + v1Nor + v2Nor;
			glm::vec3 dif = v0Dif + v1Dif + v2Dif;
			glm::vec3 spe = v0Spe + v1Spe + v2Spe;



			glm::vec3 l = lightP - in.iPoint;
			l = glm::normalize(l);
			
			n = glm::normalize(n);
			
			glm::vec3 v = cop - in.iPoint;
			v = glm::normalize(v);
			glm::vec3 r = -glm::reflect(l, n);
			r = glm::normalize(r);
			double LdotN = glm::dot(l, n);
			if (LdotN < 0.0) { LdotN = 0.0; }
			if (LdotN > 1.0) { LdotN = 1.0; }
			double RdotV = glm::dot(r, v);
			if (RdotV < 0.0) { RdotV = 0.0; }
			if (RdotV > 1.0) { RdotV = 1.0; }
			

			double rc = lights[i].color[0] * (dif[0] * LdotN)
				+ spe[0] * (double)(pow(RdotV,shi));
			double gc = lights[i].color[1] * (dif[1] * LdotN)
				+ spe[1] * (double)(pow(RdotV, shi));
			double bc = lights[i].color[2] * (dif[2] * LdotN)
				+ spe[2] * (double)(pow(RdotV, shi));

			retC.x += rc * 255.0;
			retC.y += gc * 255.0;
			retC.z += bc * 255.0;
			
			
			
			
			if (retC.x < 0.0) { retC.x = 0.0; }
			if (retC.x > 255.0) { retC.x = 255.0; }
			if (retC.y < 0.0) { retC.y = 0.0; }
			if (retC.y > 255.0) { retC.y = 255.0; }
			if (retC.z < 0.0) { retC.z = 0.0; }
			if (retC.z > 255.0) { retC.z = 255.0; }
			
		}
		else
		{
			//retC.z += 150;
			//break;
			//return retC;// = glm::vec3(0, 0, 0);
		}

	}
	return retC;
}

glm::vec3 GetColorS(vec3Pair in )
{
	glm::vec3 retC;
	retC.x = ambient_light[0]*255;
	retC.y = ambient_light[1]*255;
	retC.z = ambient_light[2]*255;
	for (int i = 0; i < num_lights; i++)
	{
		bool spass = false;
		bool tpass = false;
		//se if blocked, if not add color
		Ray out;
		out.Pos = in.iPoint;
		glm::vec3 lightP = glm::vec3(lights[i].position[0], lights[i].position[1], lights[i].position[2]);
		out.Dir = glm::normalize( lightP - in.iPoint);
		out.Pos.x += out.Dir.x*0.0000051;
		out.Pos.y += out.Dir.y*0.0000051;
		out.Pos.z += out.Dir.z*0.0000051;
		
		vec3Pair r = DoSphere(out);
		glm::vec3 intPoint = r.iPoint;
		if (intPoint == cop)
		{
			//light is not blocked by sphere
			spass = true;
		}
		
		vec3Pair r1 = DoTri(out);
		glm::vec3 intPoint2 = r1.iPoint;
		if (intPoint2 == cop)
		{
			//light not blocked by triangle 
			tpass = true;
		}

		if (tpass && spass)
		{

			//retC.z += 100;
			// l unit vec to light, n surface normal, v unit vec to cam r reflec of l at p
			//I = lightcolor * (kd * (L dot N) + ks* (R dot V)^sh)
			glm::vec3 l = lightP - in.iPoint;
			l = glm::normalize(l);
			glm::vec3 n = in.nDir;
			n = glm::normalize(n);
			glm::vec3 v = cop-in.iPoint ;
			v = glm::normalize(v);
			glm::vec3 r = -glm::reflect(l, n);
			r = glm::normalize(r);
			double LdotN = glm::dot(l, n);
			if (LdotN < 0.0){LdotN = 0.0;}
			if (LdotN > 1.0){LdotN = 1.0;}
			double RdotV = glm::dot(r, v);
			if (RdotV < 0.0){RdotV = 0.0;}
			if (RdotV > 1.0){ RdotV = 1.0;}
			
			double rc = lights[i].color[0] * (spheres[in.inde].color_diffuse[0] * LdotN)
						+ spheres[in.inde].color_specular[0] * (pow(RdotV, spheres[in.inde].shininess));
			double gc = lights[i].color[1] * (spheres[in.inde].color_diffuse[1] * LdotN)
						+ spheres[in.inde].color_specular[1] * (pow(RdotV, spheres[in.inde].shininess));
			double bc = lights[i].color[2] * (spheres[in.inde].color_diffuse[2] * LdotN)
						+ spheres[in.inde].color_specular[2] * (pow(RdotV, spheres[in.inde].shininess));
			
			retC.x += rc*255.0;
			retC.y += gc*255.0;
			retC.z += bc*255.0;
			if (retC.x < 0.0) { retC.x = 0.0; }
			if (retC.x > 255.0) { retC.x = 255.0; }
			if (retC.y < 0.0) { retC.y = 0.0; }
			if (retC.y > 255.0) { retC.y = 255.0; }
			if (retC.z < 0.0) { retC.z = 0.0; }
			if (retC.z > 255.0) { retC.z = 255.0; }


		}
		else
		{
			//break;
			//return retC;// = glm::vec3(0, 0, 0);
		}

	}
	return retC;
}

void DoRayTrace()
{
	for (int i = 0; i < rays.size(); i++)
	{
		//sphere
		//
		vec3Pair r =  DoSphere(rays[i]);
		glm::vec3 intPoint = r.iPoint;
		if (intPoint == cop)
		{
			//not intersection
		}
		else
		{
			if(rays[i].closestZ > intPoint.z)
			{
				//printf("could be a light issue later on");
			}
			else
			{
				rays[i].closestZ = intPoint.z;
				int x = rays[i].thruPixel.x;
				int y = rays[i].thruPixel.y;
				rays[i].hasColor = true;
				glm::vec3 rColor = GetColorS(r);

				PixelColor[x][y].x = rColor.x;//100;// ambient_light[0];
				PixelColor[x][y].y = rColor.y;//ambient_light[1];
				PixelColor[x][y].z = rColor.z;//ambient_light[2];
			}
		}
		//glm::vec3 intPoint2 = DoTri(rays[i]);
		//
		//sphere

		//tri
		vec3Pair r1 = DoTri(rays[i]);
		glm::vec3 intPoint2 = r1.iPoint;
		if (intPoint2 == cop)
		{
			//not intersection
		}
		else
		{
			//printf("got a color\n");
			if (rays[i].closestZ >= intPoint.z)
			{
				//printf("could be a light issue later on");
			}
			else
			{
				rays[i].closestZ = intPoint2.z;
				int x = rays[i].thruPixel.x;
				int y = rays[i].thruPixel.y;
				rays[i].hasColor = true;
				
				glm::vec3 rColor = GetColorT(r1);

				PixelColor[x][y].x = rColor.x;//100;// ambient_light[0];
				PixelColor[x][y].y = rColor.y;//ambient_light[1];
				PixelColor[x][y].z = rColor.z;//ambient_light[2];
				//PixelColor[x][y].x = ambient_light[0];
				//PixelColor[x][y].y = 100;// ambient_light[1];
				//PixelColor[x][y].z = ambient_light[2];
			}
		}
		//tri

	}
}

void SetRay(glm::vec3 pix, int x, int y)
{
	//for (int i = 0; i < pixels.size(); i++)
	{
		glm::vec3 d = pix - cop;
		glm::vec3 normD = glm::normalize(d);
		Ray r;
		r.Dir = normD;
		r.thruPixel.x = x;
		r.thruPixel.y = y;
		r.Pos = cop;
		rays.push_back(r);
	}
}
void SetPixels()
{
	//v1   v2  
	//   c = 0,0,-1
	//v3   v4 
	//v0 = -a tan(fov/2) , tan(fov/2), -1
	//v1 = a tan(fov/2) , tan(fov/2), -1
	//v2 = -a tan(fov/2) , -tan(fov/2), -1
	//v3 = a tan(fov/2) , -tan(fov/2), -1
	//
	//    480
	//    |
	//    |
	//    0 ------640
	
	//    2*tan(fov/2)
	//    |
	//    |
	//    0-------2*a*tan(fov/2)
	// y pixel steps = 2*tan(fov/2) / 480
	// x pixel steps = 2*a*tan(fov/2) / 640
	
	double a = double((float)WIDTH/(float)HEIGHT);
	double tanRes = double(tan( ( ((float)fov * (float)PI) / 180.0) / 2.0  ));
	double posX = a*tanRes;
	double negX = posX*-1.0;
	double negY = tanRes * -1.0;

	double xPixel =double( (2 * a * tanRes) /(float)WIDTH );
	double yPixel =double( (2 * tanRes) / (float)HEIGHT );
	for (int i = 0; i < HEIGHT+1; i++)
	{
		for (int j = 0; j < WIDTH+1; j++)
		{
			glm::vec3 p = (glm::vec3(negX+(j*xPixel), negY+(i*yPixel),-0.90) );
			SetRay(glm::normalize(p), j, i);
			
		}
	}
	DoRayTrace();
}

void ColorPixels()
{
	for (int k = 0; k < rays.size(); k++)
	{
		if (rays[k].hasColor)
		{
			
			int x = rays[k].thruPixel.x;
			int y = rays[k].thruPixel.y;
			plot_pixel(x,y , PixelColor[x][y].x, PixelColor[x][y].y, PixelColor[x][y].z);
		}
		else
		{  
			int x = rays[k].thruPixel.x;
			int y = rays[k].thruPixel.y;
			plot_pixel(x, y, 255, 255, 255);
		}
	}

}

//MODIFY THIS FUNCTION
void draw_scene()
{
  /*
  //a simple test output
  for(unsigned int x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    glBegin(GL_POINTS);
    for(unsigned int y=0; y<HEIGHT; y++)
    {
      plot_pixel(x, y, x % 256, y % 256, (x+y) % 256);
    }
    glEnd();
    glFlush();
  }
  printf("Done!\n"); fflush(stdout);
  */
	glPointSize(2.0);
	glBegin(GL_POINTS);
	//moved to init cam does not change 
	//RaySphereInter();
	ColorPixels();
	glEnd();
	glFlush();
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in Saving\n");
  else 
    printf("File saved Successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parse error, abnormal abortion\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(255,255,255,0);
  glClear(GL_COLOR_BUFFER_BIT);

  SetPixels();
  //RaySphereInter();
  //RayTriangleInter();
  //setrays moved inside setpixels
  //SetRays();

}

void idle()
{
  //hack to make it only draw once
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
  }
  once=1;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}

