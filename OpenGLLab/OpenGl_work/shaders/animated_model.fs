#version 330 core
out vec4 FragColor;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D shadowMap;
uniform Light light;
uniform vec3 viewPos;
uniform float shininess;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);
    float ndotl = max(dot(N, L), 0.0);
    float bias = max(0.005 * (1.0 - ndotl), 0.0005);

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;
    int radius = 1;
    for (int x = -radius; x <= radius; ++x)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            vec2 offset = vec2(x, y) * texelSize;
            float closestDepth = texture(shadowMap, projCoords.xy + offset).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    shadow /= float((2 * radius + 1) * (2 * radius + 1));
    return shadow;
}

void main()
{
    vec3 albedo = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    vec3 specTex = texture(texture_specular1, fs_in.TexCoords).rgb;

    vec3 N = normalize(fs_in.Normal);
    vec3 L = normalize(light.position - fs_in.FragPos);
    vec3 V = normalize(viewPos - fs_in.FragPos);

    vec3 ambient = light.ambient * albedo;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = light.diffuse * diff * albedo;

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = light.specular * spec * specTex;

    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, N, L);
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    FragColor = vec4(lighting, 1.0);
}

