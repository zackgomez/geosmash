#include "FrameManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <fstream>
#include <cstdio>
#include <sstream>
#include "ParamReader.h"

FrameManager * FrameManager::get()
{
    static FrameManager fm;
    return &fm;
}

void FrameManager::renderFrame(const glm::mat4 &trans, const glm::vec4 &col,
        const std::string &name) const
{
    if (frames_.find(name) == frames_.end())
    {
        logger_->error() << "Unable to find frame " << name << '\n';
        assert(false);
    }
    const anim_frame *frame = frames_.find(name)->second;

    glm::mat4 finalTransform = glm::translate(
            glm::scale(trans, glm::vec3(frame->w*5, frame->h*5, 1.0f)),
            glm::vec3(frame->x / frame->w, frame->y / frame->h, 0.0f));

    renderMaskedRectangle(finalTransform, col, frame);
}

void FrameManager::renderFighter(const glm::mat4 &trans, const glm::vec4 &col,
        const std::string &fighterName) const
{
    if (fighterName == "charlie")
    {
        renderFrame(glm::scale(trans, glm::vec3(0.2f)), col, "GroundNormal");
    }
    else if (fighterName == "stickman")
    {
        // TODO this sucks, work out a better way
        Skeleton *skeleton = new Skeleton();
        GeosmashBoneRenderer *renderer = new GeosmashBoneRenderer();
        skeleton->readSkeleton("models/test.bones");
        skeleton->setBoneRenderer(renderer);

        renderer->setColor(col);
        skeleton->resetPose();
        skeleton->render(glm::scale(trans, 1.f/5.f*glm::vec3(getParam("stickman.w"), getParam("stickman.h"),
                        1.f)));

        delete skeleton;
        delete renderer;
    }
    else
        assert(false && "Unknown fighter");
}

void FrameManager::loadFrameFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file)
    {
        logger_->error() << "Unable to open file " << filename << '\n';
        assert(false);
    }

    while (file)
    {
        anim_frame *frm = loadAnimFrame(file);
        if (!frm)
            continue;
        logger_->info() << "Loaded frame " << frm->id << '\n';
        addFrame(frm);
    }
}

void FrameManager::loadPoseFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file)
    {
        logger_->error() << "Unable to open file " << filename << '\n';
        assert(false);
    }

    while (file)
    {
        std::string posename;
        Keyframe kf = loadPose(file, posename);
        // empty posename means end of file
        if (posename.empty())
            break;
        logger_->info() << "Loaded pose '" << posename << "'\n";
        poses_[posename] = kf;
    }
}

Keyframe FrameManager::getPose(const std::string &poseName) const
{
    if (poses_.find(poseName) == poses_.end())
    {
        logger_->warning() << "Couldn't find skeleton pose '" << poseName << "'\n";
        return Keyframe();
    }

    return poses_.find(poseName)->second;
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
    logger_ = Logger::getLogger("FrameManager");
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
        logger_->warning() << "Unable to load w,h for name " << name << '\n';
        return 0;
    }
    // Next line is center x/y
    std::getline(stream, line);
    if (sscanf(line.c_str(), "%f %f\n", &x, &y) != 2)
    {
        logger_->warning() << "Unable to load x,y for name " << name << '\n';
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

Keyframe FrameManager::loadPose(std::istream &is, std::string &posename)
{
    std::string line;
    Keyframe kf;
    while (std::getline(is, line))
    {
        // Empty line signals the end
        if (line.empty())
            break;

        // First line is posename
        if (posename.empty())
        {
            posename = line;
            continue;
        }

        // Read bone frame
        std::stringstream ss(line);
        std::string name;
        ss >> name;
        BoneFrame bf = readBoneFrame(ss);
        kf.bones[name] = bf;
    }

    return kf;
}

BoneFrame FrameManager::readBoneFrame(std::istream &is)
{
    BoneFrame bf;
    is >> bf.length >> bf.rot.x >> bf.rot.y >> bf.rot.z >> bf.rot[3];
    if (!is)
    {
        std::cerr << "Unable to read BoneFrame\n";
        exit(1);
    }

    return bf;
}

