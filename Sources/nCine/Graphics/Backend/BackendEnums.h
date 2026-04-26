#pragma once

namespace nCine
{
	/// Primitive types
	enum class PrimitiveType
	{
		Points = 0x0000,
		Lines = 0x0001,
		LineLoop = 0x0002,
		LineStrip = 0x0003,
		Triangles = 0x0004,
		TriangleStrip = 0x0005,
		TriangleFan = 0x0006
	};

	/// Buffer usage flags
	enum class BufferUsage
	{
		StreamDraw = 0x88E0,
		StreamRead = 0x88E1,
		StreamCopy = 0x88E2,
		StaticDraw = 0x88E4,
		StaticRead = 0x88E5,
		StaticCopy = 0x88E6,
		DynamicDraw = 0x88E8,
		DynamicRead = 0x88E9,
		DynamicCopy = 0x88EA
	};

	/// Blending factors
	enum class BlendingFactor
	{
		Zero = 0,
		One = 1,
		SrcColor = 0x0300,
		OneMinusSrcColor = 0x0301,
		SrcAlpha = 0x0302,
		OneMinusSrcAlpha = 0x0303,
		DstAlpha = 0x0304,
		OneMinusDstAlpha = 0x0305,
		DstColor = 0x0306,
		OneMinusDstColor = 0x0307,
		SrcAlphaSaturate = 0x0308,
		ConstantColor = 0x8001,
		OneMinusConstantColor = 0x8002,
		ConstantAlpha = 0x8003,
		OneMinusConstantAlpha = 0x8004
	};
}
