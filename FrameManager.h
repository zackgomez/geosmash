#pragma once
#include <map>
#include "glutils.h"

class FrameManager
{
public:
    static FrameManager* get();

    void renderFrame(const glm::mat4 &transform, const glm::vec4 &color,
            const std::string &name);

    void loadFile(const std::string &filename);

    // Removes all currently loaded frames
    void clear();

private:
    FrameManager();
    FrameManager(const FrameManager &);

    std::map<std::string, const anim_frame*> frames_;

    void addFrame(const anim_frame *frame);
    GLuint createMaskTexture(GLubyte *data, int w, int h);
    anim_frame *loadAnimFrame(std::istream &stream);
};

