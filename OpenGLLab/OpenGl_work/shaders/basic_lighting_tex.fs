#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    float shininess;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

void main()
{
    // 텍스처 색 = object color 역할
    vec3 texColor = texture(material.diffuse, TexCoords).rgb;

   vec3 ambient = light.ambient -*texColor;

   vec3 norm = normalize(Normal);
   vec3 lightDir = normalize(light.position-FragPos);
   float diff = max(dot(norm, lightDir), 0.0);
   vec3 diffuse = light.diffuse * diff * texColor;


vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec;

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}