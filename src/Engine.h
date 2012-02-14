#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include "MatrixStack.h"
#include "kiss-skeleton.h"

struct anim_frame;

struct mesh
{
    GLuint data_buffer;
    size_t nverts;
};

GLuint make_buffer( GLenum target, const void *buffer_data, GLsizei buffer_size);

void show_info_log( GLuint object, PFNGLGETSHADERIVPROC glGet__iv, PFNGLGETSHADERINFOLOGPROC glGet__InfoLog);

GLuint make_shader(GLenum type, const char *filename);

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader, 
        GLuint geometry_shader = 0);

GLuint make_texture(const char *filename);
void   free_texture(GLuint tex);

bool initGLUtils(int screenw, int screenh);
void cleanGLUtils();

void renderRectangle(const glm::mat4 &transform, const glm::vec4 &color);
void renderRectangleProgram(const glm::mat4 &transform, GLuint program);
void renderTexturedRectangle(const glm::mat4 &transform, GLuint texture);
void renderMaskedRectangle(const glm::mat4 &transform, const glm::vec4 &color,
        const anim_frame *frame);

void preRender();
void postRender();

// Returns a millisecond resolution time
int getCurrentMillis();

// For now just creates a cube
mesh createMesh(std::string objfile);
void renderMesh(const glm::mat4 &modelMatrix, const mesh &m, GLuint program);

// Matrix operations
MatrixStack& getProjectionMatrixStack();
MatrixStack& getViewMatrixStack();
void setCamera(const glm::vec3 &pos);
// The model matrix is passed in to the render call


class GeosmashBoneRenderer : public BoneRenderer
{
public:
    void setColor(const glm::vec4 &color);
    virtual void operator() (const glm::mat4 &transform, const Bone* b);

private:
    glm::vec4 color_;
};
