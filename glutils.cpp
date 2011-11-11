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

    GLuint hblurprogram, vblurprogram;

    GLuint fbo;
    GLuint depthbuf;
    GLuint rendertex[3];

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

bool checkFramebufferStatus() {
    GLenum status;
    status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
        return true;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        printf("Framebuffer incomplete, incomplete attachment\n");
        return false;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        printf("Unsupported framebuffer format\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        printf("Framebuffer incomplete, missing attachment\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        printf("Framebuffer incomplete,  attachments must have same dimensions\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        printf("Framebuffer incomplete, attached images must have same format\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        printf("Framebuffer incomplete, missing draw buffer\n");
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        printf("Framebuffer incomplete, missing read buffer\n");
        return false;
    }
    return false;
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

void setPerspective(const glm::mat4 &perspectiveTransform)
{
    resources.perspective = perspectiveTransform;
}

void preRender()
{
    GLenum MRTBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);
    glDrawBuffers(2, MRTBuffers);
    checkFramebufferStatus();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void postRender()
{

    // TODO texture this shit


    checkFramebufferStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);

    glm::mat4 old = resources.perspective;
    resources.perspective = glm::mat4(1.0f);

    renderTexturedRectangle(glm::scale(glm::mat4(1.0f), glm::vec3(2.f, 2.f, 1.f)),
            resources.rendertex[0]);

    resources.perspective = old;
}

bool initGLUtils(int screenw, int screenh)
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

    resources.perspective = glm::mat4(1.0f);

    GLuint blurvertex_shader = make_shader(GL_VERTEX_SHADER, "blur.v.glsl");
    GLuint hblurfrag_shader = make_shader(GL_VERTEX_SHADER, "hblur.f.glsl");
    GLuint vblurfrag_shader = make_shader(GL_VERTEX_SHADER, "vblur.f.glsl");

    if (!blurvertex_shader || !hblurfrag_shader || !vblurfrag_shader)
        return false;

    resources.hblurprogram = make_program(blurvertex_shader, hblurfrag_shader);
    resources.vblurprogram = make_program(blurvertex_shader, vblurfrag_shader);

    glGenFramebuffers(1, &resources.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);

    // Set up depth buffer attachment 
    glGenRenderbuffers(1, &resources.depthbuf);
    glBindRenderbuffer(GL_RENDERBUFFER, resources.depthbuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenw, screenh);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, resources.depthbuf);

    // Create the color attachment textures
    glGenTextures(2, resources.rendertex);
    // main
    glBindTexture(GL_TEXTURE_2D, resources.rendertex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenw, screenh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glow
    glBindTexture(GL_TEXTURE_2D, resources.rendertex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenw, screenh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glow blur
    glBindTexture(GL_TEXTURE_2D, resources.rendertex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenw, screenh, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // Bind textures
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resources.rendertex[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, resources.rendertex[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, resources.rendertex[2], 0);

    if (!checkFramebufferStatus())
        return false;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void cleanGLUtils()
{
    glDeleteRenderbuffers(1, &resources.depthbuf);
    glDeleteFramebuffers(1, &resources.fbo);
    glDeleteTextures(3, resources.rendertex);
}

void renderRectangle(const glm::mat4 &transform, const glm::vec4 &color)
{
    GLuint transformUniform = glGetUniformLocation(resources.program, "transform");
    GLuint colorUniform = glGetUniformLocation(resources.program, "color");

    // Enable program and set up values
    glUseProgram(resources.program);
    glUniformMatrix4fv(transformUniform, 1, GL_FALSE, glm::value_ptr(resources.perspective * transform));
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));

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

void renderMaskedRectangle(const glm::mat4 &transform, const glm::vec4 &color,
        GLuint mask)
{
    GLuint transformUniform = glGetUniformLocation(resources.maskprogram, "transform");
    GLuint textureUniform = glGetUniformLocation(resources.maskprogram, "texture");
    GLuint colorUniform = glGetUniformLocation(resources.maskprogram, "color");

    // Enable fag program and set up values
    glUseProgram(resources.maskprogram);
    glUniformMatrix4fv(transformUniform, 1, GL_FALSE, glm::value_ptr(resources.perspective * transform));
    glUniform1i(textureUniform, 0);
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));

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

