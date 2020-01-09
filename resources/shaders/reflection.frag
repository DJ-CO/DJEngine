#version 330 core
#define MAX_NUMBER_OF_LIGHTS 20
#define POINT 0
#define DIRECTIONAL 1
#define SPOT 2

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 colour;

struct Material
{
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_reflection1;
    float shininess;
};

struct Light
{
    vec3 position;// Location
    vec3 diffuse;// RGB of diffuse
    vec3 ambient;// RGB of ambient
    vec3 specular;// RGE of specular
    vec3 direction;// Components of direction
    float constant;// Amount of constant light
    float linear;// Amount of linear falloff
    float quadratic;// Amount of quadratic falloff
    float cutOff;
    float outerCutOff;
    int type;// The type of light: 0 for point light, 1 for directional light, 2 for spot light
};

uniform vec3 viewPos;
uniform samplerCube skybox;
uniform Material material;

uniform Light light[MAX_NUMBER_OF_LIGHTS];
uniform int NUMBER_OF_LIGHTS;

// Function prototypes
vec3 GammaCorrect (vec3 colour); // Function to gamma correct the final result
vec3 CalcLight (Light light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcReflection (vec3 colour);
vec3 CalcRefraction (void);

void main ( )
{
    vec3 norm = normalize ( Normal );
    vec3 viewDir = normalize( viewPos - FragPos );
    vec3 result = vec3 (0.0f, 0.0f, 0.0f);


    for (int i = 0; ((i < MAX_NUMBER_OF_LIGHTS)&&(i<NUMBER_OF_LIGHTS)); i++)
    {
        result+= CalcLight (light[i], norm, FragPos, viewDir);
    }

    // Maybe do the calc reflection to decide diffuse colour first, then use that diffuse colour in the lights
    result = CalcReflection (result); // Calculate reflections
    result = GammaCorrect (result);// Gamma correct

    //result = CalcRefraction ();
    colour = vec4 (result, 1.0);
}

vec3 CalcLight (Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    // Variables that apply to all lights
    vec3 lightDir;
    float diff;
    vec3 reflectDir;
    float spec;
    float attenuation;
    float intensity;

    // If light is a point light, do point light stuff
    if (light.type == POINT)
    {
        // Set variables
        lightDir = normalize(light.position - fragPos);
        diff = max(dot(normal, lightDir),0.0);
        reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir,reflectDir),0.0), material.shininess);

        // Do calculations
        float distance = length(light.position - fragPos);
        attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance*distance));
    }

    // If light is a directional light, do directional light stuff
    if (light.type == DIRECTIONAL)
    {
        // Set variables
        lightDir = normalize(-light.direction);
        diff = max(dot(normal, lightDir),0.0);
        reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir,reflectDir),0.0), material.shininess);
    }

    // If light is a spot light, do spot light stuff
    if (light.type == SPOT)
    {
        // Set variables
        lightDir = normalize(light.position - fragPos);
        diff = max(dot(normal, lightDir),0.0);
        reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir,reflectDir),0.0), material.shininess);

        float distance = length(light.position - fragPos);
        attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance*distance));
        float theta = dot (lightDir, normalize(-light.direction));
        float epsilon = light.cutOff - light.outerCutOff;
        intensity = clamp((theta-light.outerCutOff)/ epsilon, 0.0, 1.0);
    }

    // Middle stuff that applies to all lights
    vec3 ambient = light.ambient* vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1,TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1,TexCoords));

    // Finalization stuff that's specific to each light
    if (light.type == POINT)
    {
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    }

    if (light.type == DIRECTIONAL)
    {
    }

    if (light.type == SPOT)
    {
    ambient *= attenuation *intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    }

    return (ambient + diffuse + specular);
}

vec3 GammaCorrect (vec3 colour)
{
    float gamma = 2.2;
    return pow(colour.rgb, vec3(1.0/gamma));
}


// Do reflection calculations with the skybox
vec3 CalcReflection (vec3 colour)
{
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = reflect(I, normalize(Normal));
    vec3 reflectionFactor = texture(material.texture_reflection1,TexCoords).rgb;
    vec3 reflection = texture(skybox, R).rgb;
    vec3 result = mix(colour,reflection, reflectionFactor);
    return (result);
}

// Do refraction calculations with the skybox
vec3 CalcRefraction (void)
{
    float IOR = 1.52;
    float ratio = 1.00 / IOR;
    vec3 I = normalize(FragPos - viewPos);
    vec3 R = refract(I, normalize(Normal), ratio);
    return vec3(texture(skybox, R).rgb);
}
