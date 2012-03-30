#include "zgfx.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct facevert
{
    int v, n, t;
};

struct face
{
    std::vector<facevert> fvs;
};

void addVert(std::vector<vert_p4n4t2> &verts, const std::vector<glm::vec4> &positions,
        const std::vector<glm::vec4> &norms, const std::vector<glm::vec2> &texcoords,
        const facevert fv)
{
    glm::vec4 pos = positions.at(fv.v - 1);
    glm::vec4 norm = norms.at(fv.n - 1);
    glm::vec2 texcoord = texcoords.at(fv.t - 1);

    vert_p4n4t2 v;
    v.pos[0] = pos[0]; v.pos[1] = pos[1]; v.pos[2] = pos[2]; v.pos[3] = pos[3];
    v.norm[0] = norm[0]; v.norm[1] = norm[1]; v.norm[2] = norm[2]; v.norm[3] = norm[3];
    v.texcoord[0] = texcoord[0]; v.texcoord[1] = texcoord[1];

    verts.push_back(v);
}

mesh * loadMesh(const std::string &objfile, bool normalize)
{
    std::ifstream file(objfile.c_str());
    if (!file)
    {
        fprintf(stderr,"Unable to open mesh file %s\n", objfile.c_str());
        exit(1);
    }

    std::vector<glm::vec4> positions;
    std::vector<glm::vec4> norms;
    std::vector<glm::vec2> texcoords;
    std::vector<face> faces;

    glm::vec3 maxes(-HUGE_VAL);

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
        {}
        else if (cmd == "s")
        {}
        else if (cmd == "o")
        {}
        else if (cmd == "usemtl")
        {}
        else if (cmd == "v")
        {
            ss >> a >> b >> c;
            if (fabs(a) > maxes.x)
                maxes.x = fabs(a);
            if (fabs(a) > maxes.y)
                maxes.y = fabs(b);
            if (fabs(c) > maxes.z)
                maxes.z = fabs(c);
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
                if (sscanf(facestr.c_str(), "%d/%d/%d", &f.v, &f.t, &f.n) != 3)
                {
                    fprintf(stderr, "Error reading %s\n", objfile.c_str());
                    exit(1);
                }
                fc.fvs.push_back(f);
            }

            faces.push_back(fc);
        }
        else
        {
            fprintf(stderr,"Unknown .obj definition: %s\n", cmd.c_str());
            exit(1);
        }
    }

    // Now create the vertices
    std::vector<vert_p4n4t2> verts;
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

    mesh *ret = new mesh();
    // Now create a buffer to hold the data
    ret->data_buffer = make_buffer(GL_ARRAY_BUFFER, &verts.front(), sizeof(vert_p4n4t2) * verts.size());
    ret->nverts = verts.size();
    // If they asked to normalize, then do it!
    if (normalize)
    {
        ret->transform = glm::scale(glm::mat4(1.f),
                glm::vec3(1.f/maxes.x, 1.f/maxes.y, 1.f/maxes.z));
    }
    else
    {
        // Just set the transform to the identity
        // TODO support reading this from the obj file
        ret->transform = glm::mat4(1.f);
    }

    return ret;
}


void renderMesh(const glm::mat4 &projMatrix, const glm::mat4 &viewMatrix, const glm::mat4 &modelMatrix, const mesh *m, GLuint program)
{
    // Uniform locations
    GLuint modelViewUniform = glGetUniformLocation(program, "modelViewMatrix");
    GLuint projectionUniform = glGetUniformLocation(program, "projectionMatrix");
    GLuint normalUniform = glGetUniformLocation(program, "normalMatrix");
    // Attributes
    GLuint positionAttrib = glGetAttribLocation(program, "position");
    GLuint normalAttrib   = glGetAttribLocation(program, "normal");
    GLuint texcoordAttrib = glGetAttribLocation(program, "texcoord");
    // Enable program and set up values
    glUseProgram(program);
    glUniformMatrix4fv(modelViewUniform, 1, GL_FALSE, glm::value_ptr(viewMatrix * modelMatrix * m->transform));
    glUniformMatrix4fv(projectionUniform, 1, GL_FALSE, glm::value_ptr(projMatrix));
    glUniformMatrix4fv(normalUniform, 1, GL_FALSE, glm::value_ptr(glm::inverse(modelMatrix)));

    // Bind data
    glBindBuffer(GL_ARRAY_BUFFER, m->data_buffer);
    glEnableVertexAttribArray(positionAttrib);
    glEnableVertexAttribArray(normalAttrib);
    glEnableVertexAttribArray(texcoordAttrib);
    glVertexAttribPointer(positionAttrib, 4, GL_FLOAT, GL_FALSE, sizeof(struct vert_p4n4t2), (void*) (0));
    glVertexAttribPointer(normalAttrib,   4, GL_FLOAT, GL_FALSE, sizeof(struct vert_p4n4t2), (void*) (4 * sizeof(float)));
    glVertexAttribPointer(texcoordAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(struct vert_p4n4t2), (void*) (8 * sizeof(float)));

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, m->nverts);

    // Clean up
    glDisableVertexAttribArray(positionAttrib);
    glDisableVertexAttribArray(normalAttrib);
    glDisableVertexAttribArray(texcoordAttrib);
    glUseProgram(0);
}

void freeMesh(mesh *m)
{
    // nop on null mesh
    if (!m) return;

    glDeleteBuffers(1, &m->data_buffer);
    delete m;
}

