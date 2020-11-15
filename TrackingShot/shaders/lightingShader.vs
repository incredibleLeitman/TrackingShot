#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;
// added information for normal maps
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

// pass to fragment shader
out VS_OUT {
    vec3 fragVert;
    vec3 fragNormal;
    vec2 texCoord;

    vec4 fragPosLightSpace;
    vec4 baseColor;

    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 color;
uniform mat4 lightSpace;

void main ()
{
    // Pass some variables to the fragment shader
    vs_out.texCoord = aTexCoord;
    //fragVert = aPos;
    vs_out.fragVert = vec3(model * vec4(aPos, 1.0));
    //fragNormal = aNormal;
    vs_out.fragNormal = transpose(inverse(mat3(model))) * aNormal;
    vs_out.fragPosLightSpace = lightSpace * vec4(vs_out.fragVert, 1.0);
    vs_out.baseColor = color;

    //mat3 TBN = transpose(mat3(T, B, N));
    //vs_out.TangentLightPos = TBN * lightPos;
    //vs_out.TangentViewPos  = TBN * viewPos;
    //vs_out.TangentFragPos  = TBN * vs_out.fragVert;

    // Apply all matrix transformations to vertices
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}