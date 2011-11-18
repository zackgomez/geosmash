#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

struct anim_frame;

struct mesh
{
    GLuint data_buffer;
    size_t nverts;
};

GLuint make_buffer( GLenum target, const void *buffer_data, GLsizei buffer_size);

void show_info_log( GLuint object, PFNGLGETSHADERIVPROC glGet__iv, PFNGLGETSHADERINFOLOGPROC glGet__InfoLog);

GLuint make_shader(GLenum type, const char *filename);

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader);

GLuint make_texture(const char *filename);

bool initGLUtils(int screenw, int screenh);
void cleanGLUtils();

void renderRectangle(const glm::mat4 &transform, const glm::vec4 &color);
void renderTexturedRectangle(const glm::mat4 &transform, GLuint texture);
void renderMaskedRectangle(const glm::mat4 &transform, const glm::vec4 &color,
        const anim_frame *frame);

void preRender();
void postRender();

// For now just creates a cube
mesh createMesh(std::string objfile);
void renderMesh(const mesh &m, const glm::mat4 &trans, const glm::vec3 &color);

// Matrix operations
void setProjectionMatrix(const glm::mat4 &mat);
void setViewMatrix(const glm::mat4 &mat);
// The model matrix is passed in to the render call

const glm::mat4& getProjectionMatrix();
const glm::mat4& getViewMatrix();
