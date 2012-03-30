#ifndef _INCLUDE_ZGFX_H_
#define _INCLUDE_ZGFX_H_

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

// Data structures
struct mesh
{
    glm::mat4 transform;

    GLuint data_buffer;
    size_t nverts;
    uint8_t vertformat;
};

// Vertex formats
const uint8_t VERT_P4N4T2 = 1;
struct vert_p4n4t2
{
    float pos[4];
    float norm[4];
    float texcoord[2];
};

// OpenGL convenience functions
GLuint make_buffer(GLenum target, const void *buffer_data, GLsizei buffer_size);
GLuint make_texture(const char *filename);
void   free_texture(GLuint tex);


// Mesh functions
mesh * loadMesh(const std::string &objfile, bool normalize = false);
void renderMesh(const glm::mat4 &projMatrix, const glm::mat4 &viewMatrix,
        const glm::mat4 &modelMatrix, const mesh *m, GLuint program);
void freeMesh(mesh *mesh);

#endif
