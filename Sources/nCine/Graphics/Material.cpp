#include "Material.h"
#include "RenderResources.h"
#include "GL/GLShaderProgram.h"
#include "GL/GLUniform.h"
#include "GL/GLTexture.h"
#include "GL/GLMapping.h"
#include "Texture.h"

#include <cstddef> // for offsetof()

namespace nCine
{
	Material::Material()
		: Material(nullptr, nullptr)
	{
	}

	Material::Material(BackendShaderProgram* program, BackendTexture* texture)
		: isBlendingEnabled_(false), srcBlendingFactor_(BlendingFactor::SrcAlpha), destBlendingFactor_(BlendingFactor::OneMinusSrcAlpha),
			shaderProgramType_(ShaderProgramType::Custom), shaderProgram_(program), uniformsHostBufferSize_(0)
	{
		for (std::uint32_t i = 0; i < BackendTexture::MaxTextureUnits; i++) {
			textures_[i] = nullptr;
		}
		textures_[0] = texture;

		if (program != nullptr) {
			SetShaderProgram(program);
		}
	}

	void Material::SetBlendingFactors(BlendingFactor srcBlendingFactor, BlendingFactor destBlendingFactor)
	{
		srcBlendingFactor_ = srcBlendingFactor;
		destBlendingFactor_ = destBlendingFactor;
	}

	bool Material::SetShaderProgramType(ShaderProgramType shaderProgramType)
	{
		BackendShaderProgram* shaderProgram = RenderResources::GetShaderProgram(shaderProgramType);
		if (shaderProgram == nullptr || shaderProgram == shaderProgram_) {
			return false;
		}

		SetShaderProgram(shaderProgram);

		// Should be assigned after calling `setShaderProgram()`
		shaderProgramType_ = shaderProgramType;
		return true;
	}

	void Material::SetShaderProgram(BackendShaderProgram* program)
	{
		// Allow self-assignment to take into account the case where the shader program loads new shaders

		shaderProgramType_ = ShaderProgramType::Custom;
		shaderProgram_ = program;
		// The camera uniforms are handled separately as they have a different update frequency
		shaderUniforms_.SetProgram(shaderProgram_, nullptr, ProjectionViewMatrixExcludeString);
		shaderUniformBlocks_.SetProgram(shaderProgram_);

		RenderResources::SetDefaultAttributesParameters(*shaderProgram_);
	}

	bool Material::SetShader(Shader* shader)
	{
		BackendShaderProgram* shaderProgram = shader->GetHandle();
		if (shaderProgram == shaderProgram_) {
			return false;
		}

		SetShaderProgram(shaderProgram);
		return true;
	}

	void Material::SetDefaultAttributesParameters()
	{
		RenderResources::SetDefaultAttributesParameters(*shaderProgram_);
	}

	void Material::ReserveUniformsDataMemory()
	{
		DEATH_ASSERT(shaderProgram_);

		// Total memory size for all uniforms and uniform blocks
		const std::uint32_t uniformsSize = shaderProgram_->GetUniformsSize() + shaderProgram_->GetUniformBlocksSize();
		if (uniformsSize > uniformsHostBufferSize_) {
			uniformsHostBuffer_ = std::make_unique<std::uint8_t[]>(uniformsSize);
			uniformsHostBufferSize_ = uniformsSize;
		}
		std::uint8_t* dataPointer = uniformsHostBuffer_.get();
		uniformsDataPointer_ = dataPointer;
		uniformsDataSize_ = uniformsSize;
		shaderUniforms_.SetUniformsDataPointer(dataPointer);
		shaderUniformBlocks_.SetUniformsDataPointer(&dataPointer[shaderProgram_->GetUniformsSize()]);
	}

	void Material::SetUniformsDataPointer(std::uint8_t* dataPointer)
	{
		DEATH_ASSERT(shaderProgram_);
		DEATH_ASSERT(dataPointer);

		uniformsHostBuffer_.reset(nullptr);
		uniformsHostBufferSize_ = 0;
		uniformsDataPointer_ = dataPointer;
		uniformsDataSize_ = shaderProgram_->GetUniformsSize() + shaderProgram_->GetUniformBlocksSize();
		shaderUniforms_.SetUniformsDataPointer(dataPointer);
		shaderUniformBlocks_.SetUniformsDataPointer(&dataPointer[shaderProgram_->GetUniformsSize()]);
	}

	const BackendTexture* Material::GetTexture(std::uint32_t unit) const
	{
		const BackendTexture* texture = nullptr;
		if (unit < BackendTexture::MaxTextureUnits) {
			texture = textures_[unit];
		}
		return texture;
	}

	bool Material::SetTexture(std::uint32_t unit, const BackendTexture* texture)
	{
		bool result = false;
		if (unit < BackendTexture::MaxTextureUnits) {
			textures_[unit] = texture;
			result = true;
		}
		return result;
	}

	bool Material::SetTexture(std::uint32_t unit, const Texture& texture)
	{
		return SetTexture(unit, texture.glTexture_.get());
	}

	bool Material::SetTexture(std::uint32_t unit, std::nullptr_t)
	{
		bool result = false;
		if (unit < BackendTexture::MaxTextureUnits) {
			textures_[unit] = nullptr;
			result = true;
		}
		return result;
	}

	void Material::Bind()
	{
		for (std::uint32_t i = 0; i < BackendTexture::MaxTextureUnits; i++) {
			if (textures_[i] != nullptr) {
				textures_[i]->Bind(i);
			} else {
				BackendTexture::Unbind(i);
			}
		}

		if (shaderProgram_) {
			shaderProgram_->Use();
			shaderUniformBlocks_.Bind();
		}
	}

	void Material::DefineVertexFormat(const BackendBufferObject* vbo, const BackendBufferObject* ibo, std::uint32_t vboOffset)
	{
		shaderProgram_->DefineVertexFormat(vbo, ibo, vboOffset);
	}

	namespace
	{
		struct SortHashData
		{
			std::uint32_t textures[BackendTexture::MaxTextureUnits];
			std::uint32_t shaderProgram;
			std::uint8_t srcBlendingFactor;
			std::uint8_t destBlendingFactor;
		};
	}

	std::uint32_t Material::GetSortKey()
	{
		constexpr std::uint32_t Seed = 1697381921;
		// Align to 64 bits for `fasthash64()` to properly work on Emscripten without alignment faults
		static SortHashData hashData alignas(8);

		for (std::uint32_t i = 0; i < BackendTexture::MaxTextureUnits; i++) {
			hashData.textures[i] = (textures_[i] != nullptr) ? (std::uint32_t)textures_[i]->GetGLHandle() : 0;
		}
		hashData.shaderProgram = (std::uint32_t)shaderProgram_->GetGLHandle();
		hashData.srcBlendingFactor = static_cast<std::uint8_t>(srcBlendingFactor_);
		hashData.destBlendingFactor = static_cast<std::uint8_t>(destBlendingFactor_);

		return (std::uint32_t)xxHash3(reinterpret_cast<const void*>(&hashData), sizeof(SortHashData), Seed);
	}
}

