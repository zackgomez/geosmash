#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

GLuint make_buffer( GLenum target, const void *buffer_data, GLsizei buffer_size);

void show_info_log( GLuint object, PFNGLGETSHADERIVPROC glGet__iv, PFNGLGETSHADERINFOLOGPROC glGet__InfoLog);

GLuint make_shader(GLenum type, const char *filename);

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader);

GLuint make_texture(const char *filename);

GLuint loadAnimFrame(const char *filename);

bool initGLUtils(const glm::mat4 &perspectiveTrans);
void cleanGLUtils();

void renderRectangle(const glm::mat4 &transform, const glm::vec3 &color);
void renderTexturedRectangle(const glm::mat4 &transform, GLuint texture);
