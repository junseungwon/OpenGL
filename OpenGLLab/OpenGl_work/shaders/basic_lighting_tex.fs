#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular; // ← 이제 텍스처에서 specular 세기를 읽어온다
    sampler2D emission; // emission map
    float     shininess;
};


struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos; // 현재 fragment의 World Space상에서의 좌표 => lighting dir, viewDir 계산에 사용              계산에 쓰임
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    vec3 texColor = texture(material.diffuse, TexCoords).rgb;
vec3 ambient  = light.ambient * texColor;

vec3 norm     = normalize(Normal);
vec3 lightDir = normalize(light.position - FragPos);
float diff    = max(dot(norm, lightDir), 0.0);
vec3 diffuse  = light.diffuse * diff * texColor;

vec3 viewDir    = normalize(viewPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);
float spec      = pow(max(dot(viewDir, reflectDir), 0.0),
                      material.shininess);

// specular map에서 intensity(흑백)를 읽어온다.
float specStrength = texture(material.specular, TexCoords).r;
vec3 specular = light.specular * spec * specStrength;

vec3 result = ambient + diffuse + specular;


  vec3 emissionColor = texture(material.emission, TexCoords).rgb;
    result += emissionColor; // 빛을 더해버림(아주 단순한 방식)



FragColor = vec4(result, 1.0);
}
