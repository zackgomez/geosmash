#include <GL/glew.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include "util.h"
#include "glutils.h"

static struct 
{
    GLuint vertex_buffer, element_buffer;
    GLuint vertex_shader, fragment_shader;
    GLuint program;
    GLuint texvertex_shader, texfragment_shader;
    GLuint texprogram;

    GLuint maskvertex_shader, maskfragment_shader;
    GLuint maskprogram;

    glm::mat4 perspective;
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

GLuint make_texture(const char *filename)
{
    int width, height;
    void *pixels = read_tga(filename, &width, &height);
    GLuint texture;

    if (!pixels)
        return 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexImage2D(
            GL_TEXTURE_2D, 0,           /* target, level */
            GL_RGB8,                    /* internal format */
            width, height, 0,           /* width, height, border */
            GL_BGR, GL_UNSIGNED_BYTE,   /* external format, type */
            pixels                      /* pixels */
            );
    free(pixels);
    return texture;
}

anim_frame* loadAnimFrame(const char *filename)
{
    // File format is
    // name (oneword)
    // w h
    // x y
    // DATA
    // ...
    GLubyte *data;
    int w, h;
    float x, y;
    std::string name;

    std::cout << "Loading frame in " << filename << '\n';
    std::ifstream file(filename);
    std::stringstream ss;
    std::string line;
    if (!file)
    {
        std::cerr << "Unable to load file: " << filename << '\n';
        return 0;
    }

    // First line is the name
    std::getline(file, name);

    // Next is the h/w
    std::getline(file, line);
    if (sscanf(line.c_str(), "%d %d\n", &w, &h) != 2)
    {
        std::cerr << "Unable to load w,h from file: " << filename << '\n';
        return 0;
    }
    // Next the center
    std::getline(file, line);
    if (sscanf(line.c_str(), "%f %f\n", &x, &y) != 2)
    {
        std::cerr << "Unable to load x,y from file: " << filename << '\n';
        return 0;
    }

    data = (GLubyte *) malloc(w * h * sizeof(GLubyte));

    char c;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            file >> c;
            if (c != '0' && c != '1')
            {
                std::cerr << "Bad character in file: " << c << '\n';
                return 0;
            }
            else
            {
                data[x + w*(h - 1 - y)] = (c == '0' ? 0 : 255);
            }
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexImage2D(
            GL_TEXTURE_2D, 0,
            1,
            w, h, 0,
            GL_RED, GL_UNSIGNED_BYTE,
            data);

    free(data);

    anim_frame *ret = new anim_frame;
    ret->id = name;
    ret->w = w*10.0f; ret->h = h*10.0f;
    ret->x = x; ret->y = y;
    ret->mask_tex = tex;

    return ret;
}

bool initGLUtils(const glm::mat4 &perspectiveTransform)
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
    
    resources.texvertex_shader = make_shader(GL_VERTEX_SHADER, "texbox.v.glsl");
    if (resources.texvertex_shader == 0)
        return false;
    resources.texfragment_shader = make_shader(GL_FRAGMENT_SHADER, "texbox.f.glsl");
    if (resources.texfragment_shader == 0)
        return false;
    resources.texprogram = make_program(resources.texvertex_shader, resources.texfragment_shader);

    resources.maskvertex_shader = make_shader(GL_VERTEX_SHADER, "texbox.v.glsl");
    if (resources.maskvertex_shader == 0)
        return false;
    resources.maskfragment_shader = make_shader(GL_FRAGMENT_SHADER, "maskbox.f.glsl");
    if (resources.maskfragment_shader == 0)
        return false;
    resources.maskprogram = make_program(resources.maskvertex_shader, resources.maskfragment_shader);

    resources.perspective = perspectiveTransform;

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
    glUniformMatrix4fv(transformUniform, 1, GL_FALSE, glm::value_ptr(resources.perspective * transform));
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

void renderTexturedRectangle(const glm::mat4 &transform, GLuint texture)
{
    GLuint transformUniform = glGetUniformLocation(resources.texprogram, "transform");
    GLuint textureUniform = glGetUniformLocation(resources.texprogram, "texture");

    // Enable program and set up values
    glUseProgram(resources.texprogram);
    glUniformMatrix4fv(transformUniform, 1, GL_FALSE, glm::value_ptr(resources.perspective * transform));
    glUniform1i(textureUniform, 0);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // Clean up
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

void renderMaskedRectangle(const glm::mat4 &transform, const glm::vec3 &color,
        GLuint mask)
{
    GLuint transformUniform = glGetUniformLocation(resources.maskprogram, "transform");
    GLuint textureUniform = glGetUniformLocation(resources.maskprogram, "texture");
    GLuint colorUniform = glGetUniformLocation(resources.maskprogram, "color");

    // Enable program and set up values
    glUseProgram(resources.maskprogram);
    glUniformMatrix4fv(transformUniform, 1, GL_FALSE, glm::value_ptr(resources.perspective * transform));
    glUniform1i(textureUniform, 0);
    glUniform3fv(colorUniform, 1, glm::value_ptr(color));

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mask);

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // Clean up
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

void renderFrame(const glm::mat4 &transform, const glm::vec3 &color, const anim_frame *frame)
{
    glm::mat4 finalTransform = glm::translate(
            glm::scale(transform, glm::vec3(frame->w/2, frame->h/2, 1.0f)),
            glm::vec3(frame->x, frame->y, 0.0f));

    renderMaskedRectangle(finalTransform, color, frame->mask_tex);
}
