#version 410
/*	(C) 2021 
	Bedrich Benes 
	Purdue University
	bbenes<#!&^#>purdue<.>edu
*/	


layout (location=0) in  vec4 iPosition;
layout (location=1) in  vec3 iNormal;
layout (location=0) out vec4 oPosCam;
layout (location=1) out vec3 oNormalCam;
uniform mat4  model;
uniform mat3  modelViewN;
uniform mat4  view;
uniform  mat4 proj;
void main()					
{     
	oNormalCam=normalize(modelViewN*iNormal);	  //n to camera coords
	oPosCam=view*model*iPosition;				  //v to camera coords	
	gl_Position = proj*view*model*iPosition;  //standard vertex out	          
}
