#pragma once
#include <Utils.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 coords;
};

class Mesh
{
public:
    Mesh() = default;
    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);

    void SetupBuffers();

    void CreateCube();

private:
    void CreateVertexBuffer();
    void CreateIndexBuffer();

    LLGL::VertexFormat vertexFormat;

    LLGL::Buffer* vertexBuffer;
    LLGL::Buffer* indexBuffer;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
