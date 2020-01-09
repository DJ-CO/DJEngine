#version 330 core
layout ( location = 0 ) in vec3 position;
layout ( location = 1 ) in vec3 normal;
layout ( location = 2 ) in vec2 texCoords;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main( )
{
    TexCoords = texCoords;
    WorldPos = vec3(model * vec4(position, 1.0));
    Normal = mat3(model) * normal;

    gl_Position =  projection * view * vec4(WorldPos, 1.0);



/*
    FragPos = vec3 (model*vec4(position, 1.0f));
    TexCoords = texCoords;
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));

    TangentLightPos = TBN * lightPos;
    TangentViewPos  = TBN * viewPos;
    TangentFragPos  = TBN * FragPos;

    gl_Position = projection * view * model * vec4( position, 1.0f );
*/
}
