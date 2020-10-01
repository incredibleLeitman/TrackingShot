#version 330 core

in vec2 TexCoord;
in vec3 fragNormal;
in vec3 fragVert;
in vec4 baseColor;

// transform
uniform mat4 transform;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;

// light
uniform struct Light {
   vec3 position;
   vec3 intensities;
} light;

out vec4 FragColor;

void main()
{
	//calculate normal in world coordinates
    mat3 normalMatrix = transpose(inverse(mat3(transform)));
    vec3 normal = normalize(normalMatrix * fragNormal);

    //calculate the location of this fragment (pixel) in world coordinates
    vec3 fragPosition = vec3(transform * vec4(fragVert, 1));

    //calculate the vector from this pixels surface to the light source
    vec3 surfaceToLight = light.position - fragPosition;

    //calculate the cosine of the angle of incidence
    float brightness = dot(normal, surfaceToLight) / (length(surfaceToLight) * length(normal));
    brightness = clamp(brightness, 0, 1);

    //calculate final color of the pixel, based on:
    // 1. The angle of incidence: brightness
    // 2. The color/intensities of the light: light.intensities
    // 3. The texture and texture coord: texture(tex, fragTexCoord)
    //      linearly interpolate between both textures (80% container, 20% awesomeface)
    //vec4 texColor = texture(tex, fragTexCoord);
    //vec4 texColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
    //vec4 texColor = vec4(.5, .5, .5, .5);
    vec4 texColor = baseColor;
    FragColor = vec4(brightness * light.intensities * texColor.rgb, texColor.a);
}