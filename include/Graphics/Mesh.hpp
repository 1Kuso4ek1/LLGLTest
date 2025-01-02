#pragma once
#include <Utils.hpp>

namespace dev
{

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
    void CreatePlane();

    void BindBuffers(LLGL::CommandBuffer* commandBuffer, bool bindMatrices = true) const;

    void Draw(LLGL::CommandBuffer* commandBuffer) const;

private:
    void CreateVertexBuffer();
    void CreateIndexBuffer();

private:
    LLGL::VertexFormat vertexFormat;

    LLGL::Buffer* vertexBuffer{};
    LLGL::Buffer* indexBuffer{};
    LLGL::Buffer* matricesBuffer{};

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

}