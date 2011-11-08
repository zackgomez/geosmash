#include "FrameManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <fstream>
#include <cstdio>

FrameManager * FrameManager::get()
{
    static FrameManager fm;
    return &fm;
}

void FrameManager::renderFrame(const glm::mat4 &trans, const glm::vec3 &col,
        const std::string &name)
{
    if (frames_.find(name) == frames_.end())
    {
        std::cerr << "Unable to find frame " << name << " || strlen: " << name.length() << '\n';
        assert(false);
    }
    const anim_frame *frame = frames_[name];

    glm::mat4 finalTransform = glm::translate(
            glm::scale(trans, glm::vec3(frame->w/2, frame->h/2, 1.0f)),
            glm::vec3(frame->x, frame->y, 0.0f));

    renderMaskedRectangle(finalTransform, col, frame->mask_tex);
}

void FrameManager::loadFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file)
    {
        std::cerr << "Unable to open file " << filename << '\n';
        assert(false);
    }

    while(!(!file))
    {
        anim_frame *frm = loadAnimFrame(file);
        if (!frm)
            continue;
        std::cout << "Loaded frame " << frm->id << '\n';
        addFrame(frm);
    }
}

void FrameManager::addFrame(const anim_frame *frame)
{
    assert(frame);
    frames_[frame->id] = frame;
}

void FrameManager::clear()
{
    std::map<std::string, const anim_frame*>::const_iterator it;
    for (it = frames_.begin(); it != frames_.end(); it++)
        delete it->second;
    frames_.clear();
}

FrameManager::FrameManager()
{
    /* Empty */
}

GLuint FrameManager::createMaskTexture(GLubyte *data, int w, int h)
{
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

    return tex;
}

anim_frame* FrameManager::loadAnimFrame(std::istream &stream)
{
    std::string line;
    std::string name;
    int w, h;
    float x, y;

    // First line is name
    std::getline(stream, name);
    name.erase(name.end() - 1);
    // Next line is height/width
    std::getline(stream, line);
    if (sscanf(line.c_str(), "%d %d\n", &w, &h) != 2)
    {
        std::cerr << "Unable to load w,h for name " << name << '\n';
        return 0;
    }
    // Next line is center x/y
    std::getline(stream, line);
    if (sscanf(line.c_str(), "%f %f\n", &x, &y) != 2)
    {
        std::cerr << "Unable to load x,y for name " << name << '\n';
        return 0;
    }

    // Next is the data
    GLubyte *data = (GLubyte *) malloc(w * h * sizeof(GLubyte));

    char c;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            stream >> c;
            if (c != '0' && c != '1')
                return 0;
            else
                data[x + w*(h - 1 - y)] = (c == '0' ? 0 : 255);
        }

    GLuint mask = createMaskTexture(data, w, h);
    free(data);
    if (mask == 0)
        return 0;

    anim_frame *ret = new anim_frame();
    ret->id = name;
    ret->w = w*10.f; ret->h = h*10.f;
    ret->x = x; ret->y = y;
    ret->mask_tex = mask;

    return ret;
}
