#version 330 core
#define MAX_NUMBER_OF_LIGHTS 20
#define POINT 0
#define DIRECTIONAL 1
#define SPOT 2

out vec4 colour;

in vec2 TexCoords;
in vec3 Normal;
in vec3 WorldPos;

struct Material
{
    sampler2D texture_albedo;
    sampler2D texture_specular;
    sampler2D texture_normal;
    sampler2D texture_metallic;
    sampler2D texture_roughness;
    sampler2D texture_AO;

    int hasAL;
    int hasSP;
    int hasNO;
    int hasME;
    int hasRO;
    int hasAO;

    float shininess;

    vec3 albedoHolder;
    float specularHolder;
    vec3 normalHolder;
    float metallicHolder;
    float roughnessHolder;
    float AOHolder;
};


struct DirLight
{
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};


struct Light
{
    vec3 position;// Location
    vec3 diffuse;// RGB of diffuse
    vec3 ambient;// RGB of ambient
    vec3 specular;// RGE of specular
    vec3 direction;// Components of direction
    /*
    float constant;// Amount of constant light
    float linear;// Amount of linear falloff
    float quadratic;// Amount of quadratic falloff
    */
    float cutOff;
    float outerCutOff;
    int type;// The type of light: 0 for point light, 1 for directional light, 2 for spot light
};

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

uniform vec3 lightPos;
uniform vec3 viewPos;
//uniform sampler2D texture_diffuse;
uniform Material material;

uniform int LIGHT_AMOUNT;
uniform DirLight dirLight;
uniform PointLight pointLight;
uniform SpotLight spotLight;
//uniform const int  NUMBER_OF_LIGHTS;
uniform Light light[MAX_NUMBER_OF_LIGHTS];


const float PI = 3.14159265359;

// Function prototypes
vec3 CalcDirLight (Light light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight (Light light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight (Light light, vec3 normal, vec3 fragPos, vec3 viewDir);
// int *testPointer;

vec3 getNormalFromMap();
vec3 GammaCorrect (vec3 colour); // Function to gamma correct the final result
vec3 fresnelSchlick(float cosTheta, vec3 F0); // Fresnel equation: caculates the ratio between specular and diffuse reflection
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness); // Normal distribution function
float GeometrySchlickGGX(float NdotV, float roughness); // Geometry function
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);

void main ( )
{

// Set parameters
    vec3 albedo     = pow(texture(material.texture_albedo, TexCoords).rgb, vec3(float (2.2)) );
    float specularAm   = texture(material.texture_specular, TexCoords).r;
    float metallic  = texture(material.texture_metallic, TexCoords).r;
    float roughness = texture(material.texture_roughness, TexCoords).r;
    float ao        = texture(material.texture_AO, TexCoords).r;
    vec3 N = getNormalFromMap();

    // Correct missing textures
    if (material.hasAL == 0) albedo = material.albedoHolder;
    if (material.hasSP == 0) specularAm = material.specularHolder;
    if (material.hasME == 0) metallic = material.metallicHolder;
    if (material.hasRO == 0) roughness = material.roughnessHolder;
    if (material.hasAO == 0) ao = material.AOHolder;
    if (material.hasNO == 0) N = normalize(Normal);

    vec3 V = normalize( viewPos - WorldPos );
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.2)*specularAm;
    F0 = mix(F0, albedo, metallic);


    // reflectance equation
    vec3 Lo = vec3(0.0);
    //for(int i = 0; i < NUMBER_OF_LIGHTS; i++)
    for(int i = 0; i < LIGHT_AMOUNT; i++)
    {
        // calculate per-light radiance
        vec3 L = normalize(light[i].position - WorldPos);
        vec3 H = normalize(V + L);
        float distance    = length(light[i].position - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance     = light[i].diffuse * attenuation;

        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
        vec3 specular     = numerator / denominator;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // ambient lighting (we now use IBL as the ambient term)
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    // Calculate ambient from environment map
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
   //vec3 prefilteredColor = textureLod(irradianceMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient    = (kD * diffuse + specular) * ao;
   // vec3 ambient    = (kD * diffuse + specular) * 0.0f;

    vec3 result = ambient + Lo;
    result = result / (result + vec3(1.0));
    result = GammaCorrect (result);// Gamma correct

    colour = vec4 (result, 1.0);

}


vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 GammaCorrect (vec3 colour)
{
    float gamma = 2.2;
    return pow(colour.rgb, vec3(1.0/gamma));
}

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(material.texture_normal, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}
