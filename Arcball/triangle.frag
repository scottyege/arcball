
varying vec3 vViewPos;
varying vec3 vViewNormal;
varying vec3 vViewLightPos;

varying vec2 fTex;

uniform sampler2D sampler;
uniform sampler2D sampler2;

uniform vec4 mat_specular;
uniform vec4 mat_diffuse;
uniform vec4 mat_ambient;

uniform vec4 light_specular;
uniform vec4 light_diffuse;
uniform vec4 light_ambient;

float shineness = 30.0f;

void main(void) 
{
	vec3 lightDir = normalize(vViewLightPos - vViewPos);
	vec3 nn = normalize(vViewNormal);
	float LdotN = dot(lightDir, nn);

	vec4 ambient = vec4((light_ambient * mat_ambient).rgb, 1.0f);
	vec4 diffuse = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 specular = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	if(LdotN > 0)
	{
		diffuse = vec4((mat_diffuse * light_diffuse * LdotN).rgb, 1.0f);

		vec3 reflectDir = normalize(2.0f * nn * LdotN - lightDir);
		vec3 viewDir = normalize(-vViewPos);

		float VdotR = dot(viewDir, reflectDir);
		if(VdotR > 0)
		{
			specular = vec4((light_specular * mat_specular * pow(VdotR, shineness)).rgb, 1.0f);
		}
	}

	vec4 textureColor1 = texture2D(sampler, fTex);
	vec4 textureColor2 = texture2D(sampler2, fTex);
	vec4 textureColor = mix(textureColor1, textureColor2, 0.1f);

	gl_FragColor = (specular + diffuse + ambient) * textureColor;
}