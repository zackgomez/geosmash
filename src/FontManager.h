#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

class FontManager
{
public:
    static FontManager * get();

    // Renders a number.  Number is drawn as a two dimensional object with
    // size 1xN where n is the number of digits in the base ten representation
    // of the number.  The transform is used to translate to model space
    // and the number is drawn with the given color.
    void renderNumber(const glm::mat4 &transform, const glm::vec3 &color,
            int num);

    void renderString(const glm::mat4 &transform, const glm::vec3 &color,
            const std::string &str);

    // Returns the number of digits to be rendered in positive integer n
    static int numDigits(int n);

private:
    FontManager();
    ~FontManager();

    void initialize();
    void renderDigit(const glm::mat4 &transform, const glm::vec3 &color, char dig);
    void renderCharacter(const glm::mat4 &transform, const glm::vec3 &color, char dig);

    GLuint numberTex_;
    GLuint letterTex_;

    GLuint numProgram_;
};

