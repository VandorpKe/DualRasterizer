#pragma once
#include "Math.h"

using namespace dae;

struct Vertex
{
	Vector3 position{};
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
};

// From software rasterizer
struct Vertex_Out
{
	Vector4 position{};
	ColorRGB color{ colors::White };
	Vector2 uv{};
	Vector3 normal{};
	Vector3 tangent{};
	Vector3 viewDirection{};
};

// From software rasterizer
enum class PrimitiveTopology
{
	TriangleList,
	TriangleStrip
};

// From software rasterizer
struct Mesh
{
	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};
	PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

	std::vector<Vertex_Out> vertices_out{};
	Matrix worldMatrix{};
};