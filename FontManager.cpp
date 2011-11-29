#include "FontManager.h"
#include "glutils.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>

const float widthRatio = 0.75f;

FontManager::FontManager() :
    numTex_(0)
{
    initialize();
}

FontManager::~FontManager()
{
    // TODO free textures etc
}

void FontManager::renderNumber(const glm::mat4 &transform,
        const glm::vec3 &color, int num)
{
    assert(num >= 0);
    std::stringstream ss;
    ss << num;
    std::string s = ss.str();
    size_t len = s.length();

    float offset = static_cast<float>(len) / -2.f + 0.5f;
    offset *= widthRatio;

    for (size_t i = 0; i < len; i++)
    {
        glm::mat4 final_transform =
            glm::translate(transform, glm::vec3(offset, 0.f, 0.f));

        renderDigit(final_transform, color, s[i]);

        offset += widthRatio;
    }
}

void FontManager::renderDigit(const glm::mat4 &transform, const glm::vec3 &color,
        char dig)
{
    char digit = dig - '0';

    glm::vec2 size(1.f/10.f, 1.f);
    glm::vec2 pos(size.x * static_cast<float>(digit), 0.f);

    // Set up the shader
    GLuint posUniform = glGetUniformLocation(numProgram_, "pos");
    GLuint sizeUniform = glGetUniformLocation(numProgram_, "size");
    GLuint colorUniform = glGetUniformLocation(numProgram_, "color");

    glUseProgram(numProgram_);
    glUniform2fv(posUniform, 1, glm::value_ptr(pos));
    glUniform2fv(sizeUniform, 1, glm::value_ptr(size));
    glUniform3fv(colorUniform, 1, glm::value_ptr(color));
    glUseProgram(0);
    // Set up the texture
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, numTex_);

    renderRectangleProgram(glm::scale(transform, glm::vec3(0.66f, -1.f, 1.f)), numProgram_);

    glDisable(GL_TEXTURE_2D);
}

FontManager * FontManager::get()
{
    static FontManager fm;
    return &fm;
}

void FontManager::initialize()
{
    numTex_ = make_texture("images/numbers-64.png");

    GLuint vert = make_shader(GL_VERTEX_SHADER, "shaders/num.v.glsl");
    GLuint frag = make_shader(GL_FRAGMENT_SHADER, "shaders/num.f.glsl");
    numProgram_ = make_program(vert, frag);
    assert(numProgram_);
}
