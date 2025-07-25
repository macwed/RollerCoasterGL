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
#include "Array_2D.hpp"
#include "SimplexNoise.hpp"

constexpr int WIN_WIDTH = 1280;
constexpr int WIN_HEIGHT = 720;

constexpr int MAP_WIDTH = 1024;
constexpr int MAP_HEIGHT = 1024;
constexpr unsigned SEED = 123432;
constexpr float SCALE = 0.001f;
constexpr float OFFSET = 131.0f;

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

    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "Rollercoaster - Terrain", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window GLFW!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    std::string vertexSource = loadShaderSource("shaders/terrain.vert");
    std::cout << "Vert: " << vertexSource << std::endl;
    std::string fragmentSource = loadShaderSource("shaders/terrain.frag");
    std::cout << "Frag: " <<  fragmentSource << std::endl;

    GLuint shaderProgram = createShaderProgram(vertexSource, fragmentSource);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(64, 100, 180), // pozycja kamery (spróbuj dopasować)
        glm::vec3(512, 0, 512),    // na co patrzy kamera (środek terenu)
        glm::vec3(0, 0, 1)       // wektor "góry"
    );
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(WIN_WIDTH) / static_cast<float>(WIN_HEIGHT),
        0.1f,
        100.0f
    );

    Array_2D<float> terrainArray(MAP_WIDTH, MAP_HEIGHT);
    Terrain terrain(MAP_WIDTH, MAP_HEIGHT, SEED);
    terrain.generate(SCALE, 0.01, 4, 2.0f, 0.5, 1.2);
    terrain.uploadToGPU();

    while (!glfwWindowShouldClose(window)) {
        // --- Tu scena
        glClearColor(0.18f, 0.18f, 0.20f, 1.0f); // Szare tło
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Potem, przed terrain.draw():
        glUseProgram(shaderProgram);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        terrain.draw();
        // --- Tutaj wywołasz terrain.draw() później ---

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 5. Sprzątanie
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}