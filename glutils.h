#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

struct anim_frame
{
    std::string id;
    GLuint mask_tex;
    float w, h; // game units
    float x, y; // Center inside texture, [-1, 1]
};

GLuint make_buffer( GLenum target, const void *buffer_data, GLsizei buffer_size);

void show_info_log( GLuint object, PFNGLGETSHADERIVPROC glGet__iv, PFNGLGETSHADERINFOLOGPROC glGet__InfoLog);

GLuint make_shader(GLenum type, const char *filename);

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader);

GLuint make_texture(const char *filename);

anim_frame* loadAnimFrame(const char *filename);

bool initGLUtils(const glm::mat4 &perspectiveTrans);
void cleanGLUtils();

void renderRectangle(const glm::mat4 &transform, const glm::vec3 &color);
void renderTexturedRectangle(const glm::mat4 &transform, GLuint texture);
void renderFrame(const glm::mat4 &transform, const glm::vec3 &color,
        const anim_frame *frame);
void renderMaskedRectangle(const glm::mat4 &transform, const glm::vec3 &color,
        GLuint texture);
