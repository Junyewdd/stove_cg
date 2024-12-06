#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#undef STB_IMAGE_IMPLEMENTATION

#include <iostream>
#include "object_rot.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "assimp-vc143-mt.lib")

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void renderFloor(const Shader& shader);
unsigned int loadTexture(const char* path);
void renderScene(Shader& shader, Model& ourModel, unsigned int albedo, unsigned int normal, unsigned int metallic, unsigned int roughness, unsigned int ao, unsigned int woodTexture);

// settings
//const unsigned int SCR_WIDTH = 800;
//const unsigned int SCR_HEIGHT = 600;
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// camera
Camera camera(glm::vec3(0.0f, 0.5f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool dragMouseLeft = false;
ObjectRot objRotate;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// floor VAO/VBO
unsigned int floorVAO, floorVBO;
bool firstMouse = true;

// 바닥 정점 데이터
float floorVertices[] = {
    // positions          // normals         // texcoords
     25.0f, -0.1f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
    -25.0f, -0.1f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
    -25.0f, -0.1f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

     25.0f, -0.1f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
    -25.0f, -0.1f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
     25.0f, -0.1f, -25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
};


int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif



    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glew: load all OpenGL function pointers
    glewInit();

    // configure global OpenGL state
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    Shader depthShader("src/depth_shader.vs", "src/depth_shader.fs");
    Shader ourShader("src/scene_shader.vs", "src/scene_shader.fs");

    // FBO와 Depth Texture 생성
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 모델 및 VAO/VBO 초기화
    Model ourModel(FileSystem::getPath("../resources/stove/stove.obj"));

    ourShader.use();
    ourShader.setInt("albedoMap", 0);
    ourShader.setInt("normalMap", 1);
    ourShader.setInt("metallicMap", 2);
    ourShader.setInt("roughnessMap", 3);
    ourShader.setInt("aoMap", 4);
    ourShader.setInt("shadowMap", 5);

    unsigned int albedo = loadTexture(FileSystem::getPath("../resources/stove/textures/albedo.png").c_str());
    unsigned int normal = loadTexture(FileSystem::getPath("../resources/stove/textures/nor.png").c_str());
    unsigned int metallic = loadTexture(FileSystem::getPath("../resources/stove/textures/metal.png").c_str());
    unsigned int roughness = loadTexture(FileSystem::getPath("../resources/stove/textures/rough.png").c_str());
    unsigned int ao = loadTexture(FileSystem::getPath("../resources/stove/textures/ao.png").c_str());
    unsigned int woodTexture = loadTexture(FileSystem::getPath("../resources/textures/wood.png").c_str());

    // setup floor VAO/VBO
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);

    // set vertex attribute pointers
    glEnableVertexAttribArray(0); // positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // normals
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2); // texcoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // 1. 깊이 맵 생성 (Light's View)
        depthShader.use();
        glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f);
        glm::mat4 lightView = glm::lookAt(glm::vec3(-2.0f, 4.0f, -1.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;
        depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        renderScene(depthShader, ourModel, albedo, normal, metallic, roughness, ao, woodTexture); // 장면 렌더링
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. 최종 장면 렌더링 (Camera's View)
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 lightPos = glm::vec3(-20.0f, 6.0f, -5.0f); // Z축으로 -5.0f 위치에 빛을 놓음 (앞쪽에서 비춤)
        glm::vec3 lightTarget = glm::vec3(0.0f, 0.0f, 0.0f); // 원점(물체의 중심)을 목표로 설정
        glm::vec3 lightDir = glm::normalize(lightTarget - lightPos); // 빛의 방향 계산


        ourShader.use();
        ourShader.setMat4("projection", glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f));
        ourShader.setMat4("view", camera.GetViewMatrix());
        ourShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        //ourShader.setVec3("lightPos", glm::vec3(-5.0f, 6.0f, -1.0f));
        ourShader.setVec3("lightPos", lightPos);
        ourShader.setVec3("lightDir", lightDir);
        ourShader.setVec3("camPos", camera.Position);
        ourShader.setFloat("shadowIntensity", 3.5f); // 그림자 강도를 70%로 설정

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        ourShader.setInt("shadowMap", 5);

        renderScene(ourShader, ourModel, albedo, normal, metallic, roughness, ao, woodTexture);

        // glfw: swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }



    // cleanup
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVBO);

    glfwTerminate();
    return 0;
}

// input handling
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    // 위, 아래 화살표로 카메라 피치 조정
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessMouseMovement(0.0f, deltaTime * 500.0f); // 마우스 이동 시뮬레이션 (위로)
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessMouseMovement(0.0f, -deltaTime * 500.0f); // 마우스 이동 시뮬레이션 (아래로)
    // 시점 조작 (좌/우 화살표 - 요)
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessMouseMovement(-deltaTime * 500.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessMouseMovement(deltaTime * 500.0f, 0.0f);
}

// callback functions
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        lastX = xpos;
        lastY = ypos;
        dragMouseLeft = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        dragMouseLeft = false;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
};

void renderScene(Shader& shader, Model& ourModel, unsigned int albedo, unsigned int normal, unsigned int metallic, unsigned int roughness, unsigned int ao, unsigned int woodTexture)
{
    // 1. Render model
    glm::mat4 modelTransform = glm::mat4(1.0f);
    modelTransform = glm::translate(modelTransform, glm::vec3(0.0f, 0.0f, 0.0f));
    modelTransform = glm::rotate(modelTransform, objRotate.pitch(), glm::vec3(1.0f, 0.0f, 0.0f));
    modelTransform = glm::rotate(modelTransform, objRotate.yaw(), glm::vec3(0.0f, 1.0f, 0.0f));
    modelTransform = glm::rotate(modelTransform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    //modelTransform = glm::rotate(modelTransform, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    shader.setMat4("model", modelTransform);

    // Bind textures for the model
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, metallic);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, roughness);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, ao);

    // Draw the model
    ourModel.Draw(shader);

    // 2. Render floor
    glm::mat4 floorModel = glm::mat4(1.0f);
    shader.setMat4("model", floorModel);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, woodTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

}
