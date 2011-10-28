#include <GL/glew.h>
#include <cstdlib>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util.h"
#include "glutils.h"

static struct 
{
    GLuint vertex_buffer, element_buffer;
    GLuint vertex_shader, fragment_shader;
    GLuint program;
} resources;


GLuint make_buffer( GLenum target, const void *buffer_data, GLsizei buffer_size)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(target, buffer);
    glBufferData(target, buffer_size, buffer_data, GL_STATIC_DRAW);
    return buffer;
}

void show_info_log(
        GLuint object,
        PFNGLGETSHADERIVPROC glGet__iv,
        PFNGLGETSHADERINFOLOGPROC glGet__InfoLog
        )
{
    GLint log_length;
    char *log;

    glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
    log = (char*) malloc(log_length);
    glGet__InfoLog(object, log_length, NULL, log);
    fprintf(stderr, "%s", log);
    free(log);
}

GLuint make_shader(GLenum type, const char *filename)
{
    GLint length;
    GLchar *source = (GLchar *)file_contents(filename, &length);
    GLuint shader;
    GLint shader_ok;

    if (!source)
        return 0;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar**)&source, &length);
    free(source);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    if (!shader_ok) {
        fprintf(stderr, "Failed to compile %s:\n", filename);
        show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLint program_ok;

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    if (!program_ok) {
        fprintf(stderr, "Failed to link shader program:\n");
        show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

bool initGLUtils()
{
    // The datas
    const GLfloat vertex_buffer_data[] = { 
        // Position Data
        -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 1.0f,
        0.5f,  0.5f, 0.0f, 1.0f
    };
    const GLushort element_buffer_data[] = { 0, 1, 2, 3 };

    resources.vertex_buffer = make_buffer(GL_ARRAY_BUFFER, vertex_buffer_data, sizeof(vertex_buffer_data));
    resources.element_buffer = make_buffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_data, sizeof(element_buffer_data));

    resources.vertex_shader = make_shader(GL_VERTEX_SHADER, "bbox.v.glsl");
    if (resources.vertex_shader == 0)
        return false;
    resources.fragment_shader = make_shader(GL_FRAGMENT_SHADER, "bbox.f.glsl");
    if (resources.fragment_shader == 0)
        return false;

    resources.program = make_program(resources.vertex_shader, resources.fragment_shader);

    return true;
}

void cleanGLUtils()
{
}

void renderRectangle(const glm::mat4 &transform, const glm::vec3 &color)
{
    GLuint transformUniform = glGetUniformLocation(resources.program, "transform");
    GLuint colorUniform = glGetUniformLocation(resources.program, "color");

    // Enable program and set up values
    glUseProgram(resources.program);
    glUniformMatrix4fv(transformUniform, 1, GL_FALSE, glm::value_ptr(transform));
    glUniform3fv(colorUniform, 1, glm::value_ptr(color));

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // Clean up
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}
