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
std::vector<unsigned int> indices;

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f,0.0f,-1.0f);    
glm::vec3 cameraUp = glm::vec3(0.0f,1.0f,0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX =  1000.0f / 2.0;
float lastY =  1000.0 / 2.0;
bool firstMouse = true;

// time
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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

    static std::vector<Object> generate(int amount, 
                         std::vector<float> posRange = std::vector<float>{50.0f,windowWidth-50.0f,50.0f,windowHeight-50.0f},
                         std::vector<float> velRange = std::vector<float>{0, 0}, 
                         std::vector<float> rRange = std::vector<float>{4.0f, 10.0f}, 
                                      float mass = 6.0f*pow(10.0f, 22.0f)) {
        std::vector<Object> balls;
        balls.reserve(amount);
        std::mt19937 rng((unsigned)std::random_device{}());
        std::uniform_real_distribution<float> px(posRange[0], posRange[1]);
        std::uniform_real_distribution<float> py(posRange[2], posRange[3]);
        std::uniform_real_distribution<float> rv(velRange[0], velRange[1]);
        std::uniform_real_distribution<float> rr(rRange[0], rRange[1]);

        for (int i = 0; i < amount; ++i) {
            float radius = rr(rng);
            float x, y;
            int attempts = 0;
            bool placed = false;
            // simple non-overlap attempt
            while (attempts < 50 && !placed) {
                x = px(rng);
                y = py(rng);
                placed = true;
                for (const auto &other : balls) {
                    float dx = other.pos[0] - x;
                    float dy = other.pos[1] - y;
                    float d = std::sqrt(dx*dx + dy*dy);
                    if (d < (other.radius + radius + 2.0f)) { placed = false; break; }
                }
                ++attempts;
            }
            float vx = rv(rng);
            float vy = rv(rng);
            balls.emplace_back(std::vector<float>{x, y}, std::vector<float>{vx, vy}, radius, mass);
        }
        return balls;
    }

    private:
    int startIndex = 0;
    int vertexCount = 0;

    int vCount = 100;    
    float dampening = 1000;
    
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

    const float cameraSpeed = 2.5f * deltaTime;
    if (!glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (!glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (!glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (!glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
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

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    // std::vector<Object> objs = {
    //     Object(std::vector<float>{500,400}, std::vector<float>{100.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{500,600}, std::vector<float>{-100.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{200,700}, std::vector<float>{0.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{800,700}, std::vector<float>{0.0f,0.0f}, 5.0f, 6 * pow(10, 22))

    // };
    // make random balls
    // std::vector<Object> objs = Object::generate(3);
    // objs.emplace_back(Object(std::vector<float>{500,500}, std::vector<float>{0,0}, 30.0f, 6 * pow(10,24)));

    // vertices = std::vector<glm::vec3> {
    //     glm::vec3(100.0f,  100.0f, 0.0f),
    //     glm::vec3(100.0f, -100.0f, 0.0f),
    //     glm::vec3(-100.0f, -100.0f, 0.0f),
    //     glm::vec3(100.0f, 100.0f, 0.0f),
    //     glm::vec3(-100.0f, -100.0f, 0.0f),
    //     glm::vec3(-100.0f,  100.0f, 0.0f)
    // };
    vertices = std::vector<glm::vec3> {
        // positions          // texture coords
        glm::vec3( 0.5f,  0.5f, 0.0f), // top right
        glm::vec3( 0.5f, -0.5f, 0.0f), // bottom right
        glm::vec3(-0.5f, -0.5f, 0.0f), // bottom left
        glm::vec3(-0.5f,  -0.5f, 0.0f),  // top left
        glm::vec3(0.5f,  0.5f, 0.0f),  // top left
        glm::vec3(-0.5f,  0.5f, 0.0f)  // top left
    };

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

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        // Main loop content
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);

        // camera



        // glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

        // glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        // glm::vec3 cameraRight = glm::normalize(glm::cross(up, cameraDirection));

        // mvp

        glm::mat4 model = glm::mat4(1.0f);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        glm::mat4 projection = glm::mat4(1.0f);

        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        //projection = glm::ortho(0.0f, windowWidth, windowHeight, 0.0f, -500.0f, 500.0f);
        projection = glm::perspective(glm::radians(45.0f), (float)windowWidth/(float)windowHeight, 0.1f, 100.0f);

        unsigned int modelLoc = glGetUniformLocation(shader.ID, "model");
        unsigned int viewLoc = glGetUniformLocation(shader.ID, "view");
        unsigned int projectionLoc = glGetUniformLocation(shader.ID, "projection");
        
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // for(Object& obj : objs) {
        //     for(Object& obj2 : objs) {
        //         if (&obj == &obj2) {continue;};
        //         float dx = obj2.pos[0] - obj.pos[0];
        //         float dy = obj2.pos[1] - obj.pos[1];
        //         float distance = sqrt(dx*dx + dy*dy);
        //         std::vector<float> direction = {dx / distance, dy / distance};
        //         if (distance < obj.radius *10) {continue;}
        //         distance *= 1000;

        //         float gf = (G * obj.mass * obj2.mass) / (distance * distance);

        //         float totalAcc = gf / obj.mass;
        //         std::vector<float> acc = {totalAcc * direction[0], totalAcc * direction[1]};
        //         obj.accelerate(acc[0], acc[1]);
        //     }

        //     obj.updatePos();
        //     obj.draw(shader);
        // }

        // glm::mat4 transform = glm::mat4(1.0f);
        // transform = glm::translate(transform, glm::vec3(200.0f, 200.0f, 0.0f));

        // unsigned int transformLoc = glGetUniformLocation(shader.ID, "transform");
        // glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

        glDrawArrays(GL_TRIANGLES, 0, 6); 

        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}