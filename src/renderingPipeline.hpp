// Minimal OpenGL example using GLFW and GLEW
// #include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "my_OpenGL_utills.h"
#include "vertexAndFrag.h"

int s_width = 1200; // Width of window in pixels
int s_height = 900; // Height of window in pixels

struct vertex {
    float pos[3];
    float col[3];
};



vertex vertices[] = {
    {-0.5, -0.5, 0.0, 1.0, 0.0, 0.0}, // b l
    {0.5, -0.5, 0.0, 0.0, 1.0, 0.0},  // b r
    {0.0, 0.5, 0.0, 0.0, 0.0, 1.0},

    {0.5, 0.5, 0.0, 0.0, 0.0, 1.0},   // t r
    {-0.5, 0.5, 0.0, 0.0, 0.0, 1.0},  // t l
};

// Indices for drawing the triangle
unsigned int indices[] = { 0, 1, 2 };

unsigned int box_indices[] = {0, 1, 3, 0, 4, 3};


void framebuffer_size_callback(GLFWwindow* window, int s_width
, int s_height)
{
    glViewport(0, 0, s_width
    , s_height);
}  

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main(int argc, char** argv)
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Create a window.
    GLFWwindow *window = glfwCreateWindow(s_width
    , s_height, "minimal OpenGL example", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    glewInit();

    glViewport(0, 0, s_width, s_height);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  

    unsigned int vertexShader, fragmentShader, shaderProgram;
    // Compile and check vertex shader.
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &ourShaders::vertexShaderSrc, NULL);
    glCompileShader(vertexShader);

    ShaderUtils::coutCompileErrors(vertexShader, "SHADER");

    // Compile and check fragment shader.
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &ourShaders::fragmentShaderSrc, NULL);
    glCompileShader(fragmentShader);

    ShaderUtils::coutCompileErrors(fragmentShader, "SHADER");

    // Link and check shader program.
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    ShaderUtils::coutCompileErrors(shaderProgram, "PROGRAM");

    unsigned int VBO, VAO, EBO;
    glGenBuffers(1, &VBO);  //video buffer object reference
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO); // binds to the opengl array buffer, bind 0 to reset
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // loads data into the buffer

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), box_indices, GL_STATIC_DRAW);
    
    glBindVertexArray(0); 
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.


    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // can use wireframe mode

    glEnable(GL_DEPTH_TEST);

    // Switch to the shader program.
    glUseProgram(shaderProgram);


    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(window)) {
        // Clear color buffer and depth buffer.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Switch to the shader program.
        glUseProgram(shaderProgram);
        // Draw triangle(s).
        glBindVertexArray(VAO);
        // glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0);
        // Swap front and back buffers.
        glfwSwapBuffers(window);
        // Poll for and process events.
        glfwPollEvents();
    };
    glfwTerminate();
    return 0;
}
