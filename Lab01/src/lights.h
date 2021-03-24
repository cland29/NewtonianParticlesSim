#ifndef __LIGHTSH__
#define __LIGHTSH__

#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <exception>
#include <stdexcept>
#include <string.h>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <time.h>
#include <string>
#include <vector>			//Standard template library class
#include <GL/glew.h>
#include <GL/glut.h>
//glm
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/half_float.hpp>

#pragma warning(disable : 4996)
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glut32.lib")

using namespace std;


class LightC
{
public:
	LightC();
	void SetPos(glm::vec4 par){lPos=par;};
	void SetLa(glm::vec3 par){la=par;};
	void SetLs(glm::vec3 par){ls=par;};
	void SetLd(glm::vec3 par){ld=par;};
	void SetLaToShader(GLuint uniform){laParameter=uniform;};
	void SetLdToShader(GLuint uniform){ldParameter=uniform;};
	void SetLsToShader(GLuint uniform){lsParameter=uniform;};
	void SetLposToShader(GLuint uniform){lPosParameter=uniform;};
	void SetShaders();

private:
	glm::vec4 lPos; //position
	glm::vec3 la;   //ambient
	glm::vec3 ld;   //diffuse
	glm::vec3 ls;   //specular
    GLuint	 laParameter;	 //shader uniform variables
    GLuint	 ldParameter;	 
    GLuint	 lsParameter;	 
    GLuint	 lPosParameter;	 
};

#endif