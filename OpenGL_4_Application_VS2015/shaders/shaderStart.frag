#version 410 core

in vec3 normal;
in vec4 fragPosEye;
in vec4 fragPosLightSpace;
in vec2 fragTexCoords;

out vec4 fColor;

//lighting
uniform	mat3 normalMatrix;
uniform mat3 lightDirMatrix;
uniform	vec3 lightColor;
uniform	vec3 lightColor1;
uniform	vec3 lightDir;
uniform	vec3 lightPos1;
uniform	vec3 lightPos2;
uniform	vec3 lightPos3;
uniform	vec3 lightPos4;
uniform	vec3 lightPos5;
uniform	vec3 lightDir1;
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;
uniform mat4 view;

vec3 ambient;
float ambientStrength = 0.9f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 64.0f;

float ambientPoint = 1.0f;
float specularStrengthPoint = 1.0f;
float shininessPoint = 64.0f;

vec3 o;

struct PointLight {    
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

float computeShadow()
{	
	// perform perspective divide
    vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;
    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;
    // Check whether current frag pos is in shadow
    float bias = 0.005f;
    float shadow = currentDepth - bias> closestDepth  ? 1.0f : 0.0f;

    return shadow;	
}


vec3 computeLightComponents(vec3 lightDirection, vec3 lightColor, int shadow)
{		
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin	
	//transform normal
	vec3 normalEye = normalize(normalMatrix * normal);	
	//compute light direction
	vec3 lightDirN = normalize(lightDirMatrix * lightDirection);	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);	
	//compute half vector
	vec3 halfVector = normalize(lightDirN + viewDirN);		
	//compute ambient light
	ambient = ambientStrength * lightColor;	
	//compute diffuse light
	diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;	
	//compute specular light
	float specCoeff = pow(max(dot(halfVector, normalEye), 0.0f), shininess);
	specular = specularStrength * specCoeff * lightColor;

	ambient *= vec3(texture(diffuseTexture, fragTexCoords));
	diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
	specular *= vec3(texture(specularTexture, fragTexCoords));
	
	if(shadow == 1)
	{
		float shadow = computeShadow();
		//if(shadow == 1.0f)
		//	shadow = 0.8f;
	
		//modulate with shadow
		return min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
	}
	else
	{
		return (ambient + diffuse + specular);
	}
    
}

float computeFog()
{
 	float fogDensity = 0.05f;
 	float fragmentDistance = length(fragPosEye);
	float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
	return clamp(fogFactor, 0.0f, 1.0f);
}

vec3 computePointLight(vec4 lightPosEye, vec3 lightColor1, float constant, float linear, float quadratic)
{
    vec3 cameraPosEye = vec3(0.0f);
	vec3 normalEye = normalize(normalMatrix * normal);
	vec3 lightDirN = normalize(lightPosEye.xyz - fragPosEye.xyz);
	vec3 viewDirN = normalize(cameraPosEye - fragPosEye.xyz);
	vec3 ambient = ambientPoint * lightColor1;
	vec3 diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor1;
	vec3 halfVector = normalize(lightDirN + viewDirN);
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(normalEye, halfVector), 0.0f), shininessPoint);
	vec3 specular = specularStrengthPoint * specCoeff * lightColor1;
	float distance = length(lightPosEye.xyz - fragPosEye.xyz);
	
	float att = 1.0f / (constant + linear * distance + quadratic * distance * distance);
	
	ambient *= vec3(texture(diffuseTexture, fragTexCoords));
	diffuse *= vec3(texture(diffuseTexture, fragTexCoords));
	specular *= vec3(texture(specularTexture, fragTexCoords));
	
	return (ambient + diffuse + specular) * att;
} 

void main() 
{
	vec3 light = computeLightComponents(lightDir, lightColor, 1);

	vec4 lightPos1 = view * vec4(lightPos1, 1.0f);
	vec4 lightPos2 = view * vec4(lightPos2, 1.0f);
	vec4 lightPos3 = view * vec4(lightPos3, 1.0f);
	vec4 lightPos4 = view * vec4(lightPos4, 1.0f);
	
	light += computePointLight(lightPos1, vec3(0.0f, 0.0f, 2.0f), 1.0f, 1.0f, 0.044f); //albastru
	light += computePointLight(lightPos2, vec3(1.0f, 1.0f, 0.0f), 1.0f, 0.35f, 0.44f); //galben
	light += computePointLight(lightPos3, vec3(1.0f, 1.0f, 0.0f), 1.0f, 0.35f, 0.44f); //galben
	light += computePointLight(lightPos4, vec3(1.0f, 1.0f, 0.0f), 1.0f, 0.35f, 0.44f); //galben

	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.3f, 0.3f, 0.3f, 1.0f);
	fColor = fogColor * (1 - fogFactor) + vec4(light, 1.0f) * fogFactor;
}
