#pragma once

#define NCINE_INCLUDE_OPENGL
#include "../../CommonHeaders.h"
#include "../Backend/BackendEnums.h"

namespace nCine
{
	namespace GLMapping
	{
		inline GLenum PrimitiveType(PrimitiveType type)
		{
			switch (type) {
				case PrimitiveType::Points: return GL_POINTS;
				case PrimitiveType::Lines: return GL_LINES;
				case PrimitiveType::LineStrip: return GL_LINE_STRIP;
				case PrimitiveType::LineLoop: return GL_LINE_LOOP;
				case PrimitiveType::Triangles: return GL_TRIANGLES;
				case PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
				case PrimitiveType::TriangleFan: return GL_TRIANGLE_FAN;
				default: return GL_TRIANGLES;
			}
		}

		inline GLenum BufferUsage(BufferUsage usage)
		{
			switch (usage) {
				case BufferUsage::StreamDraw: return GL_STREAM_DRAW;
				case BufferUsage::StreamRead: return GL_STREAM_READ;
				case BufferUsage::StreamCopy: return GL_STREAM_COPY;
				case BufferUsage::StaticDraw: return GL_STATIC_DRAW;
				case BufferUsage::StaticRead: return GL_STATIC_READ;
				case BufferUsage::StaticCopy: return GL_STATIC_COPY;
				case BufferUsage::DynamicDraw: return GL_DYNAMIC_DRAW;
				case BufferUsage::DynamicRead: return GL_DYNAMIC_READ;
				case BufferUsage::DynamicCopy: return GL_DYNAMIC_COPY;
				default: return GL_DYNAMIC_DRAW;
			}
		}

		inline GLenum BlendingFactor(BlendingFactor factor)
		{
			switch (factor) {
				case BlendingFactor::Zero: return GL_ZERO;
				case BlendingFactor::One: return GL_ONE;
				case BlendingFactor::SrcColor: return GL_SRC_COLOR;
				case BlendingFactor::OneMinusSrcColor: return GL_ONE_MINUS_SRC_COLOR;
				case BlendingFactor::DstColor: return GL_DST_COLOR;
				case BlendingFactor::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
				case BlendingFactor::SrcAlpha: return GL_SRC_ALPHA;
				case BlendingFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
				case BlendingFactor::DstAlpha: return GL_DST_ALPHA;
				case BlendingFactor::OneMinusDstAlpha: return GL_ONE_MINUS_DST_ALPHA;
				case BlendingFactor::ConstantColor: return GL_CONSTANT_COLOR;
				case BlendingFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
				case BlendingFactor::ConstantAlpha: return GL_CONSTANT_ALPHA;
				case BlendingFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
				case BlendingFactor::SrcAlphaSaturate: return GL_SRC_ALPHA_SATURATE;
				default: return GL_ONE;
			}
		}
	}
}
