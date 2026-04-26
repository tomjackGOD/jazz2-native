#pragma once

/**
 * Backend type aliases for render resources.
 *
 * This file provides backend-agnostic aliases for graphics resources.
 * Engine-level objects (Material/Shader/Texture/Geometry) reference these
 * aliases instead of hardcoding GL types.
 */

#if defined(DEATH_TARGET_IOS)
#include "Backends/iOS/MetalShaderProgram.h"
#include "Backends/iOS/MetalTexture.h"
#include "Backends/iOS/MetalBufferObject.h"

namespace nCine
{
	// `BackendShaderProgram`, `BackendShaderUniforms`, `BackendShaderUniformBlocks`,
	// `BackendTexture`, and `BackendBufferObject` are provided by the included Metal headers.
	using BackendUniformCache = MetalUniformCache;
	using BackendUniformBlockCache = MetalUniformBlockCache;
}
#else
namespace nCine
{
	class GLShaderProgram;
	class GLShaderUniforms;
	class GLShaderUniformBlocks;
	class GLTexture;
	class GLBufferObject;
	class GLUniformCache;
	class GLUniformBlockCache;

	using BackendShaderProgram = GLShaderProgram;
	using BackendShaderUniforms = GLShaderUniforms;
	using BackendShaderUniformBlocks = GLShaderUniformBlocks;
	using BackendTexture = GLTexture;
	using BackendBufferObject = GLBufferObject;
	using BackendUniformCache = GLUniformCache;
	using BackendUniformBlockCache = GLUniformBlockCache;
}
#endif


