
varying vec3 vViewPos;
varying vec3 vViewNormal;
varying vec3 vViewLightPos;

varying vec2 fTex;

uniform sampler2D sampler;

uniform vec4 mat_diffuse;
uniform vec4 mat_ambient;

uniform vec4 light_diffuse;
uniform vec4 light_ambient;

void main(void) 
{
	vec3 lightDir = normalize(vViewLightPos - vViewPos);
	vec3 nn = normalize(vViewNormal);
	float LdotN = max(0.0f, dot(lightDir, nn));

	vec4 diffuse = vec4((mat_diffuse * light_diffuse * LdotN).rgb, 1.0f);

	vec4 ambient = vec4((light_ambient * mat_ambient).rgb, 1.0f);

	vec4 textureColor = texture2D(sampler, fTex);

	gl_FragColor = (diffuse + ambient) * textureColor;
}