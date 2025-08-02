#include <iostream>
#include <glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Terrain.hpp"
#include "AppContext.hpp"
#include "FreeFlyCam.hpp"

ProjectConfig cfg {
    .windowWidth = 1280,
    .windowHeight = 1024,
    .camPos = glm::vec3(50.0f, 50.0f, 150.0f),

    .mapWidth = 512,
    .mapHeight = 512,

    .noiseSeed = 4245221,
    .noiseScale = 0.001f,
    .noiseFreq = 0.05f,
    .noiseOctaves = 8,
    .noiseLacunarity = 2.0f,
    .noisePersistence = 1.5f,
    .noiseExponent = 1.2f,
    .noiseHeightScale = 50.0f
};

std::string loadShaderSource(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // Check for errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint createShaderProgram(const std::string& vertSrc, const std::string& fragSrc) {
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSrc);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);

    // Check for linking errors
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    return program;
}


void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (ctx && key >= 0 && key < 1024) {
        ctx -> keys[key] = (action != GLFW_RELEASE);
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    static float lastX = cfg.windowWidth / 2.0f;
    static float lastY = cfg.windowHeight / 2.0f;
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    auto xoffset = static_cast<float>(xpos - lastX);
    auto yoffset = static_cast<float>(lastY - ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    if (!ctx -> cursorLocked) {
        return;
    }
    if (ctx) ctx->camera.processMouse(xoffset, yoffset);
}

int main()
{
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(cfg.windowWidth, cfg.windowHeight, "Rollercoaster - Terrain", nullptr,
                                          nullptr);
    if (!window) {
        std::cerr << "Failed to create window GLFW!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glViewport(0, 0, cfg.windowWidth, cfg.windowHeight);

    std::string vertexSource = loadShaderSource("shaders/terrain.vert");
    std::cout << "Vert: " << vertexSource << std::endl;
    std::string fragmentSource = loadShaderSource("shaders/terrain.frag");
    std::cout << "Frag: " <<  fragmentSource << std::endl;

    GLuint shaderProgram = createShaderProgram(vertexSource, fragmentSource);

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(cfg.windowWidth) / static_cast<float>(cfg.windowHeight),
        0.1f,
        1000.0f
    );

    AppContext context(cfg);
    glfwSetWindowUserPointer(window, &context);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);

    context.terrain.generate(cfg.noiseScale, cfg.noiseFreq, cfg.noiseOctaves, cfg.noiseLacunarity, cfg.noisePersistence,
                             cfg.noiseExponent, cfg.noiseHeightScale);
    context.terrain.uploadToGPU();

    std::cout << "MaxHeight = " << context.terrain.maxH() << std::endl;
    std::cout << "MinHeight = " << context.terrain.minH() << std::endl;

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {

        context.currentFrame = glfwGetTime();
        context.deltaTime = context.currentFrame - context.lastFrame;
        context.lastFrame = context.currentFrame;

        static bool prevF1 = false, prevF2 = false;
        bool f1 = context.keys[GLFW_KEY_F1];
        bool f2 = context.keys[GLFW_KEY_F2];

        if (f1 && !prevF1)
            context.polygonMode = GL_LINE;
        if (f2 && !prevF2)
            context.polygonMode = GL_FILL;

        prevF1 = f1;
        prevF2 = f2;

        static bool prevP = false, prevF = false;
        bool p = context.keys[GLFW_KEY_P];
        bool f = context.keys[GLFW_KEY_F];
        if (p && !prevP && context.cursorLocked) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            context.cursorLocked = false;
        }
        if (f && !prevF && !context.cursorLocked) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            context.cursorLocked = true;
        }


        // --- scena
        glClearColor(0.18f, 0.18f, 0.20f, 1.0f); // Szare tło
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glPolygonMode(GL_FRONT_AND_BACK, context.polygonMode);

        glUseProgram(shaderProgram);
        glUniform1f(glGetUniformLocation(shaderProgram, "minH"), context.terrain.minH());
        glUniform1f(glGetUniformLocation(shaderProgram, "maxH"), context.terrain.maxH());

        context.camera.processKeyboard(context.keys, context.deltaTime);
        glm::mat4 view = context.camera.getViewMatrix();

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        context.terrain.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Sprzątanie
    context.terrain.releaseGL();
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}