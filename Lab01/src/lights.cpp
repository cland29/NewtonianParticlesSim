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
#include "shaders.h"    
#include "shapes.h"    
#include "lights.h"    

#pragma warning(disable : 4996)
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glut32.lib")

using namespace std;


LightC::LightC()
{
	lPos=glm::vec4(0,0,0,1);
	la=glm::vec3(0,0,0);
	ls=glm::vec3(1,1,1);
	ld=glm::vec3(0.7,0.7,0.7);
}
void LightC::SetShaders()
{
	glUniform4fv(lPosParameter,1,glm::value_ptr(lPos));
	glUniform3fv(laParameter,1,glm::value_ptr(la));
	glUniform3fv(ldParameter,1,glm::value_ptr(ld));
	glUniform3fv(lsParameter,1,glm::value_ptr(ls));
}

