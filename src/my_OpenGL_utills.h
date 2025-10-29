#include <GL/glew.h>
#include <string>
#include <optional>
#include <fstream>
#include <sstream>
#include <iostream>

#ifndef SHADER_H
#define SHADER_H
std::string loadFileToString(const char* filepath) {
    std::string file_text;
    std::ifstream file;

    file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        file.open(filepath);
        std::stringstream file_string_stream;

        file_string_stream << file.rdbuf();

        file.close();

        file_text = file_string_stream.str();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Failed to open: " << filepath << "\n" << e.what() << '\n';
    }
    return file_text;
}
#endif

#ifndef OPENGL_COMP_CHECKER27
#define OPENGL_COMP_CHECKER27
namespace ShaderUtils {
    inline std::optional<std::string> getCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];

        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                return "ERROR::SHADER_COMPILATION_ERROR of type: " + type + "\n" + infoLog + "\n -- --------------------------------------------------- -- \n";
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                return "ERROR::PROGRAM_LINKING_ERROR of type: " + type + "\n" + infoLog + "\n -- --------------------------------------------------- -- \n" ;
            }
        }
        return std::nullopt;
    }

    inline void coutCompileErrors(unsigned int shader, std::string type) {
    auto res = ShaderUtils::getCompileErrors(shader, type);
        if (res) { std::cout << res.value(); };
    }
}
#endif