// shaders/basic_lighting_tex.fs
#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

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

uniform Material material;
uniform Light light;
uniform vec3 viewPos;

// shadow map 텍스처
uniform sampler2D shadowMap;

// ShadowCalculation 함수
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // 1. 관점 분할 및 텍스처 좌표 변환
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // 2. far plane 밖이면 그림자 없음
    if (projCoords.z > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    // 3. bias 계산 (빛이 비스듬히 들어올수록 bias 크게 설정함)
    vec3 N = normalize(normal);
    vec3 L = normalize(lightDir);
    float ndotl = max(dot(N, L), 0.0);
    float bias = max(0.005 * (1.0 - ndotl), 0.0005);

    // 4. 간단한 PCF (3x3)
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
    vec3 albedo = texture(material.diffuse, fs_in.TexCoords).rgb;

    vec3 N = normalize(fs_in.Normal);
    vec3 L = normalize(light.position - fs_in.FragPos);
    vec3 V = normalize(viewPos - fs_in.FragPos);

    // ambient 항
    vec3 ambient = light.ambient * albedo;

    // diffuse 항
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = light.diffuse * diff * albedo;

    // specular 항 (Blinn-Phong)
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), material.shininess);
    vec3 specTex = texture(material.specular, fs_in.TexCoords).rgb;
    vec3 specular = light.specular * spec * specTex;

    // 그림자 비율 계산
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, N, L);

    // ambient는 항상 유지, diffuse+specular에만 그림자 적용함
    vec3 lighting = ambient + (1.0 - shadow) * (diffuse + specular);

    FragColor = vec4(lighting, 1.0);
}