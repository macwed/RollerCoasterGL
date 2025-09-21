#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <fstream>
#include <glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <sstream>
#include <string>

#include "AppContext.hpp"
#include "camera/FreeFlyCam.hpp"
#include "gameplay/Car.hpp"
#include "gameplay/TrackComponent.hpp"
#include "gfx/geometry/RailGeometryBuilder.hpp"
#include "gfx/render/Texture.hpp"
#include "gfx/render/Track.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "terrain/Terrain.hpp"

ProjectConfig cfg{.windowWidth = 1920,
                  .windowHeight = 1280,
                  .camPos = glm::vec3(10.0f, 50.0f, 0.0f),

                  .mapWidth = 256,
                  .mapHeight = 256,

                  .noiseSeed = 4245221,
                  .noiseScale = 0.0010f,
                  .noiseFreq = 0.02f,
                  .noiseOctaves = 10,
                  .noiseLacunarity = 2.166f,
                  .noisePersistence = 1.483f,
                  .noiseExponent = 1.2f,
                  .noiseHeightScale = 28.0f};

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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (ctx && key >= 0 && key < 1024) {
        ctx->keys[key] = (action != GLFW_RELEASE);
    }
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));
    static float lastX = static_cast<float>(cfg.windowWidth) / 2.0f;
    static float lastY = static_cast<float>(cfg.windowHeight) / 2.0f;
    static bool firstMouse = true;
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    auto xoffset = static_cast<float>(xpos - lastX);
    auto yoffset = static_cast<float>(lastY - ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    if (!ctx->cursorLocked) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse && ctx->cursorLocked)
        ctx->camera.processMouse(xoffset, yoffset);
}

int main() {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // MSAA
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);


    GLFWwindow* window = glfwCreateWindow(cfg.windowWidth, cfg.windowHeight, "RollercoasterGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window GLFW!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_DEPTH_TEST);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glViewport(0, 0, cfg.windowWidth, cfg.windowHeight);

//----------------------------------LOAD ASSETS -------------------------------------------------

    std::string terrainVerSrc = loadShaderSource("assets/shaders/terrain.vert");
    std::string terrainFragSrc = loadShaderSource("assets/shaders/terrain.frag");
    std::string trackVerSrc = loadShaderSource("assets/shaders/track.vert");
    std::string trackFragSrc = loadShaderSource("assets/shaders/track.frag");
    std::string skyVerSrc   = loadShaderSource("assets/shaders/skybox.vert");
    std::string skyFragSrc  = loadShaderSource("assets/shaders/skybox.frag");

    GLuint texGrassD = LoadTexture2D("assets/textures/grass/grassy_d.png", true);
    GLuint texGrassS = LoadTexture2D("assets/textures/grass/grassy_s.png", false);
    GLuint texGrassN = LoadTexture2D("assets/textures/grass/grass_n.tga", false);

    GLuint texSteelD = LoadTexture2D("assets/textures/steel/diffuse.tga", true);
    GLuint texSteelS = LoadTexture2D("assets/textures/steel/specular.tga", false);

    GLuint terrainProgram = createShaderProgram(terrainVerSrc, terrainFragSrc);
    GLuint trackProgram   = createShaderProgram(trackVerSrc, trackFragSrc);
    GLuint skyProgram     = createShaderProgram(skyVerSrc, skyFragSrc);

    glUseProgram(terrainProgram);
    glUniform1i(glGetUniformLocation(terrainProgram, "texAlbedo"), 0);
    glUniform1i(glGetUniformLocation(terrainProgram, "texSpec"), 1);
    glUniform1i(glGetUniformLocation(terrainProgram, "texNormal"), 2);

    glUseProgram(trackProgram);
    glUniform1i(glGetUniformLocation(trackProgram, "texAlbedo"), 0);
    glUniform1i(glGetUniformLocation(trackProgram, "texSpec"), 1);

    // VAO nieba
    GLuint skyVAO=0, skyVBO=0;
    {
        float skyboxVertices[] = {
            // ręczny skybox
            -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
        };
        glGenVertexArrays(1,&skyVAO);
        glGenBuffers(1,&skyVBO);
        glBindVertexArray(skyVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
        glBindVertexArray(0);
    }

//------------------------------------------------------------------------------------------------
//----------------------------------------TRAKC-----------------------------------------------------

    AppContext context(cfg);
    // patrz na środek
    context.camera.lookAtTarget(glm::vec3(static_cast<float>(cfg.mapWidth) * 0.5f, context.camera.position.y, static_cast<float>(cfg.mapHeight) * 0.5f));
    glfwSetWindowUserPointer(window, &context);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);

    context.terrain.generate(cfg.noiseScale, cfg.noiseFreq, cfg.noiseOctaves, cfg.noiseLacunarity, cfg.noisePersistence,
                             cfg.noiseExponent, cfg.noiseHeightScale);
    context.terrain.uploadToGPU();

    std::cout << "MaxHeight = " << context.terrain.maxH() << std::endl;
    std::cout << "MinHeight = " << context.terrain.minH() << std::endl;

    rc::gameplay::TrackComponent trackComp;
    auto& spl = trackComp.spline();
    spl.addNode({{110.f, 22.f, 29.f}});
    spl.addNode({{62.f, 18.f, 20.f}});
    spl.addNode({{56.f, 18.f, 21.f}});
    spl.addNode({{38.f, 18.f, 31.f}});
    spl.addNode({{43.f, 18.f, 45.f}});
    spl.addNode({{45.f, 20.f, 50.f}});

    spl.addNode({{88.f, 12.f, 55.f}});
    spl.addNode({{108.f, 14.f, 50.f}});
    spl.addNode({{132.f, 16.f, 47.f}});
    spl.addNode({{157.f, 22.f, 47.f}});
    spl.addNode({{176.f, 38.f, 49.f}});


    /*s.addNode({{166.f, 24.f, 40.f}});
    s.addNode({{170.f, 32.f, 40.f}});
    s.addNode({{178.f, 39.f, 43.f}});
    s.addNode({{191.f, 49.f, 50.f}});*/

    spl.addNode({{196.f, 58.f, 53.f}});
    spl.addNode({{209.f, 65.f, 67.f}});
    spl.addNode({{224.f, 70.f, 90.f}});
    spl.addNode({{224.f, 70.f, 93.f}});
    spl.addNode({{224.f, 63.f, 103.f}});
    spl.addNode({{220.f, 51.f, 112.f}});
    spl.addNode({{215.f, 29.f, 120.f}});
    spl.addNode({{206.f, 35.f, 121.f}});
    spl.addNode({{196.f, 39.f, 101.f}});
    spl.addNode({{199.f, 35.f, 95.f}});
    spl.addNode({{202.f, 29.f, 92.f}});
    spl.addNode({{232.f, 11.f, 87.f}});
    spl.addNode({{239.f, 15.f, 82.f}});
    spl.addNode({{236.f, 21.f, 62.f}});
    spl.addNode({{218.f, 24.f, 41.f}});
    spl.addNode({{182.f, 85.f, 37.f}});
    spl.addNode({{164.f, 85.f, 35.f}});
    spl.addNode({{157.f, 72.f, 34.f}});
    spl.addNode({{147.f, 29.f, 33.f}});
    spl.addNode({{137.f, 34.f, 32.f}});




    trackComp.setClosed(true);
    trackComp.setDs(0.05f);
    trackComp.setUp({0.f, 1.f, 0.f});
    trackComp.markDirty();
    trackComp.rebuild();
    const auto& frames = trackComp.frames();


    rc::gfx::render::Track track;
    rc::gfx::geometry::RailParams railP{1.435f, 0.12f, 16, trackComp.isClosed(), 0.25f};
    rc::gfx::render::InfraParams infraP;
    infraP.beamDs        = 0.60f;
    infraP.supportHoriz  = 3.0f;
    infraP.beamThick     = 0.08f;
    infraP.beamHeight    = 0.06f;
    infraP.supportRadius = 0.06f;
    infraP.supportSides  = 12;
    infraP.minClearance  = 0.25f;

    track.build(frames, railP, context.terrain, infraP);

//---------------------------------------CAR----------------------------------------------

    rc::gameplay::Car car;
    car.bindTrack(trackComp);
    car.kick(100.f);
    GLuint carShader = createShaderProgram(
        loadShaderSource("assets/shaders/car.vert"),
        loadShaderSource("assets/shaders/car.frag")
    );

    // wagonik box bo nie mam modelu :(
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f
    };
    unsigned int cubeIndices[] = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        0,1,5, 5,4,0,
        2,3,7, 7,6,2,
        0,3,7, 7,4,0,
        1,2,6, 6,5,1
    };
    GLuint cubeVAO=0,cubeVBO=0,cubeEBO=0;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glGenBuffers(1, &cubeEBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // kółka wagonika
    GLuint wheelVAO = 0, wheelVBO = 0, wheelEBO = 0; GLsizei wheelIndexCount = 0;


//---------------------------------------proj----------------------------------------------

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 projection =
            glm::perspective(glm::radians(45.0f),
                             static_cast<float>(cfg.windowWidth) / static_cast<float>(cfg.windowHeight), 0.5f, 1000.0f);

//---------------------------------------IMGUI----------------------------------------------

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");


//---------------------------------------LOOP-----------------------------------------------
    while (!glfwWindowShouldClose(window)) {

        context.currentFrame = static_cast<float>(glfwGetTime());
        context.deltaTime = context.currentFrame - context.lastFrame;
        if (context.deltaTime >= 0.05f) context.deltaTime = 0.05f;
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
            context.showTerrainPanel = true;
        }
        if (f && !prevF && !context.cursorLocked) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            context.cursorLocked = true;
            context.showTerrainPanel = false;
        }
        static bool prevC = false;
        bool c = context.keys[GLFW_KEY_C];
        if (c && !prevC) {
            using M = CamMode;
            context.camMode = (context.camMode == M::Free) ? M::Ride
                             : (context.camMode == M::Ride) ? M::Chase
                             : M::Free;
            context.camInitialized = false; // reset filtra kamery
        }
        prevC = c;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (context.showTerrainPanel) {
            ImGui::Begin("Panel terenu", &context.showTerrainPanel);
            ImGui::Text("Parametry szumu i terenu");
            ImGui::SliderFloat("Height", &cfg.noiseHeightScale, 1.0f, 200.0f);
            ImGui::SliderFloat("Scale", &cfg.noiseScale, 0.0001f, 0.1f, "%.4f");
            ImGui::SliderInt("Octaves", &cfg.noiseOctaves, 1, 12);
            ImGui::SliderFloat("Lacunarity", &cfg.noiseLacunarity, 1.0f, 4.0f);
            ImGui::SliderFloat("Persistence", &cfg.noisePersistence, 0.1f, 2.0f);
            ImGui::InputInt("Seed", &cfg.noiseSeed);
            if (ImGui::Button("Generate Terrain")) {
                context.terrain.releaseGL();
                context.terrain = Terrain(cfg.mapWidth, cfg.mapHeight, cfg.noiseSeed);
                context.terrain.generate(cfg.noiseScale, cfg.noiseFreq, cfg.noiseOctaves, cfg.noiseLacunarity,
                                         cfg.noisePersistence, cfg.noiseExponent, cfg.noiseHeightScale);
                context.terrain.uploadToGPU();
            }

            ImGui::End();
        }

        // --- scena
        glm::vec3 upWS{0,1,0};

        auto makeCarCamera = [&](CamMode mode, float dt) -> glm::mat4 {
            constexpr glm::vec3 UP_LOCAL = glm::vec3(0,1,0);
            const glm::quat q = car.getOrientation();
            const glm::vec3 pp = car.getPos();
            upWS = glm::normalize(q * UP_LOCAL);

            glm::vec3 eye, target;
            if (mode == CamMode::Ride) {
                constexpr glm::vec3 seatOffsetLocal = glm::vec3(0.5f, 1.f, 0.0f);
                constexpr glm::vec3 lookAheadLocal  = glm::vec3(2.0f, 0.0f, 0.0f);
                eye    = pp + q * seatOffsetLocal;
                target = pp + q * (seatOffsetLocal + lookAheadLocal);
            } else { // Chase
                constexpr glm::vec3 chaseOffsetLocal = glm::vec3(-6.0f, 2.0f, 0.0f);
                constexpr glm::vec3 aimOffsetLocal   = glm::vec3( 1.0f, 0.5f, 0.0f);
                eye    = pp + q * chaseOffsetLocal;
                target = pp + q * aimOffsetLocal;
            }

            constexpr float cutoffHz = 12.0f;
            float alpha = 1.0f - std::exp(-cutoffHz * dt);
            if (!context.camInitialized) {
                context.smoothedEye    = eye;
                context.smoothedTarget = target;
                context.camInitialized = true;
            } else {
                context.smoothedEye    = glm::mix(context.smoothedEye,    eye,    alpha);
                context.smoothedTarget = glm::mix(context.smoothedTarget, target, alpha);
            }
            return glm::lookAt(context.smoothedEye, context.smoothedTarget, upWS);
        };

        glm::mat4 view;
        glm::vec3 camPosWorld;
        if (context.camMode == CamMode::Free) {
            if (!io.WantCaptureKeyboard)
                context.camera.processKeyboard(context.keys, context.deltaTime, &car);
            view = context.camera.getViewMatrix();
            camPosWorld = context.camera.position;
        } else {
            view = makeCarCamera(context.camMode, context.deltaTime);
            camPosWorld = context.smoothedEye;
        }

        // Toggle min-speed
        {
            static bool prevM = false;
            bool curM = context.keys[GLFW_KEY_M];
            if (curM && !prevM && !io.WantCaptureKeyboard) {
                car.minSpeedEnabled = !car.minSpeedEnabled;
                if (car.minSpeedEnabled && car.minSpeed < 1.0f) car.minSpeed = 20.0f;
            }
            prevM = curM;
        }

        // Car
        {
            ImGui::Begin("Car Controls");
            ImGui::Text("Speed: %.1f m/s", car.v);
            ImGui::Checkbox("Min speed enabled", &car.minSpeedEnabled);
            ImGui::SliderFloat("Min speed (m/s)", &car.minSpeed, 0.0f, 100.0f, "%.1f");
            ImGui::End();
        }

        // edycja toru
        {
            ImGui::Begin("Track Editor");

            bool canEdit = (context.camMode == CamMode::Free);
            glm::vec3 camPosUI = context.camera.position;
            ImGui::Text("CamMode: %s", canEdit ? "Free" : "(Ride/Chase) – switch to Free to edit");
            ImGui::Text("Camera XYZ: %.2f  %.2f  %.2f", camPosUI.x, camPosUI.y, camPosUI.z);

            auto& splineRef = trackComp.spline();
            int nodeCount = static_cast<int>(splineRef.nodeCount());
            ImGui::Text("Nodes: %d", nodeCount);

            bool isClosed = trackComp.isClosed();
            if (ImGui::Checkbox("Closed loop", &isClosed)) {
                trackComp.setClosed(isClosed);
                trackComp.rebuild();
                car.onTrackRebuilt(trackComp);
                const auto& frames2 = trackComp.frames();
                track.build(frames2, railP, context.terrain, infraP);
            }

            ImGui::BeginDisabled(!canEdit);
            static bool snapToGround = true;
            static float snapClearance = 0.5f;
            ImGui::Checkbox("Snap to terrain", &snapToGround);
            if (snapToGround) ImGui::SliderFloat("Clearance", &snapClearance, 0.0f, 5.0f, "%.2f m");
            if (ImGui::Button("Add Node @ Camera")) {
                glm::vec3 P = camPosUI;
                if (snapToGround) P.y = context.terrain.sampleHeightBilinear(P.x, P.z) + snapClearance;
                float r = (splineRef.nodeCount() > 0) ? splineRef.getNode(splineRef.nodeCount()-1).roll : 0.0f;
                splineRef.addNode({P, r, 0.f, 0.f, 0.f});
                trackComp.markDirty();
                trackComp.rebuild();
                car.onTrackRebuilt(trackComp);
                const auto& frames2 = trackComp.frames();
                track.build(frames2, railP, context.terrain, infraP);
            }
            // dodaj ogon - 2 nody
            static float tailSegLen = 5.0f;
            ImGui::SliderFloat("Tail seg length", &tailSegLen, 0.5f, 20.0f, "%.1f m");
            if (ImGui::Button("Add Segment @ Camera")) {
                glm::vec3 P2 = camPosUI;
                glm::vec3 fwd = context.camera.front;
                if (glm::length2(fwd) < 1e-6f) fwd = glm::vec3(1,0,0);
                glm::vec3 P1 = P2 - glm::normalize(fwd) * tailSegLen;
                if (snapToGround) {
                    P1.y = context.terrain.sampleHeightBilinear(P1.x, P1.z) + snapClearance;
                    P2.y = context.terrain.sampleHeightBilinear(P2.x, P2.z) + snapClearance;
                }
                float r = (splineRef.nodeCount() > 0) ? splineRef.getNode(splineRef.nodeCount()-1).roll : 0.0f;
                splineRef.addNode({P1, r, 0.f, 0.f, 0.f});
                splineRef.addNode({P2, r, 0.f, 0.f, 0.f});
                trackComp.markDirty();
                trackComp.rebuild();
                car.onTrackRebuilt(trackComp);
                const auto& frames2 = trackComp.frames();
                track.build(frames2, railP, context.terrain, infraP);
            }
            // dodaj nod po indeksie
            static int insertAfter = -1; if (insertAfter < -1) insertAfter = -1;
            ImGui::InputInt("Insert after idx (-1=begin)", &insertAfter);
            if (ImGui::Button("Insert Node @ Camera")) {
                glm::vec3 P = camPosUI;
                if (snapToGround) P.y = context.terrain.sampleHeightBilinear(P.x, P.z) + snapClearance;
                std::size_t n = splineRef.nodeCount();
                std::size_t pos = (insertAfter < 0) ? 0 : std::min<std::size_t>(insertAfter + 1, n);
                float rPrev = (pos > 0) ? splineRef.getNode(pos-1).roll : (n>0 ? splineRef.getNode(0).roll : 0.0f);
                splineRef.insertNode(pos, {P, rPrev, 0.f, 0.f, 0.f});
                trackComp.markDirty();
                trackComp.rebuild();
                car.onTrackRebuilt(trackComp);
                const auto& frames2 = trackComp.frames();
                track.build(frames2, railP, context.terrain, infraP);
            }
            // dodaj nod w miejsuc kamery
            static int moveIdx = 0; if (moveIdx < 0) moveIdx = 0;
            ImGui::InputInt("Move idx to camera", &moveIdx);
            if (ImGui::Button("Move Node")) {
                if (splineRef.nodeCount() > 0) {
                    std::size_t idx = std::min<std::size_t>(moveIdx, splineRef.nodeCount()-1);
                    glm::vec3 P = camPosUI;
                    if (snapToGround) P.y = context.terrain.sampleHeightBilinear(P.x, P.z) + snapClearance;
                    splineRef.moveNode(idx, P);
                    trackComp.markDirty();
                    trackComp.rebuild();
                    car.onTrackRebuilt(trackComp);
                    const auto& frames2 = trackComp.frames();
                    track.build(frames2, railP, context.terrain, infraP);
                }
            }
            // linearyzowanie ostatnich nodów
            static bool tailLinear = true; ImGui::Checkbox("Linearize tail", &tailLinear);
            static int tailCount = 1; ImGui::SliderInt("Tail segments", &tailCount, 1, 3);
            if (ImGui::Button("Apply Tail Linear")) {
                int segs = static_cast<int>(splineRef.segmentCount());
                for (int k = 0; tailLinear && k < tailCount; ++k) {
                    int segIdx = segs - 1 - k;
                    if (segIdx >= 0) {
                        trackComp.setLinearBySegment(static_cast<std::size_t>(segIdx));
                    }
                }
                trackComp.markDirty();
                trackComp.rebuild();
                car.onTrackRebuilt(trackComp);
                const auto& frames2 = trackComp.frames();
                track.build(frames2, railP, context.terrain, infraP);
            }
            ImGui::EndDisabled();

            // zmiana rolla
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Roll Editor", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Single node roll (deg)");
                static int rollIdx = 0; if (rollIdx < 0) rollIdx = 0;
                ImGui::InputInt("Node idx##singleRollIdx", &rollIdx);
                if (splineRef.nodeCount() > 0) rollIdx = std::min<int>(rollIdx, static_cast<int>(splineRef.nodeCount()) -1);
                float rollDegCurr = 0.0f;
                if (splineRef.nodeCount() > 0) rollDegCurr = glm::degrees(splineRef.getNode(static_cast<std::size_t>(rollIdx)).roll);
                static float rollDeg = 0.0f; rollDeg = rollDegCurr;
                ImGui::SliderFloat("Roll (deg)", &rollDeg, -2.f, 2.f);
                if (ImGui::Button("Apply Roll")) {
                    if (splineRef.nodeCount() > 0) {
                        trackComp.setNodeRoll(static_cast<std::size_t>(rollIdx), glm::radians(rollDeg));
                        trackComp.rebuild();
                        car.onTrackRebuilt(trackComp);
                        const auto& frames2 = trackComp.frames();
                        track.build(frames2, railP, context.terrain, infraP);
                    }
                }

                ImGui::Separator();
                ImGui::Text("Spread / Smooth Roll (deg)");
                static int idxA = 0, idxB = 0; static float rollA = 0.f, rollB = 0.f;
                ImGui::InputInt("A##rollSpreadA", &idxA); ImGui::SameLine(); ImGui::InputInt("B##rollSpreadB", &idxB);
                ImGui::InputFloat("Roll A##rollSpreadAVal", &rollA); ImGui::SameLine(); ImGui::InputFloat("Roll B##rollSpreadBVal", &rollB);
                static int smoothType = 1; // 0=Linear,1=Smoothstep,2=Cosine,3=Quintic
                ImGui::RadioButton("Linear", &smoothType, 0); ImGui::SameLine();
                ImGui::RadioButton("Smoothstep", &smoothType, 1); ImGui::SameLine();
                ImGui::RadioButton("Cosine", &smoothType, 2); ImGui::SameLine();
                ImGui::RadioButton("Quintic", &smoothType, 3);
                if (ImGui::Button("Apply Spread")) {
                    std::size_t n = splineRef.nodeCount();
                    if (n >= 2) {
                        std::size_t a = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, idxA)), 0, n-1);
                        std::size_t b = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, idxB)), 0, n-1);
                        if (a > b) std::swap(a,b);
                        for (std::size_t i = a; i <= b; ++i) {
                            float t = (b==a) ? 0.f : static_cast<float>(i - a) / static_cast<float>(b - a);
                            float w = t;
                            if (smoothType == 1)      w = t*t*(3.f - 2.f*t); // smoothstep
                            else if (smoothType == 2) w = 0.5f - 0.5f*std::cos(t * glm::pi<float>()); // cosine ease
                            else if (smoothType == 3) w = t*t*t*(t*(t*6.f - 15.f) + 10.f); // quintic smoothstep
                            float r = glm::radians(rollA * (1.f - w) + rollB * w);
                            splineRef.setNodeRoll(i, r);
                        }
                        trackComp.markDirty();
                        trackComp.rebuild();
                        car.onTrackRebuilt(trackComp);
                        const auto& frames2 = trackComp.frames();
                        track.build(frames2, railP, context.terrain, infraP);
                    }
                }
            }

            // Node list editor
            ImGui::Separator();
            if (ImGui::CollapsingHeader("Nodes", ImGuiTreeNodeFlags_DefaultOpen)) {
                static int selectedIdx = -1;
                std::size_t n = splineRef.nodeCount();
                for (std::size_t i = 0; i < n; ++i) {
                    auto node = splineRef.getNode(i);
                    bool sel = (static_cast<int>(i) == selectedIdx);
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::Selectable(("Node " + std::to_string(i)).c_str(), sel)) {
                        selectedIdx = static_cast<int>(i);
                    }
                    ImGui::SameLine();
                    ImGui::Text("(%.2f, %.2f, %.2f)  roll=%.1f", node.pos.x, node.pos.y, node.pos.z, glm::degrees(node.roll));
                    ImGui::PopID();
                }
                ImGui::Separator();
                if (selectedIdx >= 0 && static_cast<std::size_t>(selectedIdx) < splineRef.nodeCount()) {
                    static float editPos[3] = {0,0,0};
                    static float editRollDeg = 0.f;
                    static int lastSel = -2;
                    if (lastSel != selectedIdx) {
                        auto node = splineRef.getNode(static_cast<std::size_t>(selectedIdx));
                        editPos[0] = node.pos.x; editPos[1] = node.pos.y; editPos[2] = node.pos.z;
                        editRollDeg = glm::degrees(node.roll);
                        lastSel = selectedIdx;
                    }
                    ImGui::InputFloat3("Edit Pos", editPos, "%.3f");
                    ImGui::SliderFloat("Edit Roll (deg)", &editRollDeg, -2.f, 2.f, "%.1f");
                    if (ImGui::Button("Apply Selected")) {
                        splineRef.moveNode(static_cast<std::size_t>(selectedIdx), {editPos[0], editPos[1], editPos[2]});
                        trackComp.setNodeRoll(static_cast<std::size_t>(selectedIdx), glm::radians(editRollDeg));
                        trackComp.rebuild();
                        car.onTrackRebuilt(trackComp);
                        const auto& frames2 = trackComp.frames();
                        track.build(frames2, railP, context.terrain, infraP);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("SnapY Selected")) {
                        float y = context.terrain.sampleHeightBilinear(editPos[0], editPos[2]);
                        editPos[1] = y;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Selected")) {
                        splineRef.removeNode(static_cast<std::size_t>(selectedIdx));
                        selectedIdx = -1; lastSel = -2;
                        trackComp.markDirty();
                        trackComp.rebuild();
                        car.onTrackRebuilt(trackComp);
                        const auto& frames2 = trackComp.frames();
                        track.build(frames2, railP, context.terrain, infraP);
                    }
                } else {
                    ImGui::Text("Select a node to edit.");
                }
            }

            // rozsmaruj rolla
            static int idxA = 0, idxB = 0; static float rollA = 0.f, rollB = 0.f;
            ImGui::InputInt("Spread A", &idxA); ImGui::SameLine(); ImGui::InputInt("B", &idxB);
            ImGui::InputFloat("Roll A (deg)", &rollA); ImGui::SameLine(); ImGui::InputFloat("Roll B (deg)", &rollB);
            if (ImGui::Button("Spread Roll A..B")) {
                std::size_t n = splineRef.nodeCount();
                if (n >= 2) {
                    std::size_t a = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, idxA)), 0, n-1);
                    std::size_t b = std::clamp<std::size_t>(static_cast<std::size_t>(std::max(0, idxB)), 0, n-1);
                    if (a > b) std::swap(a,b);
                    for (std::size_t i = a; i <= b; ++i) {
                        float t = (b==a) ? 0.f : static_cast<float>(i - a) / static_cast<float>(b - a);
                        float r = glm::radians(rollA * (1.f - t) + rollB * t);
                        splineRef.setNodeRoll(i, r);
                    }
                    trackComp.markDirty();
                    trackComp.rebuild();
                    car.onTrackRebuilt(trackComp);
                    const auto& frames2 = trackComp.frames();
                    track.build(frames2, railP, context.terrain, infraP);
                }
            }
            ImGui::SameLine();
            static int removeIdx = -1;
            if (removeIdx < -1) removeIdx = -1;
            ImGui::InputInt("Remove idx (-1=last)", &removeIdx);
            if (ImGui::Button("Remove Node")) {
                std::size_t n = splineRef.nodeCount();
                if (n > 0) {
                    std::size_t idx = (removeIdx < 0 || removeIdx >= static_cast<int>(n)) ? (n - 1) : static_cast<std::size_t>(removeIdx);
                    if (idx < splineRef.nodeCount()) {
                        splineRef.removeNode(idx);
                        trackComp.markDirty();
                        trackComp.rebuild();
                        car.onTrackRebuilt(trackComp);
                        const auto& frames2 = trackComp.frames();
                        track.build(frames2, railP, context.terrain, infraP);
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Rebuild Track")) {
                trackComp.rebuild();
                car.onTrackRebuilt(trackComp);
                const auto& frames2 = trackComp.frames();
                track.build(frames2, railP, context.terrain, infraP);
            }

            ImGui::End();
        }

        // --- clear ---
        glClearColor(0.18f, 0.18f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ===== SKYBOX =====
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyProgram);
        // widok bez translacji (tylko rotacja kamery)
        glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(glGetUniformLocation(skyProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(skyProgram, "uViewNoTrans"), 1, GL_FALSE, glm::value_ptr(viewNoTrans));
        // kolory nieba (na czuja – można wyregulować)
        glUniform3f(glGetUniformLocation(skyProgram, "uTopColor"),     0.10f, 0.22f, 0.45f);
        glUniform3f(glGetUniformLocation(skyProgram, "uHorizonColor"), 0.52f, 0.64f, 0.80f);
        glUniform3f(glGetUniformLocation(skyProgram, "uBottomColor"),  0.80f, 0.85f, 0.92f);
        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);

        // ===== TERRAIN =====
        glUseProgram(terrainProgram);

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texGrassD);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texGrassS);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, texGrassN);

        glUniformMatrix4fv(glGetUniformLocation(terrainProgram,"model"),      1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(terrainProgram,"view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(terrainProgram,"projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3f (glGetUniformLocation(terrainProgram,"dirLightDir"),   0.2f,-0.9f,0.1f);
        glUniform3f (glGetUniformLocation(terrainProgram,"dirLightColor"), 1.0f,0.98f,0.95f);
        glUniform3fv(glGetUniformLocation(terrainProgram,"uCamPos"), 1, glm::value_ptr(camPosWorld));
        glUniform3f (glGetUniformLocation(terrainProgram,"fogColor"),   0.04f,0.045f,0.055f);
        glUniform1f (glGetUniformLocation(terrainProgram,"fogDensity"), 0.020f);

        context.terrain.draw();

        // ===== TRACK =====
        glUseProgram(trackProgram);

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texSteelD);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texSteelS);

        glUniformMatrix4fv(glGetUniformLocation(trackProgram,"model"),      1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(trackProgram,"view"),       1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(trackProgram,"projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(trackProgram,"uViewPos"), 1, glm::value_ptr(camPosWorld));
        glUniform3f (glGetUniformLocation(trackProgram,"dirLightDir"),   0.2f,-0.9f,0.1f);
        glUniform3f (glGetUniformLocation(trackProgram,"dirLightColor"), 1.0f,0.98f,0.95f);
        glUniform3fv(glGetUniformLocation(trackProgram,"pointPos"),  1, glm::value_ptr(car.getPos()));
        glUniform3f (glGetUniformLocation(trackProgram,"pointColor"),    1.0f,0.9f,0.7f);
        glUniform1f (glGetUniformLocation(trackProgram,"pointRange"),    25.0f);

        track.draw();

        // ===== CAR =====
        car.update(context.deltaTime, trackComp);

        glm::mat4 carBase(1.0f);
        carBase = glm::translate(carBase, car.getPos());
        carBase *= glm::mat4(car.getOrientation());

        float bodyLift = 0.50f; //żeby box był ponad torem ^^
        glm::mat4 carModel = carBase
                            * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, bodyLift, 0.0f))
                            * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 1.0f, 1.0f));
        glUseProgram(carShader);
        glUniformMatrix4fv(glGetUniformLocation(carShader,"model"),1,GL_FALSE,glm::value_ptr(carModel));
        glUniformMatrix4fv(glGetUniformLocation(carShader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(carShader,"projection"),1,GL_FALSE,glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(carShader, "uColor"), 0.42f, 0.45f, 0.28f);
        glBindVertexArray(cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        // kółeczka
        if (wheelVAO==0) {
            const int seg = 16;
            std::vector<float> vtx; vtx.reserve((seg+1)*2*3);
            std::vector<unsigned> idx; idx.reserve(seg*6);
            for(int ring=0; ring<2; ++ring){
                float z = (ring==0)? -0.5f : 0.5f;
                for(int i=0;i<=seg;++i){
                    float t = static_cast<float>(i) /seg; float a = t * 6.28318530718f;
                    float x = cosf(a), y = sinf(a);
                    vtx.push_back(x); vtx.push_back(y); vtx.push_back(z);
                }
            }
            for(int i=0;i<seg;++i){
                unsigned a = i;
                unsigned b = i+1;
                unsigned c = (seg+1)+i;
                unsigned d = (seg+1)+i+1;
                idx.push_back(a); idx.push_back(b); idx.push_back(c);
                idx.push_back(b); idx.push_back(d); idx.push_back(c);
            }
            glGenVertexArrays(1,&wheelVAO);
            glGenBuffers(1,&wheelVBO);
            glGenBuffers(1,&wheelEBO);
            glBindVertexArray(wheelVAO);
            glBindBuffer(GL_ARRAY_BUFFER,wheelVBO);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vtx.size()*sizeof(float)), vtx.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float), nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,wheelEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(idx.size()*sizeof(unsigned)), idx.data(), GL_STATIC_DRAW);
            wheelIndexCount = static_cast<GLsizei>(idx.size());
            glBindVertexArray(0);
        }

        //kółka względem wagonika
        const float gauge = 1.435f;
        float halfGauge = gauge * 0.5f;
        float wheelR = 0.22f; // radius
        float wheelT = 0.10f; // grubość
        float yOffset = 0.34f; // powyżej szyn
        float xOff = 0.8f; // przód/tył od środka
        glm::vec3 offs[4] = {
            { xOff, yOffset,  halfGauge}, // przód P
            { xOff, yOffset, -halfGauge}, // przód L
            {-xOff, yOffset,  halfGauge}, // tył P
            {-xOff, yOffset, -halfGauge}  // tył L
        };
        glUseProgram(carShader);
        glUniformMatrix4fv(glGetUniformLocation(carShader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(carShader,"projection"),1,GL_FALSE,glm::value_ptr(projection));
        glUniform3f(glGetUniformLocation(carShader, "uColor"), 0.0f, 0.0f, 0.0f);
        glBindVertexArray(wheelVAO);
        for (int i=0;i<4;++i){
            glm::mat4 M = carBase;
            M *= glm::translate(glm::mat4(1.0f), offs[i]);
            M *= glm::scale(glm::mat4(1.0f), glm::vec3(wheelR, wheelR, wheelT));
            glUniformMatrix4fv(glGetUniformLocation(carShader,"model"),1,GL_FALSE,glm::value_ptr(M));
            glDrawElements(GL_TRIANGLES, wheelIndexCount, GL_UNSIGNED_INT, nullptr);
        }
        glBindVertexArray(0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Sprzątanie
    context.terrain.releaseGL();
    track.releaseGL();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteProgram(terrainProgram);
    glDeleteProgram(skyProgram);
    if (skyVAO) glDeleteVertexArrays(1, &skyVAO);
    if (skyVBO) glDeleteBuffers(1, &skyVBO);
    glDeleteProgram(trackProgram);
    glDeleteProgram(carShader);
    GLuint texToDelete[5] = {texGrassD, texGrassS, texGrassN, texSteelD, texSteelS};
    glDeleteTextures(5, texToDelete);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &cubeEBO);
    if (wheelVAO) glDeleteVertexArrays(1, &wheelVAO);
    if (wheelVBO) glDeleteBuffers(1, &wheelVBO);
    if (wheelEBO) glDeleteBuffers(1, &wheelEBO);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
