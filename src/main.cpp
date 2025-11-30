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

//std::vector<glm::vec3> vertices;
//std::vector<unsigned int> indices;

std::vector<unsigned int> lineIndices;

std::vector<glm::vec3> gridVertices;

std::vector<glm::vec3> lightPositions;

// camera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 1000.0f);
glm::vec3 cameraFront = glm::vec3(0.0f,0.0f,-1.0f);    
glm::vec3 cameraUp = glm::vec3(0.0f,1.0f,0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX =  1000.0f / 2.0f;
float lastY =  1000.0f / 2.0f;
bool firstMouse = true;

bool resetSim = false;

// time
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const float PI = 3.141592654;
const float c = 299792458; // speed of light in m/s
const float G = 6.67430e-11; // gravitational constant

void CreateBuffers(GLuint& VAO, GLuint& VBO, const glm::vec3* vertices, size_t vertexCount) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(glm::vec3), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

class Object {
    public:

    std::vector<float> pos;
    std::vector<float> vel;
    float radius;
    float mass;

    bool light;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

    GLuint VAO, VBO;

    Object(std::vector<float> pos, std::vector<float> vel, float radius, float mass, bool light = false) {
        this->pos = pos;
        this->vel = vel;
        this->radius = radius;
        this->mass = mass;
        this->light = light;
        build();
    }

    glm::vec3 GetPos() const {
        return glm::vec3(pos[0], pos[1], pos[2]);
    }

    // input in force
    void accelerate(float x, float y, float z) {
        // calculate acceleration based off force and mass
        // std::vector<float> acc = std::vector<float>{x / mass, y / mass};
        this->vel[0] += x / dampening;
        this->vel[1] += y / dampening;
        this->vel[2] += z / dampening;
    };
    void updatePos(){
        this->pos[0] += vel[0] / dampening;
        this->pos[1] += vel[1] / dampening;
        this->pos[2] += vel[2] / dampening;
        if (allowCollision) { collision(); }
    };

    void draw(Shader &shader) {
        unsigned int gridLoc = glGetUniformLocation(shader.ID, "grid");
        glUniform1i(gridLoc, 0);
        unsigned int lightLoc = glGetUniformLocation(shader.ID, "light");
        glUniform1i(lightLoc, light ? 1 : 0);

        if (!light) {
            glm::vec3 lightPos = lightPositions[0];
            shader.setVec3("lightPos", lightPos);
        }


        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(pos[0], pos[1], pos[2]));
        //model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));

        unsigned int modelLoc = glGetUniformLocation(shader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
        //glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    }

    static std::vector<Object> generate(int amount, 
                         std::vector<float> posRange = std::vector<float>{0.0f,500.0f,0.0f,500.0f,0.0f,500.0f},
                         std::vector<float> velRange = std::vector<float>{0, 0, 0}, 
                         std::vector<float> rRange = std::vector<float>{4.00f, 10.0f}, 
                                      float mass = 6.0f*pow(10.0f, 22.0f)) {
        std::vector<Object> balls;
        balls.reserve(amount);
        std::mt19937 rng((unsigned)std::random_device{}());
        std::uniform_real_distribution<float> px(posRange[0], posRange[1]);
        std::uniform_real_distribution<float> py(posRange[2], posRange[3]);
        std::uniform_real_distribution<float> pz(posRange[4], posRange[5]);
        std::uniform_real_distribution<float> rv(velRange[0], velRange[1]);
        std::uniform_real_distribution<float> rr(rRange[0], rRange[1]);

        for (int i = 0; i < amount; ++i) {
            float radius = rr(rng);
            float x, y, z;
            int attempts = 0;
            bool placed = false;
            // simple non-overlap attempt
            while (attempts < 50 && !placed) {
                x = px(rng);
                y = py(rng);
                z = pz(rng);
                placed = true;
                for (const auto &other : balls) {
                    float dx = other.pos[0] - x;
                    float dy = other.pos[1] - y;
                    float dz = other.pos[2] - z;
                    float hyp = std::sqrt(dx*dx + dy*dy);
                    float d = std::sqrt(hyp*hyp + dz*dz);
                    if (d < (other.radius + radius + 2.0f)) { placed = false; break; }
                }
                ++attempts;
            }
            float vx = rv(rng);
            float vy = rv(rng);
            float vz = rv(rng);
            balls.emplace_back(std::vector<float>{x, y, z}, std::vector<float>{vx, vy, vz}, radius, mass);
        }
        return balls;
    }

    private:
    int startIndex = 0;
    int indexCount = 0;
    int vertexCount = 0;
    int startVertex = 0;

    int vCount = 100;
    int sectorCount = 50;
    int stackCount = 50;    
    float dampening = 150;
    
    bool allowCollision = false;

    
    void build() {
        // buildCircle();
        buildSphere();
        CreateBuffers(VAO, VBO, vertices.data(), vertexCount);

        // normals
        if (!normals.empty()) {
            glBindVertexArray(VAO);
            GLuint normalVBO_local;
            glGenBuffers(1, &normalVBO_local);
            glBindBuffer(GL_ARRAY_BUFFER, normalVBO_local);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
    }

    void buildCircle() {
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

    void buildSphere() {
        startVertex = static_cast<int>(vertices.size());

        float x, y, z, xy;
        float nx, ny, nz, lengthInv = 1.0f / radius;
        float s, t;

        float sectorStep = 2 * PI / sectorCount;
        float stackStep = PI / stackCount;
        float sectorAngle, stackAngle;

        for(int i = 0; i <= stackCount; ++i) {
            stackAngle = PI / 2 - i * stackStep;
            xy = radius * cosf(stackAngle);
            z = radius * sinf(stackAngle);

            for(int j = 0; j <= sectorCount; ++j)
            {
                sectorAngle = j * sectorStep;           // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
                vertices.push_back(glm::vec3(x,y,z));

                // normalized vertex normal (nx, ny, nz)
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                this->normals.push_back(glm::vec3(nx,ny,nz));
                }
        }

        startIndex = static_cast<int>(indices.size());

        int k1, k2;
        for(int i = 0; i < stackCount; ++i)
        {
            k1 = i * (sectorCount + 1);     // beginning of current stack
            k2 = k1 + sectorCount + 1;      // beginning of next stack

            for(int j = 0; j < sectorCount; ++j, ++k1, ++k2)
            {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                if(i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                // k1+1 => k2 => k2+1
                if(i != (stackCount-1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }

                // store indices for lines
                // vertical lines for all stacks, k1 => k2
                lineIndices.push_back(k1);
                lineIndices.push_back(k2);
                if(i != 0)  // horizontal lines except 1st stack, k1 => k+1
                {
                    lineIndices.push_back(k1);
                    lineIndices.push_back(k1 + 1);
                }
            }
        }

        vertexCount = static_cast<int>(vertices.size());
        indexCount = static_cast<int>(indices.size());
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

class Grid {
    public:
    float cellSize;
    int cols;
    int rows;

    Grid(float width, float height, float cellSize) {
        this->cellSize = cellSize;
        this->cols = static_cast<int>(std::ceil(width / cellSize));
        this->rows = static_cast<int>(std::ceil(height / cellSize));
    }

    void CreateGrid() {
        float length = cols * cellSize;
        float halfLength = length / 2.0f;
        float y = -10.0f;
        for (int zStep = 0; zStep <= cols + 1; zStep++) {
            float z = zStep * cellSize;
            for (int xStep = 0; xStep <= rows; xStep++) {
                float xStart = xStep * cellSize;
                float xEnd = xStart + cellSize;
                gridVertices.push_back(glm::vec3(xStart - halfLength, y, z - halfLength));
                gridVertices.push_back(glm::vec3(xEnd - halfLength, y, z - halfLength));
            }
        }
        for (int xStep = 0; xStep <= rows + 1; xStep++) {
            float x = xStep * cellSize;
            for (int zStep = 0; zStep <= cols; zStep++) {
                float zStart = zStep * cellSize;
                float zEnd = zStart + cellSize;
                gridVertices.push_back(glm::vec3(x - halfLength, y, zStart - halfLength));
                gridVertices.push_back(glm::vec3(x - halfLength, y, zEnd - halfLength));
            }
        }
    }

    std::vector<glm::vec3> UpdateGrid(std::vector<glm::vec3> vertices, std::vector<Object> objs) {
        // iterate by reference so we modify the vector elements rather than a copy
        for (glm::vec3 &vertice : vertices) {
            vertice.y = 0.0f; // reset y displacement
            glm::vec3 totalDisplacement(0.0f);
            for (const auto &obj : objs) {

                glm::vec3 toObject = obj.GetPos() - vertice;
                float distance = glm::length(toObject);
                float distance_m = distance * 1000.0f;
                float rs = (2*G*obj.mass)/(c*c);

                float dz = 2 * sqrt(rs * (distance_m - rs));

                totalDisplacement.y += dz * 2.0f;
            }
            // write the computed displacement back into the vertex
            vertice.y = totalDisplacement.y - 300.0f; // offset to center grid
        }

        return vertices;
    }

    void Draw(Shader &shader) {
        unsigned int gridLoc = glGetUniformLocation(shader.ID, "grid");
        glUniform1i(gridLoc, 1);

        glm::mat4 model = glm::mat4(1.0f);

        unsigned int modelLoc = glGetUniformLocation(shader.ID, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawArrays(GL_LINES, 0, gridVertices.size());
    }
};

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 250.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraSpeed *= 5.0f;
    if (!glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (!glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (!glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (!glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        resetSim = true;

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

    glEnable(GL_DEPTH_TEST);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //glm::mat4 projection = glm::ortho(0.0f, windowWidth, windowHeight, 0.0f, -1.0f, 1.0f);

    // std::vector<Object> objs = {
    //     Object(std::vector<float>{500,400}, std::vector<float>{100.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{500,600}, std::vector<float>{-100.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{200,700}, std::vector<float>{0.0f,0.0f}, 5.0f, 6 * pow(10, 22)),
    //     Object(std::vector<float>{800,700}, std::vector<float>{0.0f,0.0f}, 5.0f, 6 * pow(10, 22))

    std::vector<Object> objs;

    // make random balls
    //objs = Object::generate(100);
    objs.emplace_back(Object(std::vector<float>{0,1,50}, std::vector<float>{0,0,0}, 20.0f, 4 * pow(10,23), true));
    objs.emplace_back(Object(std::vector<float>{-200,1,50}, std::vector<float>{0,0,-300}, 5.0f, 6 * pow(10,21)));
    objs.emplace_back(Object(std::vector<float>{200,1,50}, std::vector<float>{0,0,300}, 5.0f, 6 * pow(10,22)));

    std::vector<Object> reset = objs;


    Shader shader("shader.vs", "shader.fs");

    unsigned int gridVBO, gridVAO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    Grid grid(5000, 5000, 70.0f);
    grid.CreateGrid();

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * gridVertices.size(), &gridVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);



    float gravity = 9.81 / 20.0f;
    const float G = 6.6743 * pow(10, -11);

    shader.use();

    while (!glfwWindowShouldClose(window)) {
        if (resetSim) {
            objs = reset;
            resetSim = false;
        }

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        //model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        //projection = glm::ortho(0.0f, windowWidth, windowHeight, 0.0f, -500.0f, 500.0f);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth/(float)windowHeight, 0.1f, 750000.0f);

        unsigned int viewLoc = glGetUniformLocation(shader.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
        unsigned int projectionLoc = glGetUniformLocation(shader.ID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        int vertexColourLoc = glGetUniformLocation(shader.ID, "colour");
        glUniform4f(vertexColourLoc, 0.3f, 0.3f, 0.3f, 1.0f);

        gridVertices = grid.UpdateGrid(gridVertices, objs);

        // upload updated grid vertex positions to the GPU so Draw() uses the new data
        glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * gridVertices.size(), &gridVertices[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(gridVAO);
        grid.Draw(shader);

        glUniform4f(vertexColourLoc, 1.0f, 1.0f, 1.0f, 1.0f);
        lightPositions.clear();
        for(Object& obj : objs) {
            for(Object& obj2 : objs) {
                if (&obj == &obj2) {continue;};
                float dx = obj2.pos[0] - obj.pos[0];
                float dy = obj2.pos[1] - obj.pos[1];
                float dz = obj2.pos[2] - obj.pos[2];
                //std::cout << "Delta Position: (" << dx << ", " << dy << ", " << dz << ")\n";
                float hyp = sqrt(abs(dx*dx + dy*dy));
                //std::cout << "Hypotenuse: " << hyp << "\n";
                float distance = sqrt(abs(hyp*hyp + dz*dz));
                std::vector<float> direction = {dx / distance, dy / distance, dz / distance};
                if (distance < obj.radius *4) {continue;}
                distance *= 1000;

                float gf = ( (G * obj2.mass) / (distance*distance));
                gf *= obj.mass;

                float totalAcc = gf / obj.mass;

                std::vector<float> acc = {totalAcc * direction[0], totalAcc * direction[1], totalAcc * direction[2]};
                obj.accelerate(acc[0], acc[1], acc[2]);
            }
            //std::cout << "Object Position: (" << obj.pos[0] << ", " << obj.pos[1] << ", " << obj.pos[2] << ")\n";
            obj.updatePos();
            if (obj.pos[2] < -100000.0f || obj.pos[2] > 10000.0f) {
                std::cout << "Object out of bounds\n";
            }
            if (obj.light) {
                lightPositions.push_back(obj.GetPos());
            }

            glBindVertexArray(obj.VAO);
            obj.draw(shader);
        }

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

