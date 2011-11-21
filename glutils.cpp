#include <GL/glew.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include "util.h"
#include "glutils.h"
#include "FrameManager.h"

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
    GLuint meshprogram;

    GLuint fbo;
    GLuint depthbuf;
    GLuint rendertex[3];
} resources;

static glm::vec2 screensize;
static glm::mat4 projectionMatrix;
static glm::mat4 viewMatrix;

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

void blurTexture(GLuint texture, bool horiz)
{
    GLuint program = horiz ? resources.hblurprogram : resources.vblurprogram;
    GLuint textureUniform = glGetUniformLocation(program, "tex");
    GLuint sizeUniform = glGetUniformLocation(program, "texsize");

    // Enable program and set up values
    glUseProgram(program);
    glUniform1i(textureUniform, 0);
    glUniform2fv(sizeUniform, 1, glm::value_ptr(screensize));

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
    glm::mat4 projMat = projectionMatrix;
    glm::mat4 viewMat = viewMatrix;
    projectionMatrix = viewMatrix = glm::mat4(1.f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    // blur the glow texture
    for (int i = 0; i < 3; i++)
    {
        glDrawBuffer(GL_COLOR_ATTACHMENT2);
        blurTexture(resources.rendertex[1], true);
        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        blurTexture(resources.rendertex[2], false);
    }

    checkFramebufferStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // clear the screen
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    glClear( GL_COLOR_BUFFER_BIT );

    // Draw normal
    renderTexturedRectangle(glm::scale(glm::mat4(1.0f), glm::vec3(2.f, 2.f, 1.f)),
            resources.rendertex[0]);

    // Blend in glow
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    renderTexturedRectangle(glm::scale(glm::mat4(1.0f), glm::vec3(2.f, 2.f, 1.f)),
            resources.rendertex[1]);
    glDisable(GL_BLEND);

    projectionMatrix = projMat;
    viewMatrix = viewMat;
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

    screensize = glm::vec2(screenw, screenh);

    resources.vertex_buffer = make_buffer(GL_ARRAY_BUFFER, vertex_buffer_data, sizeof(vertex_buffer_data));
    resources.element_buffer = make_buffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_data, sizeof(element_buffer_data));

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

    GLuint meshvert = make_shader(GL_VERTEX_SHADER, "shaders/mesh.v.glsl");
    GLuint meshfrag = make_shader(GL_FRAGMENT_SHADER, "shaders/mesh.f.glsl");
    if (!meshvert || !meshfrag)
        return false;
    resources.meshprogram = make_program(meshvert, meshfrag);

    glGenFramebuffers(1, &resources.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);

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

    // Enable program and set up values
    glUseProgram(resources.program);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix * modelMatrix));
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

void renderTexturedRectangle(const glm::mat4 &modelMatrix, GLuint texture)
{
    GLuint projectionUniform = glGetUniformLocation(resources.texprogram, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(resources.texprogram, "modelViewMatrix");
    GLuint textureUniform = glGetUniformLocation(resources.texprogram, "texture");

    // Enable program and set up values
    glUseProgram(resources.texprogram);
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix * modelMatrix));
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

void renderMaskedRectangle(const glm::mat4 &modelMatrix, const glm::vec4 &color,
        const anim_frame *frame)
{
    GLuint projectionUniform = glGetUniformLocation(resources.maskprogram, "projectionMatrix");
    GLuint modelViewUniform = glGetUniformLocation(resources.maskprogram, "modelViewMatrix");
    GLuint textureUniform = glGetUniformLocation(resources.maskprogram, "texture");
    GLuint colorUniform = glGetUniformLocation(resources.maskprogram, "color");
    GLuint texsizeUniform = glGetUniformLocation(resources.maskprogram, "texsize");

    // Enable program and set up values
    glUseProgram(resources.maskprogram);
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix * modelMatrix));
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniform1i(textureUniform, 0);
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));
    glUniform2fv(texsizeUniform, 1, glm::value_ptr(glm::vec2(frame->w, frame->h)));

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, frame->mask_tex);

    glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, resources.element_buffer);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

    // Clean up
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

struct vert
{
    float pos[4];
    float norm[4];
    float texcoord[2];
};

struct facevert
{
    int v, n, t;
};

struct face
{
    std::vector<facevert> fvs;
};

void addVert(std::vector<vert> &verts, const std::vector<glm::vec4> &positions,
        const std::vector<glm::vec4> &norms, const std::vector<glm::vec2> &texcoords,
        const facevert fv)
{
    glm::vec4 pos = positions.at(fv.v - 1);
    glm::vec4 norm = norms.at(fv.n - 1);
    glm::vec2 texcoord = texcoords.at(fv.t - 1);

    /*
    std::cout << "v t n: " << fv.v << ' ' << fv.t << ' ' << fv.n << '\t'
        << "norm: " << norm.x << ' ' << norm.y << ' ' << norm.z << '\t'
        << "uv: " << texcoord.x << ' ' << texcoord.y << '\n';
        */
        
    vert v;
    v.pos[0] = pos[0]; v.pos[1] = pos[1]; v.pos[2] = pos[2]; v.pos[3] = pos[3];
    v.norm[0] = norm[0]; v.norm[1] = norm[1]; v.norm[2] = norm[2]; v.norm[3] = norm[3];
    v.texcoord[0] = texcoord[0]; v.texcoord[1] = texcoord[1];

    verts.push_back(v);
}

mesh createMesh(std::string objfile)
{
    std::ifstream file(objfile.c_str());
    if (!file)
    {
        std::cerr << "Unable to open mesh file " << objfile << '\n';
        exit(1);
    }

    std::vector<glm::vec4> positions;
    std::vector<glm::vec4> norms;
    std::vector<glm::vec2> texcoords;
    std::vector<face> faces;

    // Parse the file
    char buf[1024];
    for (;;)
    {
        if (!file.getline(buf, 1024))
            break;
        std::stringstream ss(buf);

        std::string cmd;
        float a, b, c;
        ss >> cmd;
        if (!ss)
            continue;

        if (cmd.empty())
        {
            continue;
        }
        else if (cmd == "g")
            std::cout << "Ignoring 'g' command: " << buf << '\n';
        else if (cmd == "s")
            std::cout << "Ignoring 's' command: " << buf << '\n';
        else if (cmd == "o")
            std::cout << "Ignoring 'o' command: " << buf << '\n';
        else if (cmd == "usemtl")
            std::cout << "Ignoring 'usemtl' command: " << buf << '\n';
        else if (cmd == "v")
        {
            ss >> a >> b >> c;
            positions.push_back(glm::vec4(a, b, c, 1.0f));
        }
        // ignore comments
        else if (cmd[0] == '#')
        {
            continue;
        }
        else if (cmd == "vn")
        {
            ss >> a >> b >> c;
            norms.push_back(glm::vec4(a, b, c, 1.0f));
        }
        else if (cmd == "vt")
        {
            ss >> a >> b;
            texcoords.push_back(glm::vec2(a, b));
        }
        else if (cmd == "f")
        {
            face fc;
            while (ss)
            {
                std::string facestr;
                ss >> facestr;
                if (!ss)
                    break;

                facevert f;
                if (sscanf(facestr.c_str(), "%d//%d", &f.v, &f.t, &f.n) != 3)
                if (sscanf(facestr.c_str(), "%d/%d/%d", &f.v, &f.t, &f.n) != 3)
                {
                    std::cerr << "Error reading " << objfile << '\n';
                    exit(1);
                }
                fc.fvs.push_back(f);
            }

            faces.push_back(fc);
        }
        else
        {
            std::cerr << "Unknown .obj definition: " << cmd << '\n';
            exit(1);
        }
    }

    // Now create the vertices
    std::vector<vert> verts;
    for (unsigned i = 0; i < faces.size(); i++)
    {
        face f = faces[i];
        assert(f.fvs.size() >= 3);
        facevert first = f.fvs[0];
        for (unsigned j = 1; j < f.fvs.size() - 1; j++)
        {
            facevert a = f.fvs[j];
            facevert b = f.fvs[j+1];

            addVert(verts, positions, norms, texcoords, first);
            addVert(verts, positions, norms, texcoords, a);
            addVert(verts, positions, norms, texcoords, b);
        }
    }

    std::cout << "Loaded verts: " << verts.size() << '\n';

    mesh ret;
    // Now create a buffer to hold the data
    ret.data_buffer = make_buffer(GL_ARRAY_BUFFER, &verts.front(), sizeof(vert) * verts.size());
    ret.nverts = verts.size();

    return ret;
}

void renderMesh(const mesh &m, const glm::mat4 &modelMatrix, const glm::vec3 &color)
{
    // Uniform locations
    GLuint modelViewUniform = glGetUniformLocation(resources.meshprogram, "modelViewMatrix");
    GLuint projectionUniform = glGetUniformLocation(resources.meshprogram, "projectionMatrix");
    GLuint normalUniform = glGetUniformLocation(resources.meshprogram, "normalMatrix");
    GLuint colorUniform = glGetUniformLocation(resources.meshprogram, "color");
    // Enable program and set up values
    glUseProgram(resources.meshprogram);
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix * modelMatrix));
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(normalUniform, 1, GL_FALSE, glm::value_ptr(glm::inverse(modelMatrix)));
    glUniform4fv(colorUniform, 1, glm::value_ptr(glm::vec4(color, 1.0f)));

    // Bind data
    glBindBuffer(GL_ARRAY_BUFFER, m.data_buffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(struct vert), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(struct vert), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vert), (void*)(8 * sizeof(float)));

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, m.nverts);

    // Clean up
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glUseProgram(0);
}

void setProjectionMatrix(const glm::mat4 &mat)
{
    projectionMatrix = mat;
}

void setViewMatrix(const glm::mat4 &mat)
{
    viewMatrix = mat;
}

const glm::mat4 & getProjectionMatrix()
{
    return projectionMatrix;
}

const glm::mat4 & getViewMatrix()
{
    return viewMatrix;
}
