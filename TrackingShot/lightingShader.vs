#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

// pass to fragment shader
out vec2 TexCoord;
out vec3 fragNormal;
out vec3 fragVert;
out vec4 baseColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec4 color;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);

    // Pass some variables to the fragment shader
    //fragTexCoord = vertTexCoord;
    fragNormal = aNormal;
    //fragVert = vert;
    baseColor = color;
    
    // Apply all matrix transformations to vert
    //gl_Position = camera * model * vec4(vert, 1);
}