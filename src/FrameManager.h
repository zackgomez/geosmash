#pragma once
#include <map>
#include "Engine.h"
#include "Logger.h"
#include "kiss-skeleton.h"

struct anim_frame
{
    std::string id;
    GLuint mask_tex;
    float w, h; // game units
    float x, y; // Center inside texture, [-0.5, 0.5]
};

class FrameManager
{
public:
    static FrameManager* get();

    void renderFrame(const glm::mat4 &transform, const glm::vec4 &color,
            const std::string &name) const;

    void loadFile(const std::string &filename);

    // Returns a keyframe ready for passing to a skeleton for the given pose
    Keyframe getPose(const std::string &poseName) const;

    // Removes all currently loaded frames
    void clear();

private:
    FrameManager();
    FrameManager(const FrameManager &);
    ~FrameManager();

    LoggerPtr logger_;

    std::map<std::string, const anim_frame*> frames_;

    void addFrame(const anim_frame *frame);
    GLuint createMaskTexture(GLubyte *data, int w, int h);
    anim_frame *loadAnimFrame(std::istream &stream);
};

