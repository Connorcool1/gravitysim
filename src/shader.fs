#version 330 core
out vec4 FragColor;
in vec3 vertexColor;

in vec3 Normal;
in vec3 FragPos;

uniform vec3 lightPos;

uniform bool light;
uniform bool grid;

void main() {
    if (light) {
        FragColor = vec4(1.0);
    }
    else if (grid) {
        FragColor = 0.2 * vec4(1.0);
    }
    else {
        float ambientStrength = 0.2;
        vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);
                
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

        vec3 colour = (ambient + diffuse) * vec3(1.0, 1.0, 1.0);
        FragColor = vec4(colour, 1.0);
    }
}