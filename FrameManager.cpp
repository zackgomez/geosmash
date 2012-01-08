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

void FrameManager::renderFrame(const glm::mat4 &trans, const glm::vec4 &col,
        const std::string &name)
{
    if (frames_.find(name) == frames_.end())
    {
        std::cerr << "FATAL ERROR: Unable to find frame " << name << '\n';
        assert(false);
    }
    const anim_frame *frame = frames_[name];

    glm::mat4 finalTransform = glm::translate(
            glm::scale(trans, glm::vec3(frame->w*5, frame->h*5, 1.0f)),
            glm::vec3(frame->x / frame->w, frame->y / frame->h, 0.0f));

    renderMaskedRectangle(finalTransform, col, frame);
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

FrameManager::~FrameManager()
{
    clear();
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
    // Clean up the string
    while (!name.empty() && isspace(name[name.size()-1]))
        name.erase(name.end() - 1);
    // Ignore blank lines
    if (name.empty())
            return 0;
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
    ret->w = w; ret->h = h;
    ret->x = x; ret->y = y;
    ret->mask_tex = mask;

    return ret;
}
