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

uniform vec3 viewPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 color;
uniform mat4 lightSpace;

// light
uniform struct Light {
   vec3 position;
   vec3 color;
} light;

void main ()
{
    // Pass some variables to the fragment shader
    //vs_out.fragVert = aPos;
    vs_out.fragVert = vec3(model * vec4(aPos, 1.0));
    //vs_out.fragNormal = aNormal;
    vs_out.fragNormal = transpose(inverse(mat3(model))) * aNormal;
    vs_out.texCoord = aTexCoord;
    
    vs_out.fragPosLightSpace = lightSpace * vec4(vs_out.fragVert, 1.0);
    vs_out.baseColor = color;

    mat3 normalMatrix = transpose(inverse(mat3(model)));

    /*
    // TODO: use tan and bitan from vertex buffer
    vec3 t = normalize(normalMatrix * aTangent);
    vec3 n = normalize(normalMatrix * aNormal);
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    */

    // calculate tan and bitan per vertex
    // Harald from https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
    vec3 c1 = cross(aNormal, vec3(0.0, 0.0, 1.0));
    vec3 c2 = cross(aNormal, vec3(0.0, 1.0, 0.0));
    vec3 tangent;
    if (length(c1)>length(c2))
    {
        tangent = c1;
    }
    else
    {
        tangent = c2;
    }
    vec3 t = normalize(normalMatrix * tangent);
    vec3 n = normalize(normalMatrix * aNormal);
    t = normalize(t - dot(t, n) * n);
    vec3 b = normalize(cross(n, t));

    mat3 tbn = transpose(mat3(t, b, n));
    vs_out.TangentLightPos = tbn * light.position;
    vs_out.TangentViewPos  = tbn * viewPos;
    vs_out.TangentFragPos  = tbn * vs_out.fragVert;

    // Apply all matrix transformations to vertices
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}