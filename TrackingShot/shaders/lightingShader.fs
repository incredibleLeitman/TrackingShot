#version 330 core

// arguments from vertex shader
in VS_OUT {
    vec3 fragVert;
    vec3 fragNormal;
    vec2 texCoord;

    vec4 fragPosLightSpace;
    vec4 baseColor;

    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

// texture samplers
uniform sampler2D shadowMap;
uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

// light
uniform struct Light {
   vec3 position;
   vec3 color;
} light;

uniform float bumpiness;

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
    // to reduce shadow acne (ugly Moiré-like pattern)
    vec3 normal = normalize(fs_in.fragNormal);
    //vec3 normal = normalize(texture(normalMap, fs_in.texCoord).rgb * 2.0 - 1.0);
    vec3 lightDir = normalize(light.position - fs_in.fragVert);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005); // because bias is dependent on angle between light and surface
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
    // calculate normal in world coordinates
    //vec3 normal = normalize(fs_in.fragNormal);
    // obtain normal from normal map in range [0,1] and transform normal vector to range [-1,1]
    //vec3 normal = normalize(texture(normalMap, fs_in.texCoord).rgb * 2.0 - 1.0);
    vec3 normal = normalize(texture(normalMap, fs_in.texCoord).rgb * 2.0 - bumpiness);

    // calculate the location of this fragment (pixel) in world coordinates
    //vec3 fragPosition = vec3(model * vec4(fragVert, 1));
    // EDITED: not needed, just use input fragVert

    // calculate final color of the pixel, based on baseColor mixed with texture
    vec4 texColor = mix(texture(diffuseMap, fs_in.texCoord), fs_in.baseColor, 0.5);

    // calculate shadows
    float shadow = calcShadows(fs_in.fragPosLightSpace);

    /*
    // simple lighting
    // calculate the vector from this pixels surface to the light source
    vec3 surfaceToLight = light.position - fs_in.fragVert;
    // calculate the cosine of the angle of incidence
    float brightness = clamp(dot(normal, surfaceToLight) / (length(surfaceToLight) * length(normal)), 0, 1);
    vec3 lighting = brightness * (1.0 - shadow) * light.color * texColor.rgb;
    */

    // advanced lighting (ambient, diffuse and specular) -> need tangens
    // ambient
    vec3 ambient = 0.3 * texColor.rgb;
    //vec3 ambient = 0.3 * light.color;

    // diffuse
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * light.color * texColor.rgb;

    // specular
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * light.color;

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * texColor.rgb;

    // resulting fragment color
    FragColor = vec4(lighting, texColor.a); // 1.0
}