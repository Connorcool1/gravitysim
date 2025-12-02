#version 330 core
out vec4 FragColor;
uniform vec3 vertexColor;

in vec3 Normal;
in vec3 FragPos;
uniform vec3 viewPos;

uniform vec3 lightPos;

uniform bool light;
uniform bool grid;

void main() {
    if (light) {
        FragColor = vec4(1.0);
    }
    else if (grid) {
        FragColor = 0.6 * vec4(1.0);
    }
    else {
        float ambientStrength = 0.5;
        vec3 ambient = ambientStrength * vertexColor;
                
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * vertexColor;

        vec3 colour = (ambient + diffuse + specular) * vertexColor;
        FragColor = vec4(colour, 1.0);
    }
}