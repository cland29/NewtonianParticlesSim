	 /**********************************/
/* Lighting					      
   (C) Bedrich Benes 2021         
   Diffuse and specular per fragment.
   bbenes@purdue.edu               */
/**********************************/

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <string.h>
#include "lodepng.h"
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <vector>			//Standard template library class
#include <GL/glew.h>
#include <GL/glut.h>
#include <omp.h>



//glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/half_float.hpp>
#include "shaders.h"    
#include "shapes.h"    
#include "lights.h"    

#pragma warning(disable : 4996)
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glut32.lib")

using namespace std;

bool needRedisplay=false;
ShapesC* sphere;
ShapesC* tri;
std::vector<unsigned char> image;
int terrProfileWidth, terrProfileLength;
double terrWidth = 10.0, terrLength = 10.0, terrHeightScale = 4.0;
double terrWidStep = 0.0, terrLenStep = 0.0;

vector<glm::vec3> spherePosVec;
vector<glm::vec3> sphereVelVec;
vector<glm::vec3> sphereAccVec;
vector<vector<glm::vec3>> triList;


//shader program ID
GLuint shaderProgram;
GLfloat ftime=0.f;
glm::mat4 view=glm::mat4(1.0);
glm::mat4 proj=glm::perspective(80.0f,//fovy
				  		        1.0f,//aspect
						        0.01f,1000.f); //near, far
class ShaderParamsC
{
public:
	GLint modelParameter;		//modeling matrix
	GLint modelViewNParameter;  //modeliview for normals
	GLint viewParameter;		//viewing matrix
	GLint projParameter;		//projection matrix
	//material
	GLint kaParameter;			//ambient material
	GLint kdParameter;			//diffuse material
	GLint ksParameter;			//specular material
	GLint shParameter;			//shinenness material
} params;


LightC light;


class Triangle3D {
	public:
		glm::vec3 edgeA, edgeB, edgeC;
		glm::vec3 vertexA, vertexB, vertexC;
		Triangle3D::Triangle3D(glm::vec3 vertexA, glm::vec3 vertexB, glm::vec3 vertexC) {
			this->vertexA = vertexA;
			this->vertexB = vertexB;
			this->vertexC = vertexC;
		}
		
};


bool getRayIntersection(vector<glm::vec3> tri, glm::vec3 rayOrigin, glm::vec3 rayVector, glm::vec3& outIntersectionVector) {
	//Borrowed from my project 2!
	//Taken from the Wikipedia article discussion Triangle intersection:
	//URL: 
	glm::vec3 vertexA = tri[0];
	glm::vec3 edgeA = tri[2] - tri[1];
	glm::vec3 edgeB = tri[0] - tri[2];
	glm::vec3 edgeC = tri[1] - tri[0];
	rayVector = glm::normalize(rayVector);
	const float EPSILON = 0.0000001;
	glm::vec3 edge1, edge2, h, s, q;
	float a, f, u, v, t;
	edge1 = edgeC;
	edge2 = glm::vec3(-edgeB[0], -edgeB[1], -edgeB[2]);
	h = glm::cross(rayVector, edge2);
	a = glm::dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;    // This ray is parallel to this triangle.
	f = 1.0 / a;
	s = rayOrigin - vertexA;
	u = f * glm::dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = glm::cross(s, edge1);
	v = f * glm::dot(rayVector, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	t = f * glm::dot(edge2, q);
	if (t > EPSILON) // ray intersection
	{
		outIntersectionVector = rayOrigin + t * rayVector;
		return true;
	}
	else // This means that there is a line intersection but not a ray intersection.
		return false;
}


//the main window size
GLint wWindow=800;
GLint hWindow=800;

float sh=1;

glm::vec3 eye = glm::vec3(-10, 3, -10);
glm::vec3 eyeVelF = glm::vec3(0, 0, 0);
glm::vec3 eyeVelS = glm::vec3(0, 0, 0);

/*********************************
Some OpenGL-related functions
**********************************/

//called when a window is reshaped
void Reshape(int w, int h)
{
  glViewport(0,0,w, h);       
  glEnable(GL_DEPTH_TEST);
//remember the settings for the camera
  wWindow=w;
  hWindow=h;
}

void decodeTwoSteps(const char* filename) {
  std::vector<unsigned char> png;
   //the raw pixels
  unsigned width, height;
  

  //load and decode
  unsigned error = lodepng::load_file(png, filename);
  if(!error) error = lodepng::decode(image, width, height, png);

  terrProfileWidth = (int)width;
  terrProfileLength = (int)height;

  //if there's an error, display it
  if(error) std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;

  //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
}


double getNormTerrHeight(std::vector<unsigned char> terrImage, int x, int y) {
	return -((double)terrImage[4 * terrProfileWidth * y + 4 * x]) / 255 * terrHeightScale;
		//((double)pow(x, 2) - (double)pow(y, 2)) / 10;
}

void Arm(glm::mat4 m)
{
//let's use instancing
	m=glm::translate(m,glm::vec3(0,0.5,0.0));
	m=glm::scale(m,glm::vec3(0.1f,0.1f,0.1f));
	sphere->SetModel(m);
	//now the normals
	glm::mat3 modelViewN=glm::mat3(view*m);
	modelViewN= glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();

	m=glm::translate(m,glm::vec3(0.0,0.5,0.0));
	m=glm::rotate(m,-20.0f*ftime,glm::vec3(0.0,0.0,1.0));
	m=glm::translate(m,glm::vec3(0.0,1.5,0.0));
	sphere->SetModel(glm::scale(m,glm::vec3(0.5f,1.0f,0.5f)));

	modelViewN=glm::mat3(view*m);
	modelViewN= glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();
}

void ball(glm::mat4 m)
{
	//let's use instancing
	m = glm::translate(m, glm::vec3(0, 0.5, 0.0));
	m = glm::scale(m, glm::vec3(0.1f, 0.1f, 0.1f));
	sphere->SetModel(m);
	//now the normals
	glm::mat3 modelViewN = glm::mat3(view * m);
	modelViewN = glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();
	/*
	m = glm::translate(m, glm::vec3(0.0, 0.5, 0.0));
	m = glm::rotate(m, -20.0f * ftime, glm::vec3(0.0, 0.0, 1.0));
	m = glm::translate(m, glm::vec3(0.0, 1.5, 0.0));
	sphere->SetModel(glm::scale(m, glm::vec3(0.5f, 1.0f, 0.5f)));

	modelViewN = glm::mat3(view * m);
	modelViewN = glm::transpose(glm::inverse(modelViewN));
	sphere->SetModelViewN(modelViewN);
	sphere->Render();*/
}


void Arm2(glm::mat4 m)
{
//let's use instancing
	m=glm::translate(m,glm::vec3(0,0.5,0.0));
	m=glm::scale(m,glm::vec3(1.0f,1.0f,1.0f));
	tri->SetModel(m);
	//now the normals
	glm::mat3 modelViewN=glm::mat3(view*m);
	modelViewN= glm::transpose(glm::inverse(modelViewN));
	tri->SetModelViewN(modelViewN);
	tri->Render();

	
}

//the main rendering function
void RenderObjects()
{
	eye = eye + eyeVelF + eyeVelS;
	const int range = 3;
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(0, 0, 0);
	glPointSize(2);
	glLineWidth(1);
	//set the projection and view once for the scene
	glUniformMatrix4fv(params.projParameter, 1, GL_FALSE, glm::value_ptr(proj));
	//view=glm::lookAt(glm::vec3(25*sin(ftime/40.f),5.f,15*cos(ftime/40.f)),//eye
	//			     glm::vec3(0,0,0),  //destination
	//			     glm::vec3(0,1,0)); //up
	view = glm::lookAt(glm::vec3(eye.x, eye.y, eye.z),//eye
					 //glm::vec3(terrLength/2, 0, terrWidth/2),
		glm::vec3(eye.x + 10.0, eye.y - 5.0, eye.z + 10.0),  //destination
		glm::vec3(0, 1, 0)); //up

	glUniformMatrix4fv(params.viewParameter, 1, GL_FALSE, glm::value_ptr(view));
	//set the light
	static glm::vec4 pos;
	pos.x = 20 * sin(ftime / 12); pos.y = -10; pos.z = 20 * cos(ftime / 12); pos.w = 1;
	pos.x = 20; pos.y = -10; pos.z = -20; pos.w = 1;
	light.SetPos(pos);
	light.SetShaders();
	for (int i = 0; i < spherePosVec.size(); i++) {
		glm::mat4 m = glm::translate(glm::mat4(1.0), spherePosVec[i]);
		ball(m);

	}
	/*
	# pragma opm parallel for
		for (int i = 0; i < spherePosVec.size(); i++) {
			//Check if the proposed position hits a triangle. If so, bounce.

			glm::vec3 propPos = spherePosVec[i] + sphereVelVec[i];
			double minX = floor(min(spherePosVec[i][0], propPos[0]));
			double maxX = ceil(max(spherePosVec[i][0], propPos[0]));
			double minZ = floor(min(spherePosVec[i][2], propPos[2]));
			double maxZ = ceil(max(spherePosVec[i][2], propPos[2]));
			glm::vec3 bounceVec = glm::vec3(0, 0, 0);
			bool bounce = false;

			for (vector<glm::vec3> tri: triList) {
				glm::vec3 outVec;
				getRayIntersection(tri, spherePosVec[i], sphereVelVec[i], outVec);
				if (glm::length(outVec - spherePosVec[i]) <= glm::length(sphereVelVec[i])) {
					spherePosVec[i] = glm::vec3(0, 10, 0);

					break;
				}
			}


			if (!bounce) {
				spherePosVec[i] = propPos;
			}
			sphereVelVec[i] = sphereVelVec[i] + sphereAccVec[i];

		}*/
#pragma omp parallel
	{
	#pragma omp for
	for (int i = 0; i < spherePosVec.size(); i++) {
		//Check if the proposed position hits a triangle. If so, bounce.
		//printf("%d: %d\n", omp_get_thread_num(), i);
		glm::vec3 propPos = spherePosVec[i] + sphereVelVec[i];
		double minX = floor(min(spherePosVec[i][0], propPos[0]));
		double maxX = ceil(max(spherePosVec[i][0], propPos[0]));
		double minZ = floor(min(spherePosVec[i][2], propPos[2]));
		double maxZ = ceil(max(spherePosVec[i][2], propPos[2]));

		double minXterr = floor(minX / terrWidStep);
		double maxXterr = ceil(maxX / terrWidStep);
		double minZterr = floor(minZ / terrWidStep);
		double maxZterr = ceil(maxZ / terrWidStep);

		glm::vec3 bounceVec = glm::vec3(0, 0, 0);
		bool bounce = false;

		if (i == 0) {
			printf("Pos %f, %f, %f", spherePosVec[i][0], spherePosVec[i][1], spherePosVec[i][2]);
			printf("Min/Max %f, %f, %f, %fZ %f, %f\n", minX, maxX, spherePosVec[i][0], propPos[0], minZ, maxZ);
		}
		
		for (int x = minXterr; x <= maxXterr; x++) {
			for (int z = minZterr; z <= maxZterr; z++) {
				glm::vec3 outVec;
				if (i == 0) {
					printf("debug test for triangle intersection %f, %f\n", x * terrWidStep, z * terrLenStep);
				}
				getRayIntersection(triList[z * terrProfileWidth + x], spherePosVec[i], sphereVelVec[i], outVec);
				if (glm::length(outVec - spherePosVec[i]) <= glm::length(sphereVelVec[i])) {
					printf("bounce!");
					spherePosVec[i] = glm::vec3(0, 10, 0);
					bounce = true;
					break;
				}
			}
		}
		if (spherePosVec[i][1] < 0) {
			sphereVelVec[i] = -1.0f * sphereVelVec[i];
			spherePosVec[i] = spherePosVec[i] + sphereVelVec[i];
			bounce = true;

		}

		if (!bounce) {
			spherePosVec[i] = propPos;
		}
		sphereVelVec[i] = sphereVelVec[i] + sphereAccVec[i];

	}
	}
	glm::mat4 m = glm::translate(glm::mat4(1.0), glm::vec3(0, 0, 0));
	Arm2(m);
	
}
	
void Idle(void)
{
  glClearColor(0.1,0.1,0.1,1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  ftime+=0.05;
  glUseProgram(shaderProgram);
  RenderObjects();
  glutSwapBuffers();  
}

void Display(void)
{

}

//keyboard callback
void Kbd(unsigned char a, int x, int y)
{
	switch(a)
	{
 	  case 27 : exit(0);break;
	  case 'r': 
	  case 'R': {sphere->SetKd(glm::vec3(1,0,0));break;}
	  case 'g': 
	  case 'G': {sphere->SetKd(glm::vec3(0,1,0));break;}
	  case 'b': 
	  case 'B': {sphere->SetKd(glm::vec3(0,0,1));break;}
	  case 'w': 
	  case 'W': {sphere->SetKd(glm::vec3(0.7,0.7,0.7));break;}
	  case '+': {sphere->SetSh(sh+=1);break;}
	  case '-': {sphere->SetSh(sh-=1);if (sh<1) sh=1;break;}

	  
	}
	cout << "shineness="<<sh<<endl;
	glutPostRedisplay();
}


//special keyboard callback
void SpecKbdPress(int a, int x, int y)
{
   	switch(a)
	{
 	  case GLUT_KEY_LEFT  : 
		  {
			  eyeVelS.x = 0.1;
			  eyeVelS.z = -0.1;
			  break;
		  }
	  case GLUT_KEY_RIGHT : 
		  {
			eyeVelS.x = -0.1;
			eyeVelS.z = 0.1;
			break;
		  }
 	  case GLUT_KEY_DOWN    : 
		  {
			eyeVelF.x = -0.1;
			eyeVelF.z = -0.1;
			break;
		  }
	  case GLUT_KEY_UP  :
		  {
			eyeVelF.x = 0.1;
			eyeVelF.z = 0.1;
			break;
		  }

	}
	glutPostRedisplay();
}

//called when a special key is released
void SpecKbdRelease(int a, int x, int y)
{
	switch(a)
	{
 	  case GLUT_KEY_LEFT  : 
		  {
			  eyeVelS.x = 0.0;
			  eyeVelS.z = 0.0;
			  break;
		  }
	  case GLUT_KEY_RIGHT : 
		  {
			  eyeVelS.x = 0.0;
			  eyeVelS.z = 0.0;
			  break;
		  }
 	  case GLUT_KEY_DOWN  : 
		  {
			eyeVelF.x = 0.0;
			eyeVelF.z = 0.0;
			break;
		  }
	  case GLUT_KEY_UP  :
		  {
			eyeVelF.x = 0.0;
			eyeVelF.z = 0.0;
			break;
		  }
	}
	glutPostRedisplay();
}


void Mouse(int button,int state,int x,int y)
{
	cout << "Location is " << "[" << x << "'" << y << "]" << endl;
}


void InitializeProgram(GLuint *program)
{
	std::vector<GLuint> shaderList;

//load and compile shaders 	
	shaderList.push_back(CreateShader(GL_VERTEX_SHADER,   LoadShader("shaders/phong.vert")));
	shaderList.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShader("shaders/phong.frag")));

//create the shader program and attach the shaders to it
	*program = CreateProgram(shaderList);

//delete shaders (they are on the GPU now)
	std::for_each(shaderList.begin(), shaderList.end(), glDeleteShader);

	params.modelParameter=glGetUniformLocation(*program,"model");
	params.modelViewNParameter=glGetUniformLocation(*program,"modelViewN");
	params.viewParameter =glGetUniformLocation(*program,"view");
	params.projParameter =glGetUniformLocation(*program,"proj");
	//now the material properties
	params.kaParameter=glGetUniformLocation(*program,"mat.ka");
	params.kdParameter=glGetUniformLocation(*program,"mat.kd");
	params.ksParameter=glGetUniformLocation(*program,"mat.ks");
	params.shParameter=glGetUniformLocation(*program,"mat.sh");
	//now the light properties
	light.SetLaToShader(glGetUniformLocation(*program,"light.la"));
	light.SetLdToShader(glGetUniformLocation(*program,"light.ld"));
	light.SetLsToShader(glGetUniformLocation(*program,"light.ls"));
	light.SetLposToShader(glGetUniformLocation(*program,"light.lPos"));
}

void InitShapes(ShaderParamsC *params)
{
//create one unit sphere in the origin
	sphere=new SphereC(50,50,1);
	sphere->SetKa(glm::vec3(0.1,0.1,0.1));
	sphere->SetKs(glm::vec3(0,0,1));
	sphere->SetKd(glm::vec3(0.7,0.7,0.7));
	sphere->SetSh(200);
	sphere->SetModel(glm::mat4(1.0));
	sphere->SetModelMatrixParamToShader(params->modelParameter);
	sphere->SetModelViewNMatrixParamToShader(params->modelViewNParameter);
	sphere->SetKaToShader(params->kaParameter);
	sphere->SetKdToShader(params->kdParameter);
	sphere->SetKsToShader(params->ksParameter);
	sphere->SetShToShader(params->shParameter);

	string filename = "benes.png";
	filename = "test2.png";
	decodeTwoSteps(filename.c_str());
	printf("Width%d Length %d\n", terrProfileLength, terrProfileWidth);
	
	for (int z = 0; z < terrProfileLength - 1; z++) {
		printf("z:%d\n", z);
		for (int x = 0; x < terrProfileWidth - 1; x++) {
			
			
			double widScale = (double) terrWidth/ (double)terrProfileWidth;
			double lenScale = (double) terrLength / (double)terrProfileLength;
			terrWidStep = widScale;
			terrLenStep = lenScale;
			
			//Gathers the points from the pixels such that 
			//the following vertex are labeled as such:
			//  1----2/6
			//  |   / |
			//  |  /  |
			//  | /   |
			// 3/5----4

			glm::vec3 vert1 = glm::vec3(x * widScale, getNormTerrHeight(image, x, z), z * lenScale);
			glm::vec3 vert2 = glm::vec3((x + 1.0) * widScale, getNormTerrHeight(image, x+1.0, z), z * lenScale);
			glm::vec3 vert3 = glm::vec3(x * widScale, getNormTerrHeight(image, x, z+1), (z+1.0) * lenScale);

			vector<glm::vec3> triVertList1;
			triVertList1.push_back(vert1); triVertList1.push_back(vert2); triVertList1.push_back(vert3);
			
			glm::vec3 vert4 = glm::vec3((x + 1.0) * widScale, getNormTerrHeight(image, x + 1, z + 1), (z + 1.0) * lenScale);
			glm::vec3 vert5 = glm::vec3(x * widScale, getNormTerrHeight(image, x, z + 1), (z + 1.0) * lenScale);
			glm::vec3 vert6 = glm::vec3((x + 1.0) * widScale, getNormTerrHeight(image, x + 1, z), z * lenScale);

			vector<glm::vec3> triVertList2;
			triVertList2.push_back(vert4); triVertList2.push_back(vert5); triVertList2.push_back(vert6);
			
			triList.push_back(triVertList1); triList.push_back(triVertList2);

		}
	}
	glm::vec3 vert1 = glm::vec3(0, 0, 0);
	glm::vec3 vert2 = glm::vec3(1, 0, 0);
	glm::vec3 vert3 = glm::vec3(0, 0, 1);

	vector<glm::vec3> triVertList1;
	triVertList1.push_back(vert1); triVertList1.push_back(vert2); triVertList1.push_back(vert3);
	//triList.push_back(triVertList1);

	glm::vec3 vert4 = glm::vec3(1, 0, 0);
	glm::vec3 vert5 = glm::vec3(1, 0, 1);
	glm::vec3 vert6 = glm::vec3(0, 0, 1);

	vector<glm::vec3> triVertList2;
	triVertList2.push_back(vert4); triVertList2.push_back(vert5); triVertList2.push_back(vert6);
	//triList.push_back(triVertList2);
	
	tri=new TriC(triList);
	tri->SetKa(glm::vec3(0.1,0.1,0.1));
	tri->SetKs(glm::vec3(0,0,1));
	tri->SetKd(glm::vec3(0.7,0.7,0.7));
	tri->SetSh(200);
	tri->SetModel(glm::mat4(1.0));
	tri->SetModelMatrixParamToShader(params->modelParameter);
	tri->SetModelViewNMatrixParamToShader(params->modelViewNParameter);
	tri->SetKaToShader(params->kaParameter);
	tri->SetKdToShader(params->kdParameter);
	tri->SetKsToShader(params->ksParameter);
	tri->SetShToShader(params->shParameter);



	//Create the intial Spheres:
	for (int i = 10; i < 30; i+=2) {
		for (int j = 10; j < 30; j+=2) {
			spherePosVec.push_back(glm::vec3(((double) j) / 5, 3, ((double)i) / 5));
			sphereVelVec.push_back(glm::vec3(0, 0.0, 0));
			sphereAccVec.push_back(glm::vec3(0, -0.007, 0.0));
		}
	}

}

int main(int argc, char **argv)
{ 
  glutInitDisplayString("stencil>=2 rgb double depth samples");
  glutInit(&argc, argv);
  glutInitWindowSize(wWindow,hWindow);
  glutInitWindowPosition(500,100);
  glutCreateWindow("Model View Projection GLSL");
  GLenum err = glewInit();
  if (GLEW_OK != err){
   fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
  }
  glutDisplayFunc(Display);
  glutIdleFunc(Idle);
  glutMouseFunc(Mouse);
  glutReshapeFunc(Reshape);
  glutKeyboardFunc(Kbd); //+ and -
  glutSpecialUpFunc(SpecKbdRelease); //smooth motion
  glutSpecialFunc(SpecKbdPress);
  InitializeProgram(&shaderProgram);
  InitShapes(&params);
  glutMainLoop();
  return 0;        
}
	