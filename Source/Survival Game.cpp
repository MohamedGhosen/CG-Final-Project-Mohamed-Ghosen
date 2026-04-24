#include <iostream>
#include<cmath>

#define GLEW_STATIC 
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"layout (location = 2) in vec3 aNormal;\n"
"out vec2 TexCoord;\n"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   FragPos = vec3(model * vec4(aPos, 1.0));\n"
"   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
"   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
"   TexCoord = aTexCoord;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 TexCoord;\n"
"in vec3 Normal;\n"
"in vec3 FragPos;\n"
"uniform sampler2D texture1;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 viewPos;\n"
"uniform vec3 lightColor;\n"
"void main()\n"
"{\n"
"   float ambientStrength = 0.2;\n"
"   vec3 ambient = ambientStrength * lightColor;\n"
"   vec3 norm = normalize(Normal);\n"
"   vec3 lightDir = normalize(lightPos - FragPos);\n"
"   float diff = max(dot(norm, lightDir), 0.0);\n"
"   vec3 diffuse = diff * lightColor;\n"
"   float specularStrength = 00.0f;\n"
"   vec3 viewDir = normalize(viewPos - FragPos);\n"
"   vec3 reflectDir = reflect(-lightDir, norm);\n"
"   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2);\n"
"   vec3 specular = specularStrength * spec * lightColor;\n"
"   vec3 texColor = texture(texture1, TexCoord).rgb;\n"
"   vec3 result = (ambient + diffuse + specular) * texColor;\n"
"   FragColor = vec4(result, 1.0);\n"
"}\n\0";

const char* HUDvertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"void main()\n"
"{\n"
"gl_Position = vec4(aPos, 0.0, 1.0f);\n"
"}\n\0";

const char* HUDfragmentShaderSource = "#version 330 \n"
"out vec4 FragColor;\n"
"uniform vec3 color;\n"
"void main () \n"
"{\n"
"FragColor = vec4(color, 1.0f);\n"
"}\n\0";

const char* sunvertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}\0";

const char* sunfragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
"}\0";

const char* debugVertex = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"void main()\n"
"{\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
"}";

const char* debugFrag = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
"}";

bool piece1collected = false;
bool piece2collected = false;
bool piece3collected = false;
int  collectedcount = 0;

float waterLevel = -1.5;
float waterRiseSpeed = 0.0005f;
bool gameOver = false;
bool gameWin = false;

float velocityY = 0.0f;
float gravity = -9.8f;
bool isGrounded = false;
float groundLevel = 1.0f;

bool onPlatform = false;

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

bool checkCollision(glm::vec3 playerPos, AABB box)
{
    return (playerPos.x >= box.min.x && playerPos.x <= box.max.x) &&
        (playerPos.z >= box.min.z && playerPos.z <= box.max.z);
};

glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

void processInput(GLFWwindow* window, AABB treeBoxes[], int treecount, AABB wallBoxes[], int wallcount, AABB boat)
{
    const float cameraSpeed = 0.05f;
    glm::vec3 newPos = cameraPos;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        newPos += cameraSpeed * glm::vec3(cameraFront.x, 0.0f, cameraFront.z);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        newPos -= cameraSpeed * glm::vec3(cameraFront.x, 0.0f, cameraFront.z);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        newPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        newPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && isGrounded) {
        velocityY = 5.0f;
        isGrounded = false;
    }

    bool collided = false;
    for (int i = 0; i < treecount; i++) {
        if (checkCollision(newPos, treeBoxes[i])) {
            collided = true;
            break;
        }
    }

    for (int i = 0; i < wallcount; i++) {
        if (checkCollision(newPos, wallBoxes[i])) {
            collided = true;
            break;
        }
    }

    if (checkCollision(newPos, boat)) {
        collided = true;
    }

    if (!collided) {
        cameraPos.x = newPos.x;
        cameraPos.z = newPos.z;
    }
}

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}

void drawAABB(AABB box, unsigned int shader, unsigned int VAO)
{
    glm::vec3 size = box.max - box.min;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, box.min);
    model = glm::scale(model, size);

    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, 24);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, " CG final project", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    unsigned int HUDvertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(HUDvertexShader, 1, &HUDvertexShaderSource, NULL);
    glCompileShader(HUDvertexShader);

    glGetShaderiv(HUDvertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(HUDvertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::HUDVERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int HUDfragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(HUDfragmentShader, 1, &HUDfragmentShaderSource, NULL);
    glCompileShader(HUDfragmentShader);

    glGetShaderiv(HUDvertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(HUDvertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::HUDfragment::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int HUDshaderProgram = glCreateProgram();
    glAttachShader(HUDshaderProgram, HUDvertexShader);
    glAttachShader(HUDshaderProgram, HUDfragmentShader);
    glLinkProgram(HUDshaderProgram);


    glGetProgramiv(HUDshaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(HUDshaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::HUDPROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    unsigned int sunvertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(sunvertexShader, 1, &sunvertexShaderSource, NULL);
    glCompileShader(sunvertexShader);

    glGetShaderiv(sunvertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(sunvertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::sunVERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int sunfragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(sunfragmentShader, 1, &sunfragmentShaderSource, NULL);
    glCompileShader(sunfragmentShader);

    glGetShaderiv(sunfragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::sunfragment::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int sunshaderProgram = glCreateProgram();
    glAttachShader(sunshaderProgram, sunvertexShader);
    glAttachShader(sunshaderProgram, sunfragmentShader);
    glLinkProgram(sunshaderProgram);


    glGetProgramiv(sunshaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(sunshaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::sunPROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    unsigned int debugvertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(debugvertexShader, 1, &debugVertex, NULL);
    glCompileShader(debugvertexShader);

    glGetShaderiv(debugvertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(debugvertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::debugVERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int debugfragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(debugfragmentShader, 1, &debugFrag, NULL);
    glCompileShader(debugfragmentShader);

    glGetShaderiv(debugfragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(debugfragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::debugFRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    unsigned int debugshaderProgram = glCreateProgram();
    glAttachShader(debugshaderProgram, debugvertexShader);
    glAttachShader(debugshaderProgram, debugfragmentShader);
    glLinkProgram(debugshaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(HUDvertexShader);
    glDeleteShader(HUDfragmentShader);
    glDeleteShader(sunvertexShader);
    glDeleteShader(sunfragmentShader);
    glDeleteShader(debugvertexShader);
    glDeleteShader(debugfragmentShader);

    float waterVertices[] = {
           80.0f, -1.5f,  80.0f,  20.0f, 20.0f, 0.0f, 1.0f, 0.0f,
          -80.0f, -1.5f,  80.0f,  0.0f,  20.0f, 0.0f, 1.0f, 0.0f,
          -80.0f, -1.5f, -80.0f,  0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
          -80.0f, -1.5f, -80.0f,  0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
           80.0f, -1.5f, -80.0f,  20.0f, 0.0f,  0.0f, 1.0f, 0.0f,
           80.0f, -1.5f,  80.0f,  20.0f, 20.0f, 0.0f, 1.0f, 0.0f
    };

    unsigned int waterVBO, waterVAO;
    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glBindVertexArray(waterVAO);
    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(waterVertices), waterVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float sandVertices[] = {
         40.0f, -1.0f,  40.0f,  10.0f, 10.0f, 0.0f, 1.0f, 0.0f,
        -40.0f, -1.0f,  40.0f,  0.0f,  10.0f, 0.0f, 1.0f, 0.0f,
        -40.0f, -1.0f, -40.0f,  0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -40.0f, -1.0f, -40.0f,  0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         40.0f, -1.0f, -40.0f,  10.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         40.0f, -1.0f,  40.0f,  10.0f, 10.0f, 0.0f, 1.0f, 0.0f
    };

    unsigned int sandVBO, sandVAO;
    glGenVertexArrays(1, &sandVAO);
    glGenBuffers(1, &sandVBO);
    glBindVertexArray(sandVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sandVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sandVertices), sandVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float slopeVertices[] = {
             25.0f,  0.5f,  25.0f,  0.0f, 0.0f,  0.0f, 0.7f, 0.7f,
             40.0f, -1.0f,  40.0f,  1.0f, 0.0f,  0.0f, 0.7f, 0.7f,
            -40.0f, -1.0f,  40.0f,  1.0f, 1.0f,  0.0f, 0.7f, 0.7f,

            -40.0f, -1.0f,  40.0f,  1.0f, 1.0f,  0.0f, 0.7f, 0.7f,
            -25.0f,  0.5f,  25.0f,  0.0f, 1.0f,  0.0f, 0.7f, 0.7f,
             25.0f,  0.5f,  25.0f,  0.0f, 0.0f,  0.0f, 0.7f, 0.7f,

             25.0f,  0.5f, -25.0f,  0.0f, 0.0f,  0.0f, 0.7f, -0.7f,
             40.0f, -1.0f, -40.0f,  1.0f, 0.0f,  0.0f, 0.7f, -0.7f,
            -40.0f, -1.0f, -40.0f,  1.0f, 1.0f,  0.0f, 0.7f, -0.7f,

            -40.0f, -1.0f, -40.0f,  1.0f, 1.0f,  0.0f, 0.7f, -0.7f,
            -25.0f,  0.5f, -25.0f,  0.0f, 1.0f,  0.0f, 0.7f, -0.7f,
             25.0f,  0.5f, -25.0f,  0.0f, 0.0f,  0.0f, 0.7f, -0.7f,

             25.0f,  0.5f,  25.0f,  0.0f, 0.0f,  0.7f, 0.7f,  0.0f,
             40.0f, -1.0f,  40.0f,  1.0f, 0.0f,  0.7f, 0.7f,  0.0f,
             40.0f, -1.0f, -40.0f,  1.0f, 1.0f,  0.7f, 0.7f,  0.0f,

             40.0f, -1.0f, -40.0f,  1.0f, 1.0f,  0.7f, 0.7f,  0.0f,
             25.0f,  0.5f, -25.0f,  0.0f, 1.0f,  0.7f, 0.7f,  0.0f,
             25.0f,  0.5f,  25.0f,  0.0f, 0.0f,  0.7f, 0.7f,  0.0f,

            -25.0f,  0.5f,  25.0f,  0.0f, 0.0f, -0.7f, 0.7f,  0.0f,
            -40.0f, -1.0f,  40.0f,  1.0f, 0.0f, -0.7f, 0.7f,  0.0f,
            -40.0f, -1.0f, -40.0f,  1.0f, 1.0f, -0.7f, 0.7f,  0.0f,

            -40.0f, -1.0f, -40.0f,  1.0f, 1.0f, -0.7f, 0.7f,  0.0f,
            -25.0f,  0.5f, -25.0f,  0.0f, 1.0f, -0.7f, 0.7f,  0.0f,
            -25.0f,  0.5f,  25.0f,  0.0f, 0.0f, -0.7f, 0.7f,  0.0f,
    };

    unsigned int slopeVBO, slopeVAO;
    glGenVertexArrays(1, &slopeVAO);
    glGenBuffers(1, &slopeVBO);
    glBindVertexArray(slopeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, slopeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(slopeVertices), slopeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float grassVertices[] = {
       25.0f,  0.5f,  25.0f,  10.0f, 10.0f, 0.0f, 1.0f, 0.0f,
      -25.0f,  0.5f,  25.0f,  0.0f,  10.0f, 0.0f, 1.0f, 0.0f,
      -25.0f,  0.5f, -25.0f,  0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
      -25.0f,  0.5f, -25.0f,  0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
       25.0f,  0.5f, -25.0f,  10.0f, 0.0f,  0.0f, 1.0f, 0.0f,
       25.0f,  0.5f,  25.0f,  10.0f, 10.0f, 0.0f, 1.0f, 0.0f
    };

    unsigned int grassVBO, grassVAO;
    glGenVertexArrays(1, &grassVAO);
    glGenBuffers(1, &grassVBO);
    glBindVertexArray(grassVAO);
    glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grassVertices), grassVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float piece1Vertices[] = {
    -0.4f, 1.6f,  8.05f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.4f, 1.6f,  8.05f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.4f, 1.8f,  8.05f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
     0.4f, 1.8f,  8.05f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.4f, 1.8f,  8.05f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.4f, 1.6f,  8.05f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,

    -0.4f, 1.6f,  7.95f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.4f, 1.6f,  7.95f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.4f, 1.8f,  7.95f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     0.4f, 1.8f,  7.95f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.4f, 1.8f,  7.95f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.4f, 1.6f,  7.95f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

    -0.4f, 1.8f,  8.05f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.4f, 1.8f,  7.95f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.4f, 1.6f,  7.95f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.4f, 1.6f,  7.95f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.4f, 1.6f,  8.05f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.4f, 1.8f,  8.05f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,

     0.4f, 1.8f,  8.05f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.4f, 1.8f,  7.95f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.4f, 1.6f,  7.95f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.4f, 1.6f,  7.95f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.4f, 1.6f,  8.05f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.4f, 1.8f,  8.05f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,

    -0.4f, 1.8f,  7.95f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,
     0.4f, 1.8f,  7.95f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f,
     0.4f, 1.8f,  8.05f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
     0.4f, 1.8f,  8.05f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
    -0.4f, 1.8f,  8.05f,  0.0f, 0.0f,  0.0f, 1.0f,  0.0f,
    -0.4f, 1.8f,  7.95f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,

    -0.4f, 1.6f,  7.95f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f,
     0.4f, 1.6f,  7.95f,  1.0f, 1.0f,  0.0f,-1.0f,  0.0f,
     0.4f, 1.6f,  8.05f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
     0.4f, 1.6f,  8.05f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
    -0.4f, 1.6f,  8.05f,  0.0f, 0.0f,  0.0f,-1.0f,  0.0f,
     -0.4f, 1.6f,  7.95f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f
    };

    unsigned int piece1VBO, piece1VAO;
    glGenVertexArrays(1, &piece1VAO);
    glGenBuffers(1, &piece1VBO);
    glBindVertexArray(piece1VAO);
    glBindBuffer(GL_ARRAY_BUFFER, piece1VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(piece1Vertices), piece1Vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float piece2Vertices[] = {
50.6f, 1.6f, 51.05f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
51.4f, 1.6f, 51.05f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
51.4f, 1.8f, 51.05f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
51.4f, 1.8f, 51.05f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
50.6f, 1.8f, 51.05f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
50.6f, 1.6f, 51.05f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,

50.6f, 1.6f, 50.95f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
51.4f, 1.6f, 50.95f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
51.4f, 1.8f, 50.95f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
51.4f, 1.8f, 50.95f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
50.6f, 1.8f, 50.95f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
50.6f, 1.6f, 50.95f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

50.6f, 1.8f, 51.05f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,
50.6f, 1.8f, 50.95f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
50.6f, 1.6f, 50.95f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
50.6f, 1.6f, 50.95f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
50.6f, 1.6f, 51.05f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,
50.6f, 1.8f, 51.05f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,

51.4f, 1.8f, 51.05f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,
51.4f, 1.8f, 50.95f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
51.4f, 1.6f, 50.95f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
51.4f, 1.6f, 50.95f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
51.4f, 1.6f, 51.05f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
51.4f, 1.8f, 51.05f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,

50.6f, 1.8f, 50.95f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,
51.4f, 1.8f, 50.95f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f,
51.4f, 1.8f, 51.05f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
51.4f, 1.8f, 51.05f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
50.6f, 1.8f, 51.05f,  0.0f, 0.0f,  0.0f, 1.0f,  0.0f,
50.6f, 1.8f, 50.95f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,

50.6f, 1.6f, 50.95f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f,
51.4f, 1.6f, 50.95f,  1.0f, 1.0f,  0.0f,-1.0f,  0.0f,
51.4f, 1.6f, 51.05f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
51.4f, 1.6f, 51.05f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
50.6f, 1.6f, 51.05f,  0.0f, 0.0f,  0.0f,-1.0f,  0.0f,
50.6f, 1.6f, 50.95f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f

    };

    unsigned int piece2VBO, piece2VAO;
    glGenVertexArrays(1, &piece2VAO);
    glGenBuffers(1, &piece2VBO);
    glBindVertexArray(piece2VAO);
    glBindBuffer(GL_ARRAY_BUFFER, piece2VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(piece2Vertices), piece2Vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float piece3Vertices[] = {
    -14.4f, 1.3f, -19.95f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
    -13.6f, 1.3f, -19.95f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,
    -13.6f, 1.5f, -19.95f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -13.6f, 1.5f, -19.95f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -14.4f, 1.5f, -19.95f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -14.4f, 1.3f, -19.95f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,

    -14.4f, 1.3f, -20.05f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
    -13.6f, 1.3f, -20.05f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
    -13.6f, 1.5f, -20.05f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -13.6f, 1.5f, -20.05f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -14.4f, 1.5f, -20.05f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -14.4f, 1.3f, -20.05f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

    -14.4f, 1.5f, -19.95f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -14.4f, 1.5f, -20.05f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -14.4f, 1.3f, -20.05f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -14.4f, 1.3f, -20.05f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -14.4f, 1.3f, -19.95f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -14.4f, 1.5f, -19.95f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,

    -13.6f, 1.5f, -19.95f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,
    -13.6f, 1.5f, -20.05f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
    -13.6f, 1.3f, -20.05f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
    -13.6f, 1.3f, -20.05f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
    -13.6f, 1.3f, -19.95f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
    -13.6f, 1.5f, -19.95f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,

    -14.4f, 1.5f, -20.05f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,
    -13.6f, 1.5f, -20.05f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f,
    -13.6f, 1.5f, -19.95f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
    -13.6f, 1.5f, -19.95f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
    -14.4f, 1.5f, -19.95f,  0.0f, 0.0f,  0.0f, 1.0f,  0.0f,
    -14.4f, 1.5f, -20.05f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,

    -14.4f, 1.3f, -20.05f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f,
    -13.6f, 1.3f, -20.05f,  1.0f, 1.0f,  0.0f,-1.0f,  0.0f,
    -13.6f, 1.3f, -19.95f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
    -13.6f, 1.3f, -19.95f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
    -14.4f, 1.3f, -19.95f,  0.0f, 0.0f,  0.0f,-1.0f,  0.0f,
    -14.4f, 1.3f, -20.05f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f
    };

    unsigned int piece3VBO, piece3VAO;
    glGenVertexArrays(1, &piece3VAO);
    glGenBuffers(1, &piece3VBO);
    glBindVertexArray(piece3VAO);
    glBindBuffer(GL_ARRAY_BUFFER, piece3VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(piece3Vertices), piece3Vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    float HUDVertices[] = {

    -0.95f,  0.80f,
    -0.75f,  0.80f,
    -0.75f,  0.95f,
    -0.75f,  0.95f,
    -0.95f,  0.95f,
    -0.95f,  0.80f,

    -0.65f,  0.80f,
    -0.45f,  0.80f,
    -0.45f,  0.95f,
    -0.45f,  0.95f,
    -0.65f,  0.95f,
    -0.65f,  0.80f,

    -0.35f,  0.80f,
    -0.15f,  0.80f,
    -0.15f,  0.95f,
    -0.15f,  0.95f,
    -0.35f,  0.95f,
    -0.35f,  0.80f

    };

    unsigned int HUDVBO, HUDVAO;
    glGenVertexArrays(1, &HUDVAO);
    glGenBuffers(1, &HUDVBO);
    glBindVertexArray(HUDVAO);
    glBindBuffer(GL_ARRAY_BUFFER, HUDVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(HUDVertices), HUDVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float sunVertices[] = {
    14.75f, 69.5f,  14.75f,
    15.25f, 69.5f,  14.75f,
    15.25f, 70.0f,  14.75f,
    15.25f, 70.0f,  14.75f,
    14.75f, 70.0f,  14.75f,
    14.75f, 69.5f,  14.75f,

    14.75f, 69.5f,  15.25f,
    15.25f, 69.5f,  15.25f,
    15.25f, 70.0f,  15.25f,
    15.25f, 70.0f,  15.25f,
    14.75f, 70.0f,  15.25f,
    14.75f, 69.5f,  15.25f,

    14.75f, 70.0f,  14.75f,
    14.75f, 70.0f,  15.25f,
    14.75f, 69.5f,  15.25f,
    14.75f, 69.5f,  15.25f,
    14.75f, 69.5f,  14.75f,
    14.75f, 70.0f,  14.75f,

    15.25f, 70.0f,  14.75f,
    15.25f, 70.0f,  15.25f,
    15.25f, 69.5f,  15.25f,
    15.25f, 69.5f,  15.25f,
    15.25f, 69.5f,  14.75f,
    15.25f, 70.0f,  14.75f,

    14.75f, 70.0f,  15.25f,
    15.25f, 70.0f,  15.25f,
    15.25f, 70.0f,  14.75f,
    15.25f, 70.0f,  14.75f,
    14.75f, 70.0f,  14.75f,
    14.75f, 70.0f,  15.25f,

    14.75f, 69.5f,  15.25f,
    15.25f, 69.5f,  15.25f,
    15.25f, 69.5f,  14.75f,
    15.25f, 69.5f,  14.75f,
    14.75f, 69.5f,  14.75f,
    14.75f, 69.5f,  15.25f
    };

    unsigned int sunVBO, sunVAO;
    glGenVertexArrays(1, &sunVAO);
    glGenBuffers(1, &sunVBO);
    glBindVertexArray(sunVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sunVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sunVertices), sunVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float treecenteralVertices[] = {
    -0.2f, -0.7f,  0.2f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.2f, -0.7f,  0.2f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.2f,  2.0f,  0.2f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
     0.2f,  2.0f,  0.2f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.2f,  2.0f,  0.2f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.2f, -0.7f,  0.2f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,

    -0.2f, -0.7f, -0.2f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.2f, -0.7f, -0.2f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.2f,  2.0f, -0.2f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     0.2f,  2.0f, -0.2f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.2f,  2.0f, -0.2f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.2f, -0.7f, -0.2f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

    -0.2f,  2.0f,  0.2f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.2f,  2.0f, -0.2f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.2f, -0.7f, -0.2f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.2f, -0.7f, -0.2f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.2f, -0.7f,  0.2f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.2f,  2.0f,  0.2f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,

     0.2f,  2.0f,  0.2f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.2f,  2.0f, -0.2f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.2f, -0.7f, -0.2f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.2f, -0.7f, -0.2f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.2f, -0.7f,  0.2f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.2f,  2.0f,  0.2f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,

     -0.2f,  2.0f, -0.2f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,
      0.2f,  2.0f, -0.2f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f,
      0.2f,  2.0f,  0.2f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
      0.2f,  2.0f,  0.2f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
     -0.2f,  2.0f,  0.2f,  0.0f, 0.0f,  0.0f, 1.0f,  0.0f,
     -0.2f,  2.0f, -0.2f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,

     -0.2f, -0.7f, -0.2f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f,
      0.2f, -0.7f, -0.2f,  1.0f, 1.0f,  0.0f,-1.0f,  0.0f,
      0.2f, -0.7f,  0.2f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
      0.2f, -0.7f,  0.2f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
     -0.2f, -0.7f,  0.2f,  0.0f, 0.0f,  0.0f,-1.0f,  0.0f,
     -0.2f, -0.7f, -0.2f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f
    };

    glm::vec3 treeVertices[] = {
        glm::vec3(18.0f, 0.0f, 18.0f),
        glm::vec3(-18.0f, 0.0f, 18.0f),
        glm::vec3(18.0f, 0.0f,-18.0f),
        glm::vec3(-18.0f, 0.0f,-18.0f),
        glm::vec3(0.0f, 0.0f, 20.0f),
        glm::vec3(0.0f, 0.0f,-20.0f),
        glm::vec3(20.0f, 0.0f, 0.0f),
        glm::vec3(-20.0f, 0.0f, 0.0f),
        glm::vec3(10.0f, 0.0f,  22.0f),
        glm::vec3(-10.0f, 0.0f,  22.0f),
        glm::vec3(10.0f, 0.0f, -22.0f),
        glm::vec3(-10.0f, 0.0f, -22.0f),
        glm::vec3(22.0f, 0.0f,  10.0f),
        glm::vec3(22.0f, 0.0f, -10.0f),
        glm::vec3(-22.0f, 0.0f,  10.0f),
        glm::vec3(-22.0f, 0.0f, -10.0f),
        glm::vec3(8.0f, 0.0f,   8.0f),
        glm::vec3(-8.0f, 0.0f,   8.0f),
        glm::vec3(8.0f, 0.0f,  -8.0f),
        glm::vec3(-8.0f, 0.0f,  -8.0f),
        glm::vec3(12.0f, 0.0f,   0.0f),
        glm::vec3(-12.0f, 0.0f,   0.0f),
        glm::vec3(0.0f, 0.0f,  12.0f),
        glm::vec3(0.0f, 0.0f, -12.0f),
        glm::vec3(5.0f, 0.0f, -9.0f),
        glm::vec3(5.0f, 0.0f, -9.0f),
        glm::vec3(5.0f, 0.0f,  9.0f),
        glm::vec3(2.0f, 0.0f, -5.0f)
    };

    unsigned int treeVAO, treeVBO;
    glGenVertexArrays(1, &treeVAO);
    glGenBuffers(1, &treeVBO);
    glBindVertexArray(treeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, treeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(treecenteralVertices), treecenteralVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    float leavescentralVertices[] = {
    -0.8f,  2.0f,  0.8f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.8f,  2.0f,  0.8f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.8f,  3.5f,  0.8f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
     0.8f,  3.5f,  0.8f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.8f,  3.5f,  0.8f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.8f,  2.0f,  0.8f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,

    -0.8f,  2.0f, -0.8f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.8f,  2.0f, -0.8f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.8f,  3.5f, -0.8f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     0.8f,  3.5f, -0.8f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.8f,  3.5f, -0.8f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.8f,  2.0f, -0.8f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

    -0.8f,  3.5f,  0.8f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.8f,  3.5f, -0.8f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.8f,  2.0f, -0.8f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.8f,  2.0f, -0.8f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.8f,  2.0f,  0.8f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.8f,  3.5f,  0.8f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,

     0.8f,  3.5f,  0.8f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.8f,  3.5f, -0.8f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.8f,  2.0f, -0.8f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.8f,  2.0f, -0.8f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.8f,  2.0f,  0.8f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.8f,  3.5f,  0.8f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,

    -0.8f,  3.5f, -0.8f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,
     0.8f,  3.5f, -0.8f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f,
     0.8f,  3.5f,  0.8f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
     0.8f,  3.5f,  0.8f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
    -0.8f,  3.5f,  0.8f,  0.0f, 0.0f,  0.0f, 1.0f,  0.0f,
    -0.8f,  3.5f, -0.8f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,

    -0.8f,  2.0f, -0.8f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f,
     0.8f,  2.0f, -0.8f,  1.0f, 1.0f,  0.0f,-1.0f,  0.0f,
     0.8f,  2.0f,  0.8f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
     0.8f,  2.0f,  0.8f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
    -0.8f,  2.0f,  0.8f,  0.0f, 0.0f,  0.0f,-1.0f,  0.0f,
    -0.8f,  2.0f, -0.8f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f
    };

    glm::vec3 leavesVertices[] = {
       glm::vec3(18.0f, 0.0f, 18.0f),
        glm::vec3(-18.0f, 0.0f, 18.0f),
        glm::vec3(18.0f, 0.0f,-18.0f),
        glm::vec3(-18.0f, 0.0f,-18.0f),
        glm::vec3(0.0f, 0.0f, 20.0f),
        glm::vec3(0.0f, 0.0f,-20.0f),
        glm::vec3(20.0f, 0.0f, 0.0f),
        glm::vec3(-20.0f, 0.0f, 0.0f),
        glm::vec3(10.0f, 0.0f,  22.0f),
        glm::vec3(-10.0f, 0.0f,  22.0f),
        glm::vec3(10.0f, 0.0f, -22.0f),
        glm::vec3(-10.0f, 0.0f, -22.0f),
        glm::vec3(22.0f, 0.0f,  10.0f),
        glm::vec3(22.0f, 0.0f, -10.0f),
        glm::vec3(-22.0f, 0.0f,  10.0f),
        glm::vec3(-22.0f, 0.0f, -10.0f),
        glm::vec3(8.0f, 0.0f,   8.0f),
        glm::vec3(-8.0f, 0.0f,   8.0f),
        glm::vec3(8.0f, 0.0f,  -8.0f),
        glm::vec3(-8.0f, 0.0f,  -8.0f),
        glm::vec3(12.0f, 0.0f,   0.0f),
        glm::vec3(-12.0f, 0.0f,   0.0f),
        glm::vec3(0.0f, 0.0f,  12.0f),
        glm::vec3(0.0f, 0.0f, -12.0f),
        glm::vec3(5.0f, 0.0f, -9.0f),
        glm::vec3(5.0f, 0.0f, -9.0f),
        glm::vec3(5.0f, 0.0f,  9.0f),
        glm::vec3(2.0f, 0.0f, -5.0f)
    };

    unsigned int leavesVAO, leavesVBO;
    glGenVertexArrays(1, &leavesVAO);
    glGenBuffers(1, &leavesVBO);
    glBindVertexArray(leavesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, leavesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(leavescentralVertices), leavescentralVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    float rockcenteralVertices[] = {
    -0.5f, -0.3f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.5f, -0.3f,  0.5f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -0.5f, -0.3f,  0.5f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,

    -0.5f, -0.3f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.5f, -0.3f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -0.5f, -0.3f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
    -0.5f, -0.3f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.5f, -0.3f, -0.5f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.5f, -0.3f,  0.5f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
     0.5f, -0.3f, -0.5f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.5f, -0.3f, -0.5f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.5f, -0.3f,  0.5f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,

     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,
      0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  0.0f, 1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f,
     -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  0.0f, 1.0f,  0.0f,
     -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,  0.0f, 1.0f,  0.0f,

     -0.5f, -0.3f, -0.5f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f,
      0.5f, -0.3f, -0.5f,  1.0f, 1.0f,  0.0f,-1.0f,  0.0f,
      0.5f, -0.3f,  0.5f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
      0.5f, -0.3f,  0.5f,  1.0f, 0.0f,  0.0f,-1.0f,  0.0f,
     -0.5f, -0.3f,  0.5f,  0.0f, 0.0f,  0.0f,-1.0f,  0.0f,
     -0.5f, -0.3f, -0.5f,  0.0f, 1.0f,  0.0f,-1.0f,  0.0f
    };

    glm::vec3 rockVertices[] = {
        glm::vec3(10.0f, 0.0f, 5.0f),
        glm::vec3(-10.0f, 0.0f, 5.0f),
        glm::vec3(10.0f, 0.0f,-5.0f),
        glm::vec3(-10.0f, 0.0f,-5.0f),
        glm::vec3(5.0f, 0.0f, 15.0f),
        glm::vec3(-5.0f, 0.0f, 15.0f),
        glm::vec3(5.0f, 0.0f, -15.0f),
        glm::vec3(-5.0f, 0.0f, -15.0f),
        glm::vec3(15.0f, 0.0f,  10.0f),
        glm::vec3(-15.0f, 0.0f,  10.0f),
        glm::vec3(15.0f, 0.0f, -10.0f),
        glm::vec3(-15.0f, 0.0f, -10.0f),
        glm::vec3(8.0f, 0.0f,  20.0f),
        glm::vec3(-8.0f, 0.0f,  20.0f),
        glm::vec3(8.0f, 0.0f, -20.0f),
        glm::vec3(-8.0f, 0.0f, -20.0f) ,
        glm::vec3(6.0f, 0.0f,   3.0f),
        glm::vec3(-6.0f, 0.0f,   3.0f),
        glm::vec3(6.0f, 0.0f,  -3.0f),
        glm::vec3(-6.0f, 0.0f,  -3.0f),
        glm::vec3(3.0f, 0.0f,  10.0f),
        glm::vec3(-3.0f, 0.0f,  10.0f),
        glm::vec3(3.0f, 0.0f, -10.0f),
        glm::vec3(-3.0f, 0.0f, -10.0f),
    };

    unsigned int rockVAO, rockVBO;
    glGenVertexArrays(1, &rockVAO);
    glGenBuffers(1, &rockVBO);
    glBindVertexArray(rockVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rockVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rockcenteralVertices), rockcenteralVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    float boatVertices[] = {
     2.0f,  0.5f,  1.0f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     2.0f,  0.5f, -1.0f,  0.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     2.0f, -0.5f,  0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,


    -2.0f,  0.5f,  1.0f,  1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -2.0f,  0.5f, -1.0f,  0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
    -2.0f, -0.5f,  0.0f,  0.5f, 0.0f, -1.0f, 0.0f, 0.0f,


     2.0f,  0.5f,  1.0f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -2.0f,  0.5f,  1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -2.0f, -0.5f,  0.0f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,

     2.0f,  0.5f,  1.0f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -2.0f, -0.5f,  0.0f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     2.0f, -0.5f,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,


     2.0f,  0.5f, -1.0f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -2.0f,  0.5f, -1.0f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -2.0f, -0.5f,  0.0f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

     2.0f,  0.5f, -1.0f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -2.0f, -0.5f,  0.0f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     2.0f, -0.5f,  0.0f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,


     2.0f,  0.5f,  1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
    -2.0f,  0.5f,  1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
    -2.0f,  0.5f, -1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,

    -2.0f,  0.5f, -1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
     2.0f,  0.5f, -1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
     2.0f,  0.5f,  1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };
    unsigned int boatVAO, boatVBO;
    glGenVertexArrays(1, &boatVAO);
    glGenBuffers(1, &boatVBO);
    glBindVertexArray(boatVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boatVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boatVertices), boatVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    float mastVertices[] = {

            -0.1f,  0.5f,  0.1f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
             0.1f,  0.5f,  0.1f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,
             0.5f,  2.5f,  0.1f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
             0.5f,  2.5f,  0.1f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
             0.3f,  2.5f,  0.1f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
            -0.1f,  0.5f,  0.1f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,

            -0.1f,  0.5f, -0.1f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
             0.1f,  0.5f, -0.1f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
             0.5f,  2.5f, -0.1f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
             0.5f,  2.5f, -0.1f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
             0.3f,  2.5f, -0.1f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
            -0.1f,  0.5f, -0.1f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

            -0.1f,  0.5f,  0.1f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,
            -0.1f,  0.5f, -0.1f,  0.0f, 0.0f, -1.0f, 0.0f,  0.0f,
             0.3f,  2.5f, -0.1f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
             0.3f,  2.5f, -0.1f,  0.0f, 1.0f, -1.0f, 0.0f,  0.0f,
             0.3f,  2.5f,  0.1f,  1.0f, 1.0f, -1.0f, 0.0f,  0.0f,
            -0.1f,  0.5f,  0.1f,  1.0f, 0.0f, -1.0f, 0.0f,  0.0f,

             0.1f,  0.5f,  0.1f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
             0.1f,  0.5f, -0.1f,  0.0f, 0.0f,  1.0f, 0.0f,  0.0f,
             0.5f,  2.5f, -0.1f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
             0.5f,  2.5f, -0.1f,  0.0f, 1.0f,  1.0f, 0.0f,  0.0f,
             0.5f,  2.5f,  0.1f,  1.0f, 1.0f,  1.0f, 0.0f,  0.0f,
             0.1f,  0.5f,  0.1f,  1.0f, 0.0f,  1.0f, 0.0f,  0.0f,
    };

    unsigned int mastVAO, mastVBO;
    glGenVertexArrays(1, &mastVAO);
    glGenBuffers(1, &mastVBO);
    glBindVertexArray(mastVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mastVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mastVertices), mastVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    float houseWallVertices[] = {
    -1.5f, 0.0f, -1.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     1.5f, 0.0f, -1.5f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     1.5f, 2.5f, -1.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     1.5f, 2.5f, -1.5f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -1.5f, 2.5f, -1.5f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -1.5f, 0.0f, -1.5f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

     1.5f, 0.0f, -1.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     1.5f, 0.0f,  1.5f,  1.0f, 0.0f,  1.0f, 0.0f, 0.0f,
     1.5f, 2.5f,  1.5f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     1.5f, 2.5f,  1.5f,  1.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     1.5f, 2.5f, -1.5f,  0.0f, 1.0f,  1.0f, 0.0f, 0.0f,
     1.5f, 0.0f, -1.5f,  0.0f, 0.0f,  1.0f, 0.0f, 0.0f,

     -1.5f, 0.0f,  1.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
     -1.5f, 0.0f, -1.5f,  1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
     -1.5f, 2.5f, -1.5f,  1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
     -1.5f, 2.5f, -1.5f,  1.0f, 1.0f, -1.0f, 0.0f, 0.0f,
     -1.5f, 2.5f,  1.5f,  0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
     -1.5f, 0.0f,  1.5f,  0.0f, 0.0f, -1.0f, 0.0f, 0.0f
    };

    unsigned int houseWallVAO, houseWallVBO;
    glGenVertexArrays(1, &houseWallVAO);
    glGenBuffers(1, &houseWallVBO);
    glBindVertexArray(houseWallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, houseWallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(houseWallVertices), houseWallVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);


    float frontWallVertices[] = {
    -1.5f, 0.0f,  1.5f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
    -0.5f, 0.0f,  1.5f,  0.3f, 0.0f,  0.0f, 0.0f, 1.0f,
    -0.5f, 2.5f,  1.5f,  0.3f, 1.0f,  0.0f, 0.0f, 1.0f,
    -0.5f, 2.5f,  1.5f,  0.3f, 1.0f,  0.0f, 0.0f, 1.0f,
    -1.5f, 2.5f,  1.5f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,
    -1.5f, 0.0f,  1.5f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,

     0.5f, 0.0f,  1.5f,  0.7f, 0.0f,  0.0f, 0.0f, 1.0f,
     1.5f, 0.0f,  1.5f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,
     1.5f, 2.5f,  1.5f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
     1.5f, 2.5f,  1.5f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
     0.5f, 2.5f,  1.5f,  0.7f, 1.0f,  0.0f, 0.0f, 1.0f,
     0.5f, 0.0f,  1.5f,  0.7f, 0.0f,  0.0f, 0.0f, 1.0f
    };

    unsigned int frontWallVAO, frontWallVBO;
    glGenVertexArrays(1, &frontWallVAO);
    glGenBuffers(1, &frontWallVBO);
    glBindVertexArray(frontWallVAO);
    glBindBuffer(GL_ARRAY_BUFFER, frontWallVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(frontWallVertices), frontWallVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    float roofVertices[] = {
     0.0f, 4.0f,  1.5f,  0.5f, 1.0f,  0.7f, 0.7f, 0.0f,
     1.5f, 2.5f,  1.5f,  1.0f, 0.0f,  0.7f, 0.7f, 0.0f,
     1.5f, 2.5f, -1.5f,  0.0f, 0.0f,  0.7f, 0.7f, 0.0f,
     1.5f, 2.5f, -1.5f,  0.0f, 0.0f,  0.7f, 0.7f, 0.0f,
     0.0f, 4.0f, -1.5f,  0.5f, 1.0f,  0.7f, 0.7f, 0.0f,
     0.0f, 4.0f,  1.5f,  0.5f, 1.0f,  0.7f, 0.7f, 0.0f,

     0.0f, 4.0f,  1.5f,  0.5f, 1.0f, -0.7f, 0.7f, 0.0f,
    -1.5f, 2.5f,  1.5f,  1.0f, 0.0f, -0.7f, 0.7f, 0.0f,
    -1.5f, 2.5f, -1.5f,  0.0f, 0.0f, -0.7f, 0.7f, 0.0f,
    -1.5f, 2.5f, -1.5f,  0.0f, 0.0f, -0.7f, 0.7f, 0.0f,
     0.0f, 4.0f, -1.5f,  0.5f, 1.0f, -0.7f, 0.7f, 0.0f,
     0.0f, 4.0f,  1.5f,  0.5f, 1.0f, -0.7f, 0.7f, 0.0f,

    -1.5f, 2.5f,  1.5f,  0.0f, 0.0f,  0.0f, 0.5f, 0.7f,
     1.5f, 2.5f,  1.5f,  1.0f, 0.0f,  0.0f, 0.5f, 0.7f,
     0.0f, 4.0f,  1.5f,  0.5f, 1.0f,  0.0f, 0.5f, 0.7f,

    -1.5f, 2.5f, -1.5f,  0.0f, 0.0f,  0.0f, 0.7f,-0.7f,
     1.5f, 2.5f, -1.5f,  1.0f, 0.0f,  0.0f, 0.7f,-0.7f,
     0.0f, 4.0f, -1.5f,  0.5f, 1.0f,  0.0f, 0.7f,-0.7f
    };

    unsigned int roofVAO, roofVBO;
    glGenVertexArrays(1, &roofVAO);
    glGenBuffers(1, &roofVBO);
    glBindVertexArray(roofVAO);
    glBindBuffer(GL_ARRAY_BUFFER, roofVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(roofVertices), roofVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    glm::vec3 platformPosition[] = {
    glm::vec3(28.0f, 0.5f, 28.0f),
    glm::vec3(33.0f, 0.5f, 33.0f),
    glm::vec3(39.0f, 0.5f, 39.0f),
    glm::vec3(45.0f, 0.5f, 45.0f),
    glm::vec3(51.0f, 0.5f, 51.0f)
    };

    float platformVertices[] = {
    -1.0f, 0.0f, -1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
     1.0f, 0.0f, -1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
     1.0f, 0.0f,  1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
     1.0f, 0.0f,  1.0f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
    -1.0f, 0.0f,  1.0f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
    -1.0f, 0.0f, -1.0f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,

    -1.0f, -0.2f, -1.0f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     1.0f, -0.2f, -1.0f,  1.0f, 0.0f,  0.0f, 0.0f, -1.0f,
     1.0f,  0.0f, -1.0f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
     1.0f,  0.0f, -1.0f,  1.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -1.0f,  0.0f, -1.0f,  0.0f, 1.0f,  0.0f, 0.0f, -1.0f,
    -1.0f, -0.2f, -1.0f,  0.0f, 0.0f,  0.0f, 0.0f, -1.0f,

    -1.0f, -0.2f,  1.0f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     1.0f, -0.2f,  1.0f,  1.0f, 0.0f,  0.0f, 0.0f,  1.0f,
     1.0f,  0.0f,  1.0f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
     1.0f,  0.0f,  1.0f,  1.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -1.0f,  0.0f,  1.0f,  0.0f, 1.0f,  0.0f, 0.0f,  1.0f,
    -1.0f, -0.2f,  1.0f,  0.0f, 0.0f,  0.0f, 0.0f,  1.0f
    };

    unsigned int platformVAO, platformVBO;
    glGenVertexArrays(1, &platformVAO);
    glGenBuffers(1, &platformVBO);
    glBindVertexArray(platformVAO);
    glBindBuffer(GL_ARRAY_BUFFER, platformVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(platformVertices), platformVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(2);

    float boxVertices[] = {

        0,0,0,  1,0,0,
        1,0,0,  1,0,1,
        1,0,1,  0,0,1,
        0,0,1,  0,0,0,


        0,1,0,  1,1,0,
        1,1,0,  1,1,1,
        1,1,1,  0,1,1,
        0,1,1,  0,1,0,


        0,0,0,  0,1,0,
        1,0,0,  1,1,0,
        1,0,1,  1,1,1,
        0,0,1,  0,1,1
    };

    unsigned int boxVAO, boxVBO;
    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);

    glBindVertexArray(boxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boxVertices), boxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    AABB treeBoxes[] = {
    { glm::vec3(-0.2f,  0.5f, -12.2f), glm::vec3(0.2f,  2.0f, -11.8f) },
    { glm::vec3(-0.2f,  0.5f,  11.8f), glm::vec3(0.2f,  2.0f,  12.2f) },
    { glm::vec3(17.8f, 0.5f, -18.2f), glm::vec3(18.2f,  2.0f, -17.8f) },
    { glm::vec3(-18.2f, 0.5f,  17.8f), glm::vec3(-17.8f, 2.0f,  18.2f) },
    { glm::vec3(7.8f, 0.5f,   7.8f), glm::vec3(8.2f,  2.0f,   8.2f) },
    { glm::vec3(4.8f, 0.5f,   8.8f), glm::vec3(5.2f,  2.0f,   9.2f) },
    { glm::vec3(-0.2f, 0.5f,  19.8f), glm::vec3(0.2f,  2.0f,  20.2f) },
    { glm::vec3(-10.2f, 0.5f,  21.8f), glm::vec3(-9.8f,  2.0f,  22.2f) },
    { glm::vec3(9.8f, 0.5f,  21.8f), glm::vec3(10.2f,  2.0f,  22.2f) },
    { glm::vec3(17.8f, 0.5f,  17.8f), glm::vec3(18.2f,  2.0f,  18.2f) },
    { glm::vec3(21.8f, 0.5f,   9.8f), glm::vec3(22.2f,  2.0f,  10.2f) },
    { glm::vec3(-22.2f, 0.5f,   9.8f), glm::vec3(-21.8f, 2.0f,  10.2f) },
    { glm::vec3(-18.2f, 0.5f, -18.2f), glm::vec3(-17.8f, 2.0f, -17.8f) },
    { glm::vec3(-10.2f, 0.5f, -22.2f), glm::vec3(-9.8f,  2.0f, -21.8f) },
    { glm::vec3(9.8f, 0.5f, -22.2f), glm::vec3(10.2f,  2.0f, -21.8f) },
    { glm::vec3(-0.2f, 0.5f, -20.2f), glm::vec3(0.2f,  2.0f, -19.8f) },
    { glm::vec3(-8.2f, 0.5f,  -8.2f), glm::vec3(-7.8f,  2.0f,  -7.8f) },
    { glm::vec3(-22.2f, 0.5f, -10.2f), glm::vec3(-21.8f, 2.0f,  -9.8f) },
    { glm::vec3(21.8f, 0.5f, -10.2f), glm::vec3(22.2f,  2.0f,  -9.8f) },
    { glm::vec3(19.8f, 0.5f,  -0.2f), glm::vec3(20.2f,  2.0f,   0.2f) },
    { glm::vec3(-20.2f, 0.5f,  -0.2f), glm::vec3(-19.8f, 2.0f,   0.2f) },
    { glm::vec3(-12.2f, 0.5f,  -0.2f), glm::vec3(-11.8f, 2.0f,   0.2f) },
    { glm::vec3(-8.2f, 0.5f,   7.8f), glm::vec3(-7.8f,  2.0f,   8.2f) },
    { glm::vec3(7.8f, 0.5f,  -8.2f), glm::vec3(8.2f,  2.0f,  -7.8f) },
    { glm::vec3(1.8f, 0.5f,  -5.2f), glm::vec3(2.2f,  2.0f,  -4.8f) }
    };

    AABB wallBoxes[] = {
    {glm::vec3(-16.0f,0.5f, -20.79f), glm::vec3(-15.9f, 3.0, -20.77f)},
    {glm::vec3(-15.95f,0.5f, -20.77f), glm::vec3(-15.9f, 3.0, -20.63f)},
    {glm::vec3(-15.90f,0.5f, -20.71f), glm::vec3(-15.85f, 3.0, -20.50f)},
    {glm::vec3(-15.85f,0.5f, -20.50f), glm::vec3(-15.80f, 3.0, -20.29f)},
    {glm::vec3(-15.75f,0.5f, -20.29f), glm::vec3(-15.70f, 3.0, -20.08f)},
    {glm::vec3(-15.65f,0.5f, -20.08f), glm::vec3(-15.6f, 3.0, -19.87f)},
    {glm::vec3(-15.55f,0.5f, -19.87f), glm::vec3(-15.5f, 3.0, -19.66f)},
    {glm::vec3(-15.45f,0.5f, -19.66f), glm::vec3(-15.4f, 3.0, -19.45f)},
    {glm::vec3(-15.35f,0.5f, -19.45f), glm::vec3(-15.3f, 3.0, -19.24f)},
    {glm::vec3(-15.25f,0.5f, -19.24f), glm::vec3(-15.2f, 3.0, -19.03f)},
    {glm::vec3(-15.15f,0.5f, -19.03f), glm::vec3(-15.1f, 3.0, -18.82f)},
    {glm::vec3(-15.05f,0.5f, -18.82f), glm::vec3(-15.0f, 3.0, -18.61f)},
    {glm::vec3(-14.95f,0.5f, -18.61f), glm::vec3(-14.9f, 3.0, -18.40f)},
    {glm::vec3(-14.85f,0.5f, -18.40f), glm::vec3(-14.8f, 3.0, -18.19f)},
    {glm::vec3(-14.75f,0.5f, -18.19f), glm::vec3(-14.7f, 3.0, -18.058f)},
    {glm::vec3(-14.685f,0.5f, -18.088f), glm::vec3(-14.55f, 3.0, -18.128f)},
    {glm::vec3(-14.55f,0.5f, -18.18f), glm::vec3(-14.45f, 3.0, -18.128f)},
    {glm::vec3(-14.4f,0.5f,-18.2f), glm::vec3(-14.3f, 3.0, -18.148f)},
    {glm::vec3(-14.2f,0.5f,-18.295f), glm::vec3(-14.1f, 3.0, -18.218f)},
    {glm::vec3(-14.0f,0.5f,-18.395f), glm::vec3(-13.9f, 3.0, -18.348f)},
    {glm::vec3(-13.25f,0.5f, -21.974f), glm::vec3(-13.15f, 3.0, -21.912f)},
    {glm::vec3(-13.175f,0.5f, -21.812f), glm::vec3(-13.081f, 3.0, -21.650f)},
    {glm::vec3(-13.175f,0.5f, -21.912f), glm::vec3(-13.091f, 3.0, -21.750f)},
    {glm::vec3(-13.125f,0.5f, -21.712f), glm::vec3(-13.021f, 3.0, -21.550f)},
    {glm::vec3(-13.075f,0.5f, -21.612f), glm::vec3(-12.951f, 3.0, -21.450f)},
    {glm::vec3(-13.000f,0.5f, -21.512f), glm::vec3(-12.881f, 3.0, -21.350f)},
    {glm::vec3(-12.98f,0.5f, -21.412f), glm::vec3(-12.841f, 3.0, -21.250f)},
    {glm::vec3(-12.95f,0.5f, -21.312f), glm::vec3(-12.851f, 3.0, -21.150f)},
    {glm::vec3(-12.9f,0.5f, -21.212f), glm::vec3(-12.841f, 3.0, -21.050f)},
    {glm::vec3(-12.859f,0.5f, -21.112f), glm::vec3(-12.751f, 3.0, -20.950f)},
    {glm::vec3(-12.809f,0.5f, -21.012f), glm::vec3(-12.661f, 3.0, -20.850f)},
    {glm::vec3(-12.759f,0.5f, -20.912f), glm::vec3(-12.691f, 3.0, -20.750f)},
    {glm::vec3(-12.709f,0.5f, -20.812f), glm::vec3(-12.641f, 3.0, -20.650f)},
    {glm::vec3(-12.659f,0.5f, -20.712f), glm::vec3(-12.591f, 3.0, -20.550f)},
    {glm::vec3(-12.609f,0.5f, -20.612f), glm::vec3(-12.541f, 3.0, -20.450f)},
    {glm::vec3(-12.559f,0.5f, -20.512f), glm::vec3(-12.491f, 3.0, -20.350f)},
    {glm::vec3(-12.509f,0.5f, -20.412f), glm::vec3(-12.441f, 3.0, -20.250f)},
    {glm::vec3(-12.459f,0.5f, -20.312f), glm::vec3(-12.391f, 3.0, -20.150f)},
    {glm::vec3(-12.409f,0.5f, -20.212f), glm::vec3(-12.341f, 3.0, -20.050f)},
    {glm::vec3(-12.359f,0.5f, -20.112f), glm::vec3(-12.291f, 3.0, -19.950f)},
    {glm::vec3(-12.309f,0.5f, -20.012f), glm::vec3(-12.241f, 3.0, -19.850f)},
    {glm::vec3(-12.259f,0.5f, -19.912f), glm::vec3(-12.191f, 3.0, -19.750f)},
    {glm::vec3(-12.239f,0.5f, -19.812f), glm::vec3(-12.141f, 3.0, -19.650f)},
    {glm::vec3(-12.199f,0.5f, -19.712f), glm::vec3(-12.151f, 3.0, -19.550f)},
    {glm::vec3(-12.189f,0.5f, -19.612f), glm::vec3(-12.141f, 3.0, -19.450f)},
    {glm::vec3(-12.179f,0.5f, -19.512f), glm::vec3(-12.131f, 3.0, -19.350f)},
    {glm::vec3(-12.169f,0.5f, -19.412f), glm::vec3(-12.121f, 3.0, -19.250f)},
    {glm::vec3(-12.159f,0.5f, -19.312f), glm::vec3(-12.111f, 3.0, -19.150f)},
    {glm::vec3(-12.259f,0.5f, -19.212f), glm::vec3(-12.211f, 3.0, -19.120f)},
    {glm::vec3(-12.309f,0.5f, -19.152f), glm::vec3(-12.251f, 3.0, -19.080f)},
    {glm::vec3(-12.359f,0.5f, -19.102f), glm::vec3(-12.251f, 3.0, -19.050f)},
    {glm::vec3(-12.409f,0.5f, -19.102f), glm::vec3(-12.291f, 3.0, -19.010f)},
    {glm::vec3(-12.459f,0.5f, -19.142f), glm::vec3(-12.241f, 3.0, -19.050f)},
    {glm::vec3(-12.509f,0.5f, -19.102f), glm::vec3(-12.291f, 3.0, -19.010f)},
    {glm::vec3(-12.559f,0.5f, -19.052f), glm::vec3(-12.341f, 3.0, -18.960f)},
    {glm::vec3(-12.609f,0.5f, -19.048f), glm::vec3(-12.391f, 3.0, -18.940f)},
    {glm::vec3(-12.659f,0.5f, -19.034f), glm::vec3(-12.441f, 3.0, -18.920f)},
    {glm::vec3(-12.709f,0.5f, -19.02f), glm::vec3(-12.491f, 3.0, -18.920f)},
    {glm::vec3(-12.909f,0.5f, -18.898f), glm::vec3(-12.391f, 3.0, -18.940f)},
    {glm::vec3(-15.9f,0.5f, -20.865f), glm::vec3(-15.8f, 3.0, -20.785f)},
    {glm::vec3(-15.8f,0.5f, -20.939f), glm::vec3(-15.7f, 3.0, -20.859f)},
    {glm::vec3(-15.7f,0.5f, -20.939f), glm::vec3(-15.6f, 3.0, -20.913f)},
    {glm::vec3(-15.6f,0.5f, -20.959f), glm::vec3(-15.5f, 3.0, -20.933f)},
    {glm::vec3(-15.5f,0.5f, -21.029f), glm::vec3(-15.4f, 3.0, -20.953f)},
    {glm::vec3(-15.4f,0.5f, -21.079f), glm::vec3(-15.3f, 3.0, -20.993f)},
    {glm::vec3(-15.3f,0.5f, -21.084f), glm::vec3(-15.2f, 3.0, -20.999f)},
    {glm::vec3(-15.2f,0.5f, -21.209f), glm::vec3(-15.0f, 3.0, -21.143f)},
    {glm::vec3(-15.0f,0.5f, -21.349f), glm::vec3(-14.8f, 3.0, -21.273f)},
    {glm::vec3(-14.8f,0.5f, -21.419f), glm::vec3(-14.6f, 3.0, -21.353f)},
    {glm::vec3(-14.6f,0.5f, -21.489f), glm::vec3(-14.4f, 3.0, -21.423f)},
    {glm::vec3(-14.4f,0.5f, -21.559f), glm::vec3(-14.2f, 3.0, -21.493f)},
    {glm::vec3(-14.2f,0.5f, -21.599f), glm::vec3(-14.0f, 3.0, -21.533f)},
    {glm::vec3(-14.0f,0.5f, -21.739f), glm::vec3(-13.8f, 3.0, -21.673f)},
    {glm::vec3(-13.8f,0.5f, -21.799f), glm::vec3(-13.6f, 3.0, -21.733f)},
    {glm::vec3(-13.6f,0.5f, -21.859f), glm::vec3(-13.4f, 3.0, -21.793f)},
    {glm::vec3(-13.4f,0.5f, -21.959f), glm::vec3(-13.2f, 3.0, -21.833f)}
    };

    unsigned int grasstexture;
    glGenTextures(1, &grasstexture);
    glBindTexture(GL_TEXTURE_2D, grasstexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("Assets/grass.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    unsigned int sandtexture, rocktexture, leavestexture, oceantexture, treetexture, piecetexture;
    glGenTextures(1, &sandtexture);
    glBindTexture(GL_TEXTURE_2D, sandtexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("Assets/sand.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glGenTextures(1, &rocktexture);
    glBindTexture(GL_TEXTURE_2D, rocktexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("Assets/rock.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glGenTextures(1, &leavestexture);
    glBindTexture(GL_TEXTURE_2D, leavestexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    data = stbi_load("Assets/leaves.jpg", &width, &height, &nrChannels, 0);

    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glGenTextures(1, &oceantexture);
    glBindTexture(GL_TEXTURE_2D, oceantexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("Assets/ocean.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glGenTextures(1, &treetexture);
    glBindTexture(GL_TEXTURE_2D, treetexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("Assets/wood.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glGenTextures(1, &piecetexture);
    glBindTexture(GL_TEXTURE_2D, piecetexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("Assets/woodpiece.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);


    AABB boat{
    glm::vec3(-1.75f,0.4f, 25.3f), glm::vec3(1.75f, 3.0f, 27.3f)
    };

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, treeBoxes, 27, wallBoxes, 76, boat);

        std::cout << "X: " << cameraPos.x << " " << " Y: " << cameraPos.y << " " << " Z: " << cameraPos.z << std::endl;

        velocityY += gravity * deltaTime;
        cameraPos.y += velocityY * deltaTime;

        float playerX = cameraPos.x;
        float playerZ = cameraPos.z;

        if (abs(playerX) <= 25.0f && abs(playerZ) <= 25.0f) {
            groundLevel = 1.5f;
        }
        else if (abs(playerX) <= 40.0f && abs(playerZ) <= 40.0f) {
            float dist = std::max(abs(playerX), abs(playerZ));
            float t = (dist - 25.0f) / 15.0f;
            t = glm::min(t, 1.0f);
            groundLevel = 1.5f + (t * (-1.0f - 1.5f));
        }
        else {
            groundLevel = -1.0f;
        }

        for (int i = 0; i < 5; i++) {
            float dx = abs(cameraPos.x - platformPosition[i].x);
            float dz = abs(cameraPos.z - platformPosition[i].z);

            if (dx < 1.0f && dz < 1.0f) {
                groundLevel = platformPosition[i].y + 1.0f;
                onPlatform = true;
                break;
            }
            else {
                onPlatform = false;
            }
        }

        if (cameraPos.y <= groundLevel) {
            cameraPos.y = groundLevel;
            velocityY = 0.0f;
            isGrounded = true;
        }

        if (!gameOver && !gameWin) {
            waterLevel += waterRiseSpeed * deltaTime * 60.0f;
        }

        if (cameraPos.y <= waterLevel + 1.0f) {
            cameraPos = glm::vec3(0.0f, 1.5f, 5.0f);
            velocityY = 0.0f;
            isGrounded = true;
        }

        if (waterLevel >= 0.5f) {
            gameOver = true;
        }

        if (collectedcount >= 3) {
            float dx = cameraPos.x - 0.0f;
            float dz = cameraPos.z - 27.0f;
            if (sqrt(dx * dx + dz * dz) < 2.0f) {
                gameWin = true;
            }
        }

        glClearColor(0.53f, 0.81f, 0.98f, 1.0f);

        if (gameWin) {
            glClearColor(0.0f, 0.8f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glfwSwapBuffers(window);
            glfwPollEvents();
            float endTime = glfwGetTime() + 2.0f;
            while (glfwGetTime() < endTime) {
                glfwPollEvents();
            }
            glfwSetWindowShouldClose(window, true);
            std::cout << "You Win" << std::endl;
        }
        else if (gameOver) {
            glClearColor(0.8f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glfwSwapBuffers(window);
            glfwPollEvents();
            float endTime = glfwGetTime() + 2.0f;
            while (glfwGetTime() < endTime) {
                glfwPollEvents();
            }
            glfwSetWindowShouldClose(window, true);
            std::cout << "You Lose" << std::endl;
        }
        else {
            glClearColor(0.53f, 0.81f, 0.98f, 1.0f);
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 15.0f, 70.0f, 15.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.5f, 1.5f, 1.5f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 300.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(5.0f, 1.0f, 5.0f));
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");

        glm::mat4 waterModel = glm::mat4(1.0f);
        waterModel = glm::translate(waterModel, glm::vec3(0.0f, waterLevel + 1.5f, 0.0f));
        waterModel = glm::scale(waterModel, glm::vec3(5.0f, 1.0f, 5.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(waterModel));
        glActiveProgramEXT(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, oceantexture);
        glBindVertexArray(waterVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, sandtexture);
        glBindVertexArray(sandVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, sandtexture);
        glBindVertexArray(slopeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glBindTexture(GL_TEXTURE_2D, grasstexture);
        glBindVertexArray(grassVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glm::mat4 houseModel = glm::mat4(1.0f);
        houseModel = glm::translate(houseModel, glm::vec3(-14.0f, 0.5f, -20.0f));
        houseModel = glm::rotate(houseModel, glm::radians(23.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(houseModel));
        glBindTexture(GL_TEXTURE_2D, piecetexture);
        glBindVertexArray(houseWallVAO);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        houseModel = glm::mat4(1.0f);
        houseModel = glm::translate(houseModel, glm::vec3(-14.0f, 0.5f, -20.0f));
        houseModel = glm::rotate(houseModel, glm::radians(23.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(houseModel));
        glBindTexture(GL_TEXTURE_2D, piecetexture);
        glBindVertexArray(frontWallVAO);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        houseModel = glm::mat4(1.0f);
        houseModel = glm::translate(houseModel, glm::vec3(-14.0f, 0.5f, -20.0f));
        houseModel = glm::rotate(houseModel, glm::radians(23.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(houseModel));
        glBindTexture(GL_TEXTURE_2D, piecetexture);
        glBindVertexArray(roofVAO);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        for (int i = 0; i < 5; i++) {
            glm::mat4 platformModel = glm::mat4(1.0f);
            platformModel = glm::translate(platformModel, platformPosition[i]);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(platformModel));
            glBindTexture(GL_TEXTURE_2D, sandtexture);
            glBindVertexArray(platformVAO);
            glDrawArrays(GL_TRIANGLES, 0, 18);
        }

        for (int i = 0; i < 28; i++) {

            glm::mat4 treeModel = glm::mat4(1.0f);
            treeModel = glm::translate(treeModel, treeVertices[i]);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(treeModel));
            glBindTexture(GL_TEXTURE_2D, treetexture);
            glBindVertexArray(treeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glm::mat4 leavesModel = glm::mat4(1.0f);
            leavesModel = glm::translate(leavesModel, leavesVertices[i]);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(leavesModel));
            glBindTexture(GL_TEXTURE_2D, leavestexture);
            glBindVertexArray(leavesVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        for (int i = 0; i < 27; i++) {
            glm::mat4 leavesModel = glm::mat4(1.0f);
            leavesModel = glm::translate(leavesModel, leavesVertices[i]);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(leavesModel));
            glBindTexture(GL_TEXTURE_2D, leavestexture);
            glBindVertexArray(leavesVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        for (int i = 0; i < 24; i++) {
            glm::mat4 rockModel = glm::mat4(1.0f);
            rockModel = glm::translate(rockModel, rockVertices[i]);
            rockModel = glm::rotate(rockModel, glm::radians(15.0f * i), glm::vec3(0.0f, 1.0f, 0.3f));
            rockModel = glm::scale(rockModel, glm::vec3(1.2f, 0.8f, 1.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(rockModel));
            glBindTexture(GL_TEXTURE_2D, rocktexture);
            glBindVertexArray(rockVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glm::mat4 boatModel = glm::mat4(1.0f);
        boatModel = glm::translate(boatModel, glm::vec3(0.0f, 1.3f, 26.2f));
        boatModel = glm::rotate(boatModel, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(boatModel));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, piecetexture);
        glBindVertexArray(boatVAO);
        glDrawArrays(GL_TRIANGLES, 0, 24);

        glm::mat4 mastModel = glm::mat4(1.0f);
        mastModel = glm::translate(mastModel, glm::vec3(0.0f, 1.3f, 26.2f));
        mastModel = glm::rotate(mastModel, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(mastModel));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, piecetexture);
        glBindVertexArray(mastVAO);
        glDrawArrays(GL_TRIANGLES, 0, 24);

        float collectDistance = 2.0f;

        if (!piece1collected) {
            float dx = cameraPos.x - 0.0f;
            float dz = cameraPos.z - 8.0f;
            if (sqrt(dx * dx + dz * dz) < collectDistance) {
                piece1collected = true;
                collectedcount++;
            }
        };

        if (!piece2collected) {
            float dx = cameraPos.x - 51.0f;
            float dz = cameraPos.z - 51.0f;
            if (sqrt(dx * dx + dz * dz) < collectDistance) {
                piece2collected = true;
                collectedcount++;
            }
        };

        if (!piece3collected) {
            float dx = cameraPos.x - (-14.0f);
            float dz = cameraPos.z - (-20.0f);
            if (sqrt(dx * dx + dz * dz) < collectDistance) {
                piece3collected = true;
                collectedcount++;
            }
        };

        if (!piece1collected) {
            glm::mat4 piece1Model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(piece1Model));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, piecetexture);
            glBindVertexArray(piece1VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        }

        if (!piece2collected) {
            glm::mat4 piece2Model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(piece2Model));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, piecetexture);
            glBindVertexArray(piece2VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        }

        if (!piece3collected) {
            glm::mat4 piece3Model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(piece3Model));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, piecetexture);
            glBindVertexArray(piece3VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        }

        glUseProgram(sunshaderProgram);
        glm::mat4 sunModel = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(sunshaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
        glUniformMatrix4fv(glGetUniformLocation(sunshaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(sunshaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindVertexArray(sunVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glDisable(GL_DEPTH_TEST);
        glUseProgram(HUDshaderProgram);
        glBindVertexArray(HUDVAO);

        if (piece1collected) {
            glUniform3f(glGetUniformLocation(HUDshaderProgram, "color"), 0.35f, 0.1f, 0.01f);
        }
        else {
            glUniform3f(glGetUniformLocation(HUDshaderProgram, "color"), 0.25f, 0.25f, 0.25f);
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);

        if (piece2collected) {
            glUniform3f(glGetUniformLocation(HUDshaderProgram, "color"), 0.35f, 0.1f, 0.01f);
        }
        else {
            glUniform3f(glGetUniformLocation(HUDshaderProgram, "color"), 0.25f, 0.25f, 0.25f);
        }
        glDrawArrays(GL_TRIANGLES, 6, 6);

        if (piece3collected) {
            glUniform3f(glGetUniformLocation(HUDshaderProgram, "color"), 0.35f, 0.1f, 0.01f);
        }
        else {
            glUniform3f(glGetUniformLocation(HUDshaderProgram, "color"), 0.25f, 0.25f, 0.25f);
        }
        glDrawArrays(GL_TRIANGLES, 12, 6);

        glUseProgram(debugshaderProgram);


        glUniformMatrix4fv(glGetUniformLocation(debugshaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(debugshaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glEnable(GL_DEPTH_TEST);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteVertexArrays(1, &sandVAO);
    glDeleteBuffers(1, &sandVBO);
    glDeleteVertexArrays(1, &slopeVAO);
    glDeleteBuffers(1, &slopeVBO);
    glDeleteVertexArrays(1, &grassVAO);
    glDeleteBuffers(1, &grassVBO);;
    glDeleteVertexArrays(1, &piece1VAO);
    glDeleteBuffers(1, &piece1VBO);
    glDeleteVertexArrays(1, &piece2VAO);
    glDeleteBuffers(1, &piece2VBO);
    glDeleteVertexArrays(1, &piece3VAO);
    glDeleteBuffers(1, &piece3VBO);
    glDeleteBuffers(1, &houseWallVBO);
    glDeleteVertexArrays(1, &houseWallVAO);
    glDeleteBuffers(1, &piece3VBO);
    glDeleteBuffers(1, &frontWallVBO);
    glDeleteVertexArrays(1, &frontWallVAO);
    glDeleteBuffers(1, &roofVBO);
    glDeleteVertexArrays(1, &roofVAO);
    glDeleteBuffers(1, &treeVBO);
    glDeleteVertexArrays(1, &treeVAO);
    glDeleteBuffers(1, &leavesVBO);
    glDeleteVertexArrays(1, &leavesVAO);
    glDeleteBuffers(1, &rockVBO);
    glDeleteVertexArrays(1, &rockVAO);
    glDeleteBuffers(1, &boatVBO);
    glDeleteVertexArrays(1, &boatVAO);
    glDeleteBuffers(1, &mastVBO);
    glDeleteVertexArrays(1, &mastVAO);
    glDeleteBuffers(1, &platformVBO);
    glDeleteVertexArrays(1, &platformVAO);
    glDeleteBuffers(1, &sunVBO);
    glDeleteVertexArrays(1, &sunVAO);
    glDeleteBuffers(1, &HUDVBO);
    glDeleteVertexArrays(1, &HUDVAO);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(HUDshaderProgram);
    glDeleteProgram(sunshaderProgram);
    glDeleteProgram(debugshaderProgram);
    glfwTerminate();
    return 0;
}
