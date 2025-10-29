namespace ourShaders {
static const char* vertexShaderSrc = R"glsl(

#version 330 core

layout (location = 0) in vec3 aPos;   // the position variable has attribute position 0
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

void main() {
    gl_Position = vec4(aPos, 1);
    ourColor = aColor;
}
)glsl";

static const char* fragmentShaderSrc = R"glsl(
#version 330 core

out vec4 FragColor;

in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)glsl";
}