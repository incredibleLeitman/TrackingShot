#version 330 core

in vec2 texCoord;
in vec3 fragNormal;
in vec3 fragVert;
in vec4 fragPosLightSpace;
in vec4 baseColor;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;
//uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

// light
uniform struct Light {
   vec3 position;
   vec3 intensities;
} light;

out vec4 FragColor;

float calcShadows (vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(light.position - fragVert);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    // check whether current frag pos is in shadow
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    //float shadow = 0.0;

    vec2 texelSize = 1.0/textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}

void main ()
{
    //calculate normal in world coordinates
    //mat3 normalMatrix = transpose(inverse(mat3(model)));
    //vec3 normal = normalize(normalMatrix * fragNormal);
    vec3 normal = normalize(fragNormal);
    //vec3 normal = fragNormal;

    //calculate the location of this fragment (pixel) in world coordinates
    //vec3 fragPosition = vec3(model * vec4(fragVert, 1));
    vec3 fragPosition = fragVert;

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
    // EDIT: add shadow
    //FragColor = vec4(brightness * light.intensities * texColor.rgb, texColor.a);

    // calculate shadows
    float shadow = calcShadows(fragPosLightSpace);
    //vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;
    FragColor = vec4(brightness * (1.0 - shadow) * light.intensities * texColor.rgb, texColor.a);
    //FragColor = vec4(brightness + (1.0 - shadow) * light.intensities * texColor.rgb, texColor.a);
}