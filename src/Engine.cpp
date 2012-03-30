#include "Engine.h"
#include <GL/glew.h>
#include <SDL/SDL.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include "util.h"
#include "FrameManager.h"
#include "ParamReader.h"
#include "stb_image.c"
#include "PManager.h"
#include "Logger.h"

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

    GLuint particleprogram;
    GLuint part_buffer;

    GLuint boneprogram;

    // XXX remove this... it's only used by geosmash bone renderer
    mesh *cubemesh;

    int plane_res;
    GLuint plane_buffer;
    GLuint **plane_indices;

    GLuint fbo;
    GLuint depthbuf;
    GLuint rendertex[3];
} resources;

static LoggerPtr logger;

static glm::vec2 screensize;
static MatrixStack projectionMatrixStack;
static MatrixStack viewMatrixStack;

GLuint make_buffer(GLenum target, const void *buffer_data, GLsizei buffer_size)
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
    logger->fatal() << log << '\n';
    free(log);
}

bool checkFramebufferStatus() {
    GLenum status;
    status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
    case GL_FRAMEBUFFER_COMPLETE_EXT:
        return true;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        logger->warning() << "Framebuffer incomplete, incomplete attachment\n";
        return false;
    case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
        logger->warning() << "Unsupported framebuffer format\n";
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        logger->warning() << "Framebuffer incomplete, missing attachment\n";
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        logger->warning() << "Framebuffer incomplete,  attachments must have same dimensions\n";
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        logger->warning() << "Framebuffer incomplete, attached images must have same format\n";
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        logger->warning() << "Framebuffer incomplete, missing draw buffer\n";
        return false;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        logger->warning() << "Framebuffer incomplete, missing read buffer\n";
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
        logger->fatal() << "Failed to compile " << filename << ":\n";
        show_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint make_program(const char *vertfile, const char *fragfile)
{
    GLuint vert = make_shader(GL_VERTEX_SHADER, vertfile);
    GLuint frag = make_shader(GL_FRAGMENT_SHADER, fragfile);
    if (!vert || !frag)
    {
        std::cerr << "Unable to read shaders\n";
        exit(1);
    }
    GLuint program = make_program(vert, frag);
    assert(program);
    return program;
}

GLuint make_program(GLuint vertex_shader, GLuint fragment_shader, GLuint geometry_shader)
{
    GLuint program = glCreateProgram();

    glAttachShader(program, geometry_shader);
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint program_ok;
    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    if (!program_ok) {
        logger->fatal() << "Failed to link shader program:\n";
        show_info_log(program, glGetProgramiv, glGetProgramInfoLog);
        glDeleteProgram(program);
        return 0;
    }
    return program;
} 

GLuint make_texture(const char *filename)
{
    int width, height, depth;
    void *pixels = stbi_load(filename, &width, &height, &depth, 4);
    GLuint texture;

    if (!pixels)
    {
        logger->warning() << "Unable to load texture from " << filename << "\n";
        return 0;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);
    glTexImage2D(
            GL_TEXTURE_2D, 0,           /* target, level */
            GL_RGBA8,                   /* internal format */
            width, height, 0,           /* width, height, border */
            GL_RGBA, GL_UNSIGNED_BYTE,   /* external format, type */
            pixels                      /* pixels */
            );
    stbi_image_free(pixels);
    return texture;
}

void free_texture(GLuint tex)
{
    glDeleteTextures(1, &tex);
}

void blurTexture(GLuint texture, bool horiz)
{
    GLuint program = horiz ? resources.hblurprogram : resources.vblurprogram;
    GLuint textureUniform = glGetUniformLocation(program, "tex");
    GLuint sizeUniform = glGetUniformLocation(program, "texsize");
    GLuint positionAttrib = glGetAttribLocation(program, "position");

    // Enable program and set up values
    glUseProgram(program);
    glUniform1i(textureUniform, 0);
    glUniform2fv(sizeUniform, 1, glm::value_ptr(screensize));

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // Clean up
    glDisableVertexAttribArray(positionAttrib);
    glUseProgram(0);

}

void preRender()
{
    GLenum MRTBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);
    glDrawBuffers(2, MRTBuffers);
    checkFramebufferStatus();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
}

void postRender()
{
    projectionMatrixStack.push();
    viewMatrixStack.push();
    projectionMatrixStack.current() = viewMatrixStack.current() = glm::mat4(1.f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    if (getParam("engine.useBlur"))
    {
        // Set the viewport to be the size of the blur textures
        //glViewport(0, 0, getParam("engine.blurW"), getParam("engine.blurH"));
        // Copy glow buffer into blur texture
        // blur the glow texture
        for (int i = 0; i < 3; i++)
        {
            glDrawBuffer(GL_COLOR_ATTACHMENT2);
            blurTexture(resources.rendertex[1], true);
            glDrawBuffer(GL_COLOR_ATTACHMENT1);
            blurTexture(resources.rendertex[2], false);
        }
        // Reset viewport
        //glViewport(0, 0, screensize.x, screensize.y);
    }
    checkFramebufferStatus();

    // Render to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // clear the screen
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Draw normal colors
    renderTexturedRectangle(glm::scale(glm::mat4(1.0f), glm::vec3(2.f, 2.f, 1.f)),
            resources.rendertex[0]);

    // Blend in glow
    if (getParam("engine.useBlur"))
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        renderTexturedRectangle(glm::scale(glm::mat4(1.0f), glm::vec3(2.f, 2.f, 1.f)),
                resources.rendertex[1]);
        glDisable(GL_BLEND);
    }

    projectionMatrixStack.pop();
    viewMatrixStack.pop();
}

bool initPlane()
{
    // Create a mesh [-1, 1] with some number of vertices
    int meshRes = getParam("engine.planeRes");
    resources.plane_res = meshRes;
    float *mesh = new float[2 * meshRes * meshRes];
    for (int y = 0; y < meshRes; y++)
    {
        for (int x = 0; x < meshRes; x++)
        {
            int ind = 2*(y*meshRes + x);
            // Map to [-1, 1]
            mesh[ind]     = (x - meshRes/2.f) / (meshRes-1) * 10.f;
            mesh[ind + 1] = (y - meshRes/2.f) / (meshRes-1) * 10.f;
        }
    }
    resources.plane_buffer = make_buffer(GL_ARRAY_BUFFER, mesh, sizeof(float) * meshRes * meshRes * 2);
    delete[] mesh;

    // Create the element indices for the mesh
    resources.plane_indices = new GLuint*[meshRes - 1];
    for (int i = 0; i < meshRes - 1; i++)
    {
        resources.plane_indices[i] = new GLuint[meshRes*2];
        GLuint *array = resources.plane_indices[i];
        for (int j = 0; j < meshRes; j++)
        {
            array[2*j + 0] = i * meshRes + j;
            array[2*j + 1] = (i + 1) * meshRes + j;
        }
    }

    return true;
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

    logger = Logger::getLogger("Engine");

    screensize = glm::vec2(screenw, screenh);

    resources.vertex_buffer = make_buffer(GL_ARRAY_BUFFER, vertex_buffer_data, sizeof(vertex_buffer_data));
    resources.element_buffer = make_buffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_data, sizeof(element_buffer_data));
    glGenBuffers(1, &resources.part_buffer);

    resources.vertex_shader = make_shader(GL_VERTEX_SHADER, "shaders/bbox.v.glsl");
    if (resources.vertex_shader == 0)
        return false;
    resources.fragment_shader = make_shader(GL_FRAGMENT_SHADER, "shaders/bbox.f.glsl");
    if (resources.fragment_shader == 0)
        return false;

    resources.program = make_program(resources.vertex_shader, resources.fragment_shader);
    
    resources.texvertex_shader = make_shader(GL_VERTEX_SHADER, "shaders/texbox.v.glsl");
    if (resources.texvertex_shader == 0)
        return false;
    resources.texfragment_shader = make_shader(GL_FRAGMENT_SHADER, "shaders/texbox.f.glsl");
    if (resources.texfragment_shader == 0)
        return false;
    resources.texprogram = make_program(resources.texvertex_shader, resources.texfragment_shader);

    resources.maskvertex_shader = make_shader(GL_VERTEX_SHADER, "shaders/texbox.v.glsl");
    if (resources.maskvertex_shader == 0)
        return false;
    resources.maskfragment_shader = make_shader(GL_FRAGMENT_SHADER, "shaders/maskbox.f.glsl");
    if (resources.maskfragment_shader == 0)
        return false;
    resources.maskprogram = make_program(resources.maskvertex_shader, resources.maskfragment_shader);

    GLuint blurvertex_shader = make_shader(GL_VERTEX_SHADER, "shaders/blur.v.glsl");
    GLuint hblurfrag_shader = make_shader(GL_FRAGMENT_SHADER, "shaders/hblur.f.glsl");
    GLuint vblurfrag_shader = make_shader(GL_FRAGMENT_SHADER, "shaders/vblur.f.glsl");

    if (!blurvertex_shader || !hblurfrag_shader || !vblurfrag_shader)
        return false;
    resources.hblurprogram = make_program(blurvertex_shader, hblurfrag_shader);
    resources.vblurprogram = make_program(blurvertex_shader, vblurfrag_shader);

    // PARTICLE SHADER
    GLuint partfrag;
    GLuint partvert;
    if (GLEW_EXT_geometry_shader4)
    {
        GLuint partgeom = 0;
        partfrag = make_shader(GL_VERTEX_SHADER, "shaders/particle.v.glsl");
        partvert = make_shader(GL_FRAGMENT_SHADER, "shaders/particle.f.glsl");
        if (!partfrag || !partvert)
            return false;

        logger->info() << "Using geometry shader for particles\n";
        partgeom = make_shader(GL_GEOMETRY_SHADER, "shaders/particle.g.glsl");
        if (!partgeom)
            return false;
        resources.particleprogram = glCreateProgram();
        glAttachShader(resources.particleprogram, partvert);
        glAttachShader(resources.particleprogram, partgeom);
        glAttachShader(resources.particleprogram, partfrag);
        glProgramParameteriEXT(resources.particleprogram, GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
        glProgramParameteriEXT(resources.particleprogram, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
        glProgramParameteriEXT(resources.particleprogram, GL_GEOMETRY_VERTICES_OUT_EXT, 4);
        glLinkProgram(resources.particleprogram);
        GLint program_ok;
        glGetProgramiv(resources.particleprogram, GL_LINK_STATUS, &program_ok);
        if (!program_ok) {
            logger->fatal() << "Failed to link shader program:\n";
            show_info_log(resources.particleprogram, glGetProgramiv, glGetProgramInfoLog);
            glDeleteProgram(resources.particleprogram);
            return false;
        }
    }
    else
    {
        partfrag = make_shader(GL_VERTEX_SHADER, "shaders/particle-point.v.glsl");
        partvert = make_shader(GL_FRAGMENT_SHADER, "shaders/particle-point.f.glsl");
        if (!partfrag || !partvert)
            return false;
        resources.particleprogram = make_program(partfrag, partvert);
        if (!resources.particleprogram)
            return false;
    }


    partfrag = make_shader(GL_VERTEX_SHADER, "shaders/stage.v.glsl");
    partvert = make_shader(GL_FRAGMENT_SHADER, "shaders/stage.f.glsl");
    if (!partfrag || !partvert)
        return false;
    resources.boneprogram = make_program(partvert, partfrag);

    glGenFramebuffers(1, &resources.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);

    resources.cubemesh = loadMesh("models/cube.obj");

    // Set up depth buffer attachment 
    glGenRenderbuffers(1, &resources.depthbuf);
    glBindRenderbuffer(GL_RENDERBUFFER, resources.depthbuf);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, screenw, screenh);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, resources.depthbuf);

    // Create the color attachment textures
    glGenTextures(3, resources.rendertex);
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

    if (!initPlane())
        return false;

    return true;
}

void cleanGLUtils()
{
    glDeleteRenderbuffers(1, &resources.depthbuf);
    glDeleteFramebuffers(1, &resources.fbo);
    glDeleteTextures(3, resources.rendertex);
}

void renderRectangle(const glm::mat4 &modelMatrix, const glm::vec4 &color)
{
    GLuint projectionUniform = glGetUniformLocation(resources.program, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(resources.program, "modelViewMatrix");
    GLuint colorUniform = glGetUniformLocation(resources.program, "color");
    GLuint positionAttrib = glGetAttribLocation(resources.program, "position");

    // Enable program and set up values
    glUseProgram(resources.program);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrixStack.current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrixStack.current() * modelMatrix));
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionAttrib);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Clean up
    glDisableVertexAttribArray(positionAttrib);
    glUseProgram(0);
}

void renderRectangleProgram(const glm::mat4 &modelMatrix, GLuint program)
{
    GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");
    GLuint positionAttrib = glGetAttribLocation(program, "position");

    // Enable program and set up values
    glUseProgram(program);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrixStack.current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrixStack.current() * modelMatrix));

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionAttrib);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Clean up
    glDisableVertexAttribArray(positionAttrib);
    glUseProgram(0);
}

void renderTexturedRectangle(const glm::mat4 &modelMatrix, GLuint texture)
{
    GLuint projectionUniform = glGetUniformLocation(resources.texprogram, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(resources.texprogram, "modelViewMatrix");
    GLuint textureUniform = glGetUniformLocation(resources.texprogram, "texture");
    GLuint positionAttrib = glGetAttribLocation(resources.texprogram, "position");

    // Enable program and set up values
    glUseProgram(resources.texprogram);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrixStack.current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrixStack.current() * modelMatrix));
    glUniform1i(textureUniform, 0);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionAttrib);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Clean up
    glDisableVertexAttribArray(positionAttrib);
    glUseProgram(0);
}

void renderMaskedRectangle(const glm::mat4 &modelMatrix, const glm::vec4 &color,
        const anim_frame *frame)
{
    GLuint projectionUniform = glGetUniformLocation(resources.maskprogram, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(resources.maskprogram, "modelViewMatrix");
    GLuint textureUniform = glGetUniformLocation(resources.maskprogram, "texture");
    GLuint colorUniform = glGetUniformLocation(resources.maskprogram, "color");
    GLuint texsizeUniform = glGetUniformLocation(resources.maskprogram, "texsize");
    GLuint positionAttrib = glGetAttribLocation(resources.maskprogram, "position");

    // Enable program and set up values
    glUseProgram(resources.maskprogram);
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrixStack.current() * modelMatrix));
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrixStack.current()));
    glUniform1i(textureUniform, 0);
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));
    glUniform2fv(texsizeUniform, 1, glm::value_ptr(glm::vec2(frame->w, frame->h)));

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, frame->mask_tex);

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Clean up
    glDisableVertexAttribArray(positionAttrib);
    glUseProgram(0);
}

void renderMesh(const glm::mat4 &modelMatrix, const mesh *m, GLuint program)
{
    renderMesh(projectionMatrixStack.current(), viewMatrixStack.current(),
            modelMatrix, m, program);
}

void setCamera(const glm::vec3 &pos)
{
    projectionMatrixStack.current() =
        glm::perspective(getParam("camera.fov"), 16.f / 9.f, 0.1f, 10000.f);
    viewMatrixStack.current() =
        glm::translate(glm::mat4(1.f), -pos);
}

MatrixStack & getProjectionMatrixStack()
{
    return projectionMatrixStack;
}

MatrixStack & getViewMatrixStack()
{
    return viewMatrixStack;
}

void renderParticles(const std::vector<Particle> &data)
{
	if (data.empty()) return;

    GLuint modelViewUniform = glGetUniformLocation(resources.particleprogram, "modelViewMatrix");
    GLuint projectionUniform = glGetUniformLocation(resources.particleprogram, "projectionMatrix");
    GLuint positionAttrib = glGetAttribLocation(resources.particleprogram, "position");
    GLuint colorAttrib = glGetAttribLocation(resources.particleprogram, "color");
    GLuint sizeAttrib = glGetAttribLocation(resources.particleprogram, "size");
    GLuint lifeAttrib = glGetAttribLocation(resources.particleprogram, "lifetime");
    GLuint velocityAttrib = glGetAttribLocation(resources.particleprogram, "velocity");

    glUseProgram(resources.particleprogram);
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrixStack.current()));
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrixStack.current()));

    glBindBuffer(GL_ARRAY_BUFFER, resources.part_buffer);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(Particle), &data.front(), GL_STREAM_DRAW);

    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, loc));
    glEnableVertexAttribArray(colorAttrib);
    glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, color));
    glEnableVertexAttribArray(sizeAttrib);
    glVertexAttribPointer(sizeAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, size));
    glEnableVertexAttribArray(velocityAttrib);
    glVertexAttribPointer(velocityAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, vel));
    glEnableVertexAttribArray(lifeAttrib);
    glVertexAttribPointer(lifeAttrib, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, t));

    glDrawArrays(GL_POINTS, 0, data.size());

    glDisableVertexAttribArray(positionAttrib);
    glDisableVertexAttribArray(colorAttrib);
    glDisableVertexAttribArray(sizeAttrib);
    glDisableVertexAttribArray(lifeAttrib);
    glDisableVertexAttribArray(velocityAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void GeosmashBoneRenderer::setColor(const glm::vec4 &color)
{
    color_ = color;
}

void GeosmashBoneRenderer::operator() (const glm::mat4 &transform, const Bone* b)
{
    glm::mat4 fullTransform = glm::scale(transform, glm::vec3(b->length, 0.05, 0.05));
    fullTransform = glm::translate(fullTransform, glm::vec3(0.5f, 0.f, 0.f));

    GLuint colorUniform = glGetUniformLocation(resources.boneprogram, "color");
    glUseProgram(resources.boneprogram);
    glUniform4fv(colorUniform, 1, glm::value_ptr(color_));
    glUseProgram(0);

    renderMesh(fullTransform, resources.cubemesh, resources.boneprogram);
}

int getCurrentMillis()
{
    return SDL_GetTicks();
}

std::string getTimeString()
{
    char buf[100];
    time_t curtime = time(NULL);
    strftime(buf, sizeof(buf), "%Y%m%d%M%S", localtime(&curtime));

    return std::string(buf);
}

void renderPlane(const glm::mat4& transform, GLuint program)
{
    GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");
    GLuint positionAttrib = glGetAttribLocation(program, "position");

    glUseProgram(program);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(getProjectionMatrixStack().current()));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(getViewMatrixStack().current() * transform));

    glBindBuffer(GL_ARRAY_BUFFER, resources.plane_buffer);
    glEnableVertexAttribArray(positionAttrib);
    glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, (void*)0);

    for (int i = 0; i < resources.plane_res - 1; i++)
        glDrawElements(GL_TRIANGLE_STRIP, resources.plane_res*2, GL_UNSIGNED_INT,
                resources.plane_indices[i]);

    glDisableVertexAttribArray(positionAttrib);
    glUseProgram(0);
}

