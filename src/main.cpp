// This example is heavily based on the tutorial at https://open.gl

// OpenGL Helpers to reduce the clutter
#include "Helpers.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#else
// GLFW is necessary to handle the OpenGL context
#include <GLFW/glfw3.h>
#endif

// OpenGL Mathematics Library
#include <glm/glm.hpp>                  // glm::vec3
#include <glm/vec3.hpp>                 // glm::vec3
#include <glm/vec4.hpp>                 // glm::vec4
#include <glm/mat4x4.hpp>               // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp>

#include <earcut.hpp>
#include <json.hpp>
using json = nlohmann::json;

#include <iostream>
#include <fstream>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>

#pragma region // Variables
// VertexBufferObject wrapper
BufferObject VBO;
BufferObject VBOC;
// VertexBufferObject wrapper
BufferObject NBO;
// VertexBufferObject wrapper
BufferObject TBO;
// VertexBufferObject wrapper
BufferObject IndexBuffer;
BufferObject Indexes;

// Program def
Program program;
Program countriesProgram;
// Types def
using Coord = double;
using N = uint32_t;
using Point = std::array<Coord, 2>;
using MultiPoly = std::vector<std::vector<std::vector<Point>>>;

// Contains the vertex positions
std::vector<glm::vec3> V(3);
// Contains the vertex positions
std::vector<glm::vec3> VN(3);
// Contains the vertex positions
std::vector<glm::ivec3> T(3);
// Contains the texture posions
std::vector<glm::vec2> TX(3);
// Contains values for countries
std::vector<glm::vec3> VC;
std::vector<glm::ivec3> I;
MultiPoly vertices;

std::vector<float> pixels;

// Last position of the mouse on click
double xpos, ypos;

// camera setup and matrix calculations
glm::vec3 cameraPos;
glm::vec3 cameraTarget;
glm::vec3 cameraDirection;
glm::vec3 cameraUp;
glm::vec3 cameraRight;
glm::mat4 viewMatrix;
glm::mat4 projMatrix;
glm::mat4 rotationMatrix = glm::mat4(1.0f);

// Axis and rotations
glm::vec3 centroid;
glm::vec3 axisDir;
glm::vec3 axisUp;
glm::vec3 axisRight;

float camRadius = 5.0f;
#pragma endregion
// PPM Reader code from http://josiahmanson.com/prose/optimize_ppm/

struct RGB
{
    unsigned char r, g, b;
};

struct ImageRGB
{
    int w, h;
    std::vector<RGB> data;
};

void eat_comment(std::ifstream &f)
{
    char linebuf[1024];
    char ppp;
    while (ppp = f.peek(), ppp == '\n' || ppp == '\r')
        f.get();
    if (ppp == '#')
        f.getline(linebuf, 1023);
}

bool loadPPM(ImageRGB &img, const std::string &name)
{
    std::ifstream f(name.c_str(), std::ios::binary);
    if (f.fail())
    {
        std::cout << "Could not open file: " << name << std::endl;
        return false;
    }

    // get type of file
    eat_comment(f);
    int mode = 0;
    std::string s;
    f >> s;
    if (s == "P3")
        mode = 3;
    else if (s == "P6")
        mode = 6;

    // get w
    eat_comment(f);
    f >> img.w;

    // get h
    eat_comment(f);
    f >> img.h;

    // get bits
    eat_comment(f);
    int bits = 0;
    f >> bits;

    // error checking
    if (mode != 3 && mode != 6)
    {
        std::cout << "Unsupported magic number" << std::endl;
        f.close();
        return false;
    }
    if (img.w < 1)
    {
        std::cout << "Unsupported width: " << img.w << std::endl;
        f.close();
        return false;
    }
    if (img.h < 1)
    {
        std::cout << "Unsupported height: " << img.h << std::endl;
        f.close();
        return false;
    }
    if (bits < 1 || bits > 255)
    {
        std::cout << "Unsupported number of bits: " << bits << std::endl;
        f.close();
        return false;
    }

    // load image data
    img.data.resize(img.w * img.h);

    if (mode == 6)
    {
        f.get();
        f.read((char *)&img.data[0], img.data.size() * 3);
    }
    else if (mode == 3)
    {
        for (int i = 0; i < img.data.size(); i++)
        {
            int v;
            f >> v;
            img.data[i].r = v;
            f >> v;
            img.data[i].g = v;
            f >> v;
            img.data[i].b = v;
        }
    }

    // close file
    f.close();
    return true;
}

bool loadOFFFile(std::string filename, std::vector<glm::vec3> &vertex, std::vector<glm::ivec3> &tria, glm::vec3 &min, glm::vec3 &max)
{
    min.x = FLT_MAX;
    max.x = FLT_MIN;
    min.y = FLT_MAX;
    max.y = FLT_MIN;
    min.z = FLT_MAX;
    max.z = FLT_MIN;
    try
    {
        std::ifstream ofs(filename, std::ios::in | std::ios_base::binary);
        if (ofs.fail())
            return false;
        std::string line, tmpStr;
        // First line(optional) : the letters OFF to mark the file type.
        // Second line : the number of vertices, number of faces, and number of edges, in order (the latter can be ignored by writing 0 instead).
        int numVert = 0;
        int numFace = 0;
        int numEdge = 0;
        // first line must be OFF
        getline(ofs, line);
        if (line.rfind("OFF", 0) == 0)
            getline(ofs, line);
        std::stringstream tmpStream(line);
        getline(tmpStream, tmpStr, ' ');
        numVert = std::stoi(tmpStr);
        getline(tmpStream, tmpStr, ' ');
        numFace = std::stoi(tmpStr);
        getline(tmpStream, tmpStr, ' ');
        numEdge = std::stoi(tmpStr);

        // read all vertices and get min/max values
        V.resize(numVert);
        for (int i = 0; i < numVert; i++)
        {
            getline(ofs, line);
            tmpStream.clear();
            tmpStream.str(line);
            getline(tmpStream, tmpStr, ' ');
            V[i].x = std::stof(tmpStr);
            min.x = std::fminf(V[i].x, min.x);
            max.x = std::fmaxf(V[i].x, max.x);
            getline(tmpStream, tmpStr, ' ');
            V[i].y = std::stof(tmpStr);
            min.y = std::fminf(V[i].y, min.y);
            max.y = std::fmaxf(V[i].y, max.y);
            getline(tmpStream, tmpStr, ' ');
            V[i].z = std::stof(tmpStr);
            min.z = std::fminf(V[i].z, min.z);
            max.z = std::fmaxf(V[i].z, max.z);
        }

        // read all faces (triangles)
        T.resize(numFace);
        for (int i = 0; i < numFace; i++)
        {
            getline(ofs, line);
            tmpStream.clear();
            tmpStream.str(line);
            getline(tmpStream, tmpStr, ' ');
            if (std::stoi(tmpStr) != 3)
                return false;
            getline(tmpStream, tmpStr, ' ');
            T[i].x = std::stoi(tmpStr);
            getline(tmpStream, tmpStr, ' ');
            T[i].y = std::stoi(tmpStr);
            getline(tmpStream, tmpStr, ' ');
            T[i].z = std::stoi(tmpStr);
        }

        ofs.close();
    }
    catch (const std::exception &e)
    {
        // return false if an exception occurred
        std::cerr << e.what() << std::endl;
        return false;
    }
    return true;
}

void sphere(float sphereRadius, int sectorCount, int stackCount, std::vector<glm::vec3> &vertex, std::vector<glm::vec3> &normal, std::vector<glm::ivec3> &tria, std::vector<glm::vec2> &texture)
{
    // init variables
    vertex.resize(0);
    normal.resize(0);
    tria.resize(0);
    texture.resize(0);
    // temp variables
    glm::vec3 sphereVertexPos;
    float xy;
    float sectorStep = 2.0f * M_PI / float(sectorCount);
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;
    float s, t;

    // compute vertices and normals
    for (int i = 0; i <= stackCount; ++i)
    {
        stackAngle = M_PI / 2.0f - i * stackStep;
        xy = sphereRadius * cosf(stackAngle);
        sphereVertexPos.z = sphereRadius * sinf(stackAngle);

        for (int j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep;

            // vertex position
            sphereVertexPos.x = xy * cosf(sectorAngle);
            sphereVertexPos.y = xy * sinf(sectorAngle);
            vertex.push_back(sphereVertexPos);

            // normalized vertex normal
            normal.push_back(sphereVertexPos / sphereRadius);

            // texture coordinates
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            texture.push_back(glm::vec2(s, t));
        }
    }

    // compute triangle indices
    int k1, k2;
    for (int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);
        k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector excluding first and last stacks
            // k1 => k2 => k1+1
            if (i != 0)
            {
                T.push_back(glm::ivec3(k1, k2, k1 + 1));
            }
            // k1+1 => k2 => k2+1
            if (i != (stackCount - 1))
            {
                T.push_back(glm::ivec3(k1 + 1, k2, k2 + 1));
            }
        }
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    // Get the size of the window
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    // Update the position of the first vertex if the left button is pressed
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Get the position of the mouse in the window
        glfwGetCursorPos(window, &xpos, &ypos);
        std::cout << xpos << " " << ypos << std::endl;
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // temp variables
    glm::mat3 rot;
    glm::mat4 rota;
    // Update the position of the first vertex if the keys 1,2, or 3 are pressed
    switch (key)
    {
    case GLFW_KEY_A:
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(-5.0f), cameraUp);
        cameraPos = rot * cameraPos;
        cameraDirection = glm::normalize(cameraPos - cameraTarget);
        cameraRight = glm::normalize(glm::cross(cameraUp, cameraDirection));
        break;
    case GLFW_KEY_D:
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(5.0f), cameraUp);
        cameraPos = rot * cameraPos;
        cameraDirection = glm::normalize(cameraPos - cameraTarget);
        cameraRight = glm::normalize(glm::cross(cameraUp, cameraDirection));
        break;
    case GLFW_KEY_W:
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(-5.0f), cameraRight);
        cameraPos = rot * cameraPos;
        cameraDirection = glm::normalize(cameraPos - cameraTarget);
        cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
        break;
    case GLFW_KEY_S:
        rot = glm::rotate(glm::mat4(1.0f), glm::radians(5.0f), cameraRight);
        cameraPos = rot * cameraPos;
        cameraDirection = glm::normalize(cameraPos - cameraTarget);
        cameraUp = glm::normalize(glm::cross(cameraDirection, cameraRight));
        break;
    case GLFW_KEY_UP:
        cameraPos -= cameraDirection * 0.25f;
        break;
    case GLFW_KEY_DOWN:
        cameraPos += cameraDirection * 0.25f;
        break;
    case GLFW_KEY_R:
        cameraPos = glm::vec3(0.0f, 0.0f, camRadius);
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraDirection = glm::normalize(cameraPos - cameraTarget);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraRight = glm::normalize(glm::cross(cameraUp, cameraDirection));
        break;
    case GLFW_KEY_I:
        rota = glm::rotate(glm::mat4(1.0f), glm::radians(-5.0f), axisUp);
        centroid = rota * glm::vec4(centroid, 1);
        rotationMatrix *= rota;
        axisDir = glm::normalize(centroid - cameraTarget);
        axisRight = glm::normalize(glm::cross(axisUp, axisDir));
        break;
    case GLFW_KEY_K:
        rota = glm::rotate(glm::mat4(1.0f), glm::radians(5.0f), axisUp);
        centroid = rota * glm::vec4(centroid, 1);
        rotationMatrix *= rota;
        axisDir = glm::normalize(centroid - cameraTarget);
        axisRight = glm::normalize(glm::cross(axisUp, axisDir));
        break;
    case GLFW_KEY_J:
        rota = glm::rotate(glm::mat4(1.0f), glm::radians(-5.0f), axisRight);
        centroid = rota * glm::vec4(centroid, 1);
        rotationMatrix *= rota;
        axisDir = glm::normalize(centroid - cameraTarget);
        axisUp = glm::normalize(glm::cross(axisRight, axisDir));
        break;
    case GLFW_KEY_L:
        rota = glm::rotate(glm::mat4(1.0f), glm::radians(5.0f), axisRight);
        centroid = rota * glm::vec4(centroid, 1);
        rotationMatrix *= rota;
        axisDir = glm::normalize(centroid - cameraTarget);
        axisUp = glm::normalize(glm::cross(axisRight, axisDir));
        break;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GL_TRUE);
        break;
    default:
        break;
    }
}

void readGeoJSON(std::string filename, MultiPoly &vertex)
{
    // Read file
    std::ifstream i(filename);
    if (i.fail())
    {
        std::cout << "Could not open file: " << filename << std::endl;
    }
    json j;
    i >> j;

    int countryFID = 149;
    if (j["features"][countryFID]["geometry"]["type"].get<std::string>() == "Polygon")
    {
        std::vector<std::vector<Point>> coordinates = j["features"][countryFID]["geometry"]["coordinates"].get<std::vector<std::vector<Point>>>();
        vertex.push_back(coordinates);
    }
    else
    {
        vertex.resize(j["features"][countryFID]["geometry"]["coordinates"].get<MultiPoly>().size());
        vertex = j["features"][countryFID]["geometry"]["coordinates"].get<MultiPoly>();
    }
    // check if one exits
    for (int i = 0; i < 195; i++)
    {
        if (j["features"][i]["geometry"]["type"].get<std::string>() == "Polygon" && j["features"][i]["geometry"]["coordinates"].get<std::vector<std::vector<std::vector<float>>>>().size() > 1)
        {
            std::cout << i << std::endl;
        }
    }
    // coord
    // write prettified JSON to another file
    std::ofstream o("pretty.json");
    o << std::setw(4) << j["features"][countryFID] << std::endl;

    // for (auto &co : coord)
    // {
    //     vertex.push_back({co[0].get<float>(), co[1].get<float>()});
    // }
}
void toSphericalCoord(float lat, float lon, glm::vec3 &coord, float radius)
{
    float lattitude = glm::radians(lon);
    float longitude = glm::radians(lat);
    float f = 0.0f;
    float ls = atanf(pow((1 - f), 2) * tanf(lattitude));
    coord.x = radius * cosf(ls) * cosf(longitude);
    coord.y = radius * cosf(ls) * sinf(longitude);
    coord.z = radius * sinf(ls);
}
void countriesRearrange(MultiPoly &vertex, std::vector<glm::vec3> &V, std::vector<glm::ivec3> &indices)
{
    if (vertex.size() == 1)
    // if (1)
    {
        std::vector<N> indice = mapbox::earcut<N>(vertex[0]);
        for (int i = 0; i < indice.size(); i += 3)
        {
            indices.push_back(glm::ivec3(indice[i], indice[i + 1], indice[i + 2]));
        }
        for (int i = 0; i < vertex[0].size(); i++)
        {
            for (int j = 0; j < vertex[0][i].size(); j++)
            {
                glm::vec3 coord;
                toSphericalCoord(vertex[0][i][j][0], vertex[0][i][j][1], coord, 1.01f);
                V.push_back(coord);
            }
        }
    }
    else
    {
        N prevMax = 0, currMax = 0;
        for (int i = 0; i < vertex.size(); i++)
        {
            std::vector<N> indice = mapbox::earcut<N>(vertex[i]);
            for (int j = 0; j < indice.size(); j += 3)
            {
                indices.push_back(glm::ivec3(prevMax + indice[j],prevMax +  indice[j + 1],prevMax +  indice[j + 2]));
                currMax = std::max(std::max(currMax, indice[j]), std::max(indice[j+1],indice[j+2]));
            }
            for (int j = 0; j < vertex[i].size(); j++)
            {
                for (int k = 0; k < vertex[i][j].size(); k++)
                {
                    glm::vec3 coord;
                    toSphericalCoord(vertex[i][j][k][0], vertex[i][j][k][1], coord, 1.01f);
                    V.push_back(coord);
                }
            }
            prevMax += currMax + 1;
            currMax = 0;
            std::cout << prevMax << std::endl;
        }
    }
}

int main(void)
{

#pragma region // Window creation and stuff
    GLFWwindow *window;

    // Initialize the library
    if (!glfwInit())
        return -1;

    // Activate supersampling
    glfwWindowHint(GLFW_SAMPLES, 8);

    // Ensure that we get at least a 3.2 context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    // On apple we have to load a core profile with forward compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(800, 600, "Hello OpenGL", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

#ifndef __APPLE__
    glewExperimental = true;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
    }
    glGetError(); // pull and savely ignonre unhandled errors like GL_INVALID_ENUM
    std::cout << "Status: Using GLEW " << glewGetString(GLEW_VERSION) << std::endl;
#endif

    int major, minor, rev;
    major = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR);
    minor = glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR);
    rev = glfwGetWindowAttrib(window, GLFW_CONTEXT_REVISION);
    std::cout << "OpenGL version recieved: " << major << "." << minor << "." << rev << std::endl;
    std::cout << "Supported OpenGL is " << (const char *)glGetString(GL_VERSION) << std::endl;
    std::cout << "Supported GLSL is " << (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
#pragma endregion

#pragma region // Read data
    readGeoJSON("../data/WB_Boundaries_GeoJSON_lowres/WB_countries_Admin0_lowres.geojson", vertices);
    countriesRearrange(vertices, VC, I);
    centroid = std::accumulate(VC.begin(), VC.end(), glm::vec3(0));
    ImageRGB image;
    bool imageAvailable = loadPPM(image, "../data/land_shallow_topo_2048.ppm");
    if (imageAvailable)
    {
        for (int j = 0; j < image.w * image.h; j++)
        {
            pixels.push_back(image.data[j].r);
            pixels.push_back(image.data[j].g);
            pixels.push_back(image.data[j].b);
        }
    }

#pragma endregion
#pragma region // Earth VAO and program
    VertexArrayObject VAO;
    VAO.init();
    VAO.bind();

    // Initialize the VBO with the vertices data
    VBO.init();
    // initialize normal array buffer
    NBO.init();
    // initialize texture array buffer
    TBO.init();
    // initialize element array buffer
    IndexBuffer.init(GL_ELEMENT_ARRAY_BUFFER);

    sphere(1.0f, 20, 10, V, VN, T, TX);
    VBO.update(V);
    NBO.update(VN);
    TBO.update(TX);
    IndexBuffer.update(T);

    // load fragment shader file
    std::ifstream fragShaderFile("../shader/fragment.glsl");
    std::stringstream fragCode;
    fragCode << fragShaderFile.rdbuf();
    // load vertex shader file
    std::ifstream vertShaderFile("../shader/vertex.glsl");
    std::stringstream vertCode;
    vertCode << vertShaderFile.rdbuf();
    // Compile the two shaders and upload the binary to the GPU
    // Note that we have to explicitly specify that the output "slot" called outColor
    // is the one that we want in the fragment buffer (and thus on screen)
    program.init(vertCode.str(), fragCode.str(), "outColor");
    program.bind();

    // The vertex shader wants the position of the vertices as an input.
    // The following line connects the VBO we defined above with the position "slot"
    // in the vertex shader
    program.bindVertexAttribArray("position", VBO);
    program.bindVertexAttribArray("normal", NBO);
    program.bindVertexAttribArray("texCoord", TBO);

    // Create texture and upload image data
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.w, image.h, 0, GL_RGB, GL_FLOAT, &pixels[0]);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Unbind everything
    glBindVertexArray(0);
    glUseProgram(0);
#pragma endregion

#pragma region // Countries VAO and program
    VertexArrayObject VAOC;
    VAOC.init();
    VAOC.bind();
    VBOC.init();
    Indexes.init(GL_ELEMENT_ARRAY_BUFFER);

    VBOC.update(VC);
    Indexes.update(I);

    // New Program
    std::ifstream fragShaderFile2("../shader/fragment2.glsl");
    std::stringstream fragCode2;
    fragCode2 << fragShaderFile2.rdbuf();
    // load vertex shader file
    std::ifstream vertShaderFile2("../shader/vertex2.glsl");
    std::stringstream vertCode2;
    vertCode2 << vertShaderFile2.rdbuf();
    countriesProgram.init(vertCode2.str(), fragCode2.str(), "outColor");
    countriesProgram.bind();
    countriesProgram.bindVertexAttribArray("position", VBOC);

    // Unbind everything
    glBindVertexArray(0);
    glUseProgram(0);
#pragma endregion

#pragma region // Callbacks and camera position
    // Register the keyboard callback
    glfwSetKeyCallback(window, key_callback);

    // Register the mouse callback
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Update viewport
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // camera setup
    cameraPos = glm::vec3(0.0f, 0.0f, camRadius);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraDirection = glm::normalize(cameraPos - cameraTarget);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraRight = glm::normalize(glm::cross(cameraUp, cameraDirection));

    // country set up
    axisDir = glm::normalize(centroid - cameraTarget);
    axisUp = glm::normalize(glm::cross(cameraUp, centroid));
    axisRight = glm::normalize(glm::cross(axisUp, axisDir));
#pragma endregion

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Get the size of the window
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // matrix calculations
        // initialize model matrix
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);
        projMatrix = glm::perspective(glm::radians(35.0f), (float)width / (float)height, 0.1f, 100.0f);
        // Clear the framebuffer
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
#pragma region // Earth
        // Bind your VAO (not necessary if you have only one)
        VAO.bind();

        // // bind your element array
        IndexBuffer.bind();

        // // Bind your program
        program.bind();

        // // Set the uniform values
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform3f(program.uniform("triangleColor"), 0.006f, 0.006f, 0.006f);
        glUniform3f(program.uniform("camPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniformMatrix4fv(program.uniform("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(program.uniform("viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(program.uniform("projMatrix"), 1, GL_FALSE, glm::value_ptr(projMatrix));
        // direction towards the light
        glUniform3fv(program.uniform("lightPos"), 1, glm::value_ptr(glm::vec3(-1.0f, 2.0f, 3.0f)));
        // x: ambient;
        glUniform3f(program.uniform("lightParams"), 0.1f, 50.0f, 0.0f);

        // Draw a triangle
        glDrawElements(GL_TRIANGLES, T.size() * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
#pragma endregion
#pragma region // Countries rendering
        VAOC.bind();
        countriesProgram.bind();
        Indexes.bind();
        glUniform3f(countriesProgram.uniform("triangleColor"), 1.0f, 0.606f, 0.006f);
        glUniformMatrix4fv(countriesProgram.uniform("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(countriesProgram.uniform("viewMatrix"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(countriesProgram.uniform("projMatrix"), 1, GL_FALSE, glm::value_ptr(projMatrix));
        glUniformMatrix4fv(countriesProgram.uniform("rotationMatrix"), 1, GL_FALSE, glm::value_ptr(rotationMatrix));
        glDrawElements(GL_TRIANGLES, I.size() * 3, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
#pragma endregion
        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }
#pragma region // Deallocation
    // Deallocate opengl memory
    // program.free();
    // VAO.free();
    // VBO.free();
    // TBO.free();
    // VAOC.free();
    // VBOC.free();

    // Deallocate glfw internals
    glfwTerminate();
    return 0;
#pragma endregion
}
