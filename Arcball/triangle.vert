
attribute vec3 vObjPos;
attribute vec3 vObjNormal;
attribute vec2 vTex;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

uniform mat4 lightModelMatrix;
uniform vec4 lightPosition;

uniform mat3 normalMatrix;

varying vec3 vViewPos;
varying vec3 vViewNormal;
varying vec3 vViewLightPos;

varying vec2 fTex;

void main(void) 
{
	gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(vObjPos, 1.0f);

	vViewPos = (viewMatrix * modelMatrix * vec4(vObjPos, 1.0f)).xyz;
	vViewNormal = normalize(normalMatrix * vObjNormal);
	vViewLightPos = (viewMatrix * lightModelMatrix * lightPosition).xyz;

	fTex = vTex;
}