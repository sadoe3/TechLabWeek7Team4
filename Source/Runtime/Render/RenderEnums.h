#pragma once

enum class EShaderType
{
	Vertex,
	Pixel,
};

enum class EViewportMode : uint8
{
	Unlit,
	Lit,
	Wireframe,
	SceneDepth,
	None
};

enum class EShadingModel : uint8
{
	Lambert,
	Phong,
	BlinnPhong,
	Gouraud,
};
