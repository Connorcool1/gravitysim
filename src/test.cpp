#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>
#include "shader.h"
#include <random>

const float windowHeight = 1000;
const float windowWidth = 1000;

std::vector<glm::vec3> vertices;

class Object {
    public:

    std::vector<float> pos;
    std::vector<float> vel;
    float radius;
    float mass;

    Object(std::vector<float> pos, std::vector<float> vel, float radius, float mass) {
        this->pos = pos;
        this->vel = vel;
        this->radius = radius;
        this->mass = mass;
        build();
    }

    // input in force
    void accelerate(float x, float y) {
        // calculate acceleration based off force and mass
        // std::vector<float> acc = std::vector<float>{x / mass, y / mass};
        this->vel[0] += x / dampening;
        this->vel[1] += y / dampening;
    };
    void updatePos(){
        this->pos[0] += vel[0] / dampening;
        this->pos[1] += vel[1] / dampening;
        if (allowCollision) { collision(); }
    };

    void processGravity(float g) {
        // only doing y right now
        accelerate(g, 0); 
    }

    void draw(Shader &shader) {
        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, glm::vec3(pos[0], pos[1], 0.0f));

        unsigned int transformLoc = glGetUniformLocation(shader.ID, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

        glDrawArrays(GL_TRIANGLES, startIndex, vertexCount);
    }

    private:
    int startIndex = 0;
    int vertexCount = 0;

    int vCount = 100;    
    float dampening = 100;
    
    bool allowCollision = true;
    
    void build() {
        startIndex = static_cast<int>(vertices.size());
        float angle = 360.0f / vCount;

        int triangleCount = vCount - 2;

        std::vector<glm::vec3> temp;
        for (int i = 0; i < vCount; i++) {
            float currentAngle = angle * i;
            float x = radius * cos(glm::radians(currentAngle));
            float y = radius * sin(glm::radians(currentAngle));
            float z = 0.0f;

            temp.push_back(glm::vec3(x,y,z));
        }
        
        for (int i = 0; i < triangleCount; i++) {
            vertices.push_back(temp[0]);
            vertices.push_back(temp[i + 1]);
            vertices.push_back(temp[i + 2]);
        }
        vertexCount = static_cast<int>(vertices.size()) - startIndex;
    }

    void collision() {
        if (pos[0] + radius > windowWidth) {
            pos[0] = windowWidth - radius;
            vel[0] *= -0.1;
        }
        if (pos[0] - radius < 0) {
            pos[0] = radius;
            vel[0] *= -0.1;
        }
        if (pos[1] + radius > windowHeight) {
            pos[1] = windowHeight - radius;
            vel[1] *= -0.1;
        }
        if (pos[1] - radius < 0) {
            pos[1] = radius;
            vel[1] *= -0.1;
        }
    }
};

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    GLFWwindow* window;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    window = glfwCreateWindow(windowHeight, windowWidth, "My GLFW Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glm::mat4 projection = glm::ortho(0.0f, windowWidth, windowHeight, 0.0f, -1.0f, 1.0f);

    // std::vector<Object> objs = {
    //     Object(std::vector<float>{500,400}, std::vector<float>{100.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{500,600}, std::vector<float>{-100.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{200,700}, std::vector<float>{0.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{800,700}, std::vector<float>{0.0f,0.0f}, 5.0f, 6 * pow(10, 22))

    // };
    // make random balls
    int ballCount = 50;
    std::vector<Object> objs;
    objs.reserve(ballCount);
    std::mt19937 rng((unsigned)std::random_device{}());
    std::uniform_real_distribution<float> px(50.0f, windowWidth - 50.0f);
    std::uniform_real_distribution<float> py(50.0f, windowHeight - 50.0f);
    std::uniform_real_distribution<float> rv(-60.0f, 60.0f);
    std::uniform_real_distribution<float> rr(4.0f, 10.0f);
    const float defaultMass = 6.0f * pow(10.0f, 22.0f);

    for (int i = 0; i < ballCount; ++i) {
        float radius = rr(rng);
        float x, y;
        int attempts = 0;
        bool placed = false;
        // simple non-overlap attempt
        while (attempts < 50 && !placed) {
            x = px(rng);
            y = py(rng);
            placed = true;
            for (const auto &other : objs) {
                float dx = other.pos[0] - x;
                float dy = other.pos[1] - y;
                float d = std::sqrt(dx*dx + dy*dy);
                if (d < (other.radius + radius + 2.0f)) { placed = false; break; }
            }
            ++attempts;
        }
        float vx = rv(rng);
        float vy = rv(rng);
        objs.emplace_back(std::vector<float>{x, y}, std::vector<float>{vx, vy}, radius, defaultMass);
    }
    
    // soo much graphics code
    Shader shader("shader.vs", "shader.fs");

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * vertices.size(), &vertices[0], GL_STATIC_DRAW); // Change static draw if vertices changes a lot

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float gravity = 9.81 / 20.0f;
    const float G = 6.6743 * pow(10, -11);

    shader.use();
    unsigned int projectionLoc = glGetUniformLocation(shader.ID, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        // Main loop content
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);

        for(Object& obj : objs) {
            for(Object& obj2 : objs) {
                if (&obj == &obj2) {continue;};
                float dx = obj2.pos[0] - obj.pos[0];
                float dy = obj2.pos[1] - obj.pos[1];
                float distance = sqrt(dx*dx + dy*dy);
                std::vector<float> direction = {dx / distance, dy / distance};
                if (distance < obj.radius *10) {continue;}
                distance *= 1000;

                float gf = (G * obj.mass * obj2.mass) / (distance * distance);

                float totalAcc = gf / obj.mass;
                std::vector<float> acc = {totalAcc * direction[0], totalAcc * direction[1]};
                obj.accelerate(acc[0], acc[1]);
            }

            obj.updatePos();
            obj.draw(shader);
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}