#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

// pass to fragment shader
out vec2 texCoord;
out vec3 fragNormal;
out vec3 fragVert;
out vec4 fragPosLightSpace;
out vec4 baseColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 color;
uniform mat4 lightSpace;

void main ()
{
    // Pass some variables to the fragment shader
    //texCoord = vec2(aTexCoord.x, aTexCoord.y);
    texCoord = aTexCoord;
    //fragNormal = aNormal;
    //fragVert = aPos;
    fragNormal = transpose(inverse(mat3(model))) * aNormal;
    fragVert = vec3(model * vec4(aPos, 1.0));
    fragPosLightSpace = lightSpace * vec4(fragVert, 1.0);
    baseColor = color;

    // Apply all matrix transformations to vertices
    //gl_Position = camera * model * vec4(vert, 1);
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}