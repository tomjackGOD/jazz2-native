#include "RenderResources.h"
#include "BinaryShaderCache.h"
#include "RenderBuffersManager.h"
#include "RenderVaoPool.h"
#include "RenderCommandPool.h"
#include "RenderBatcher.h"
#include "Camera.h"
#include "IGfxCapabilities.h"
#include "../Application.h"
#include "../ServiceLocator.h"
#include "../../Main.h"

#include <cstddef>	// for offsetof()
#include <cstring>

#if defined(WITH_EMBEDDED_SHADERS)
#	include "shader_strings.h"
#else
#	include <Containers/DateTime.h>
#	include <IO/FileSystem.h>
using namespace Death::IO;
#endif

using namespace Death::Containers::Literals;

namespace nCine
{
	namespace
	{
		static const char BatchSizeFormatString[] = "#ifndef BATCH_SIZE\n\t#define BATCH_SIZE ({})\n#endif\n#line 0\n";

		struct ShaderLoad
		{
			std::unique_ptr<BackendShaderProgram>& shaderProgram;
			const char* vertexShader;
			const char* fragmentShader;
			BackendShaderProgram::Introspection introspection;
			const char* shaderName;
		};
	}

	std::unique_ptr<BinaryShaderCache> RenderResources::binaryShaderCache_;
	std::unique_ptr<RenderBuffersManager> RenderResources::buffersManager_;
	std::unique_ptr<RenderVaoPool> RenderResources::vaoPool_;
	std::unique_ptr<RenderCommandPool> RenderResources::renderCommandPool_;
	std::unique_ptr<RenderBatcher> RenderResources::renderBatcher_;

	std::unique_ptr<BackendShaderProgram> RenderResources::defaultShaderPrograms_[DefaultShaderProgramsCount];
	HashMap<const BackendShaderProgram*, BackendShaderProgram*> RenderResources::batchedShaders_(32);

	std::uint8_t RenderResources::cameraUniformsBuffer_[UniformsBufferSize];
	HashMap<BackendShaderProgram*, RenderResources::CameraUniformData> RenderResources::cameraUniformDataMap_(32);

	Camera* RenderResources::currentCamera_ = nullptr;
	std::unique_ptr<Camera> RenderResources::defaultCamera_;
	Viewport* RenderResources::currentViewport_ = nullptr;

	BackendShaderProgram* RenderResources::GetShaderProgram(Material::ShaderProgramType shaderProgramType)
	{
		return (shaderProgramType != Material::ShaderProgramType::Custom ? defaultShaderPrograms_[std::int32_t(shaderProgramType)].get() : nullptr);
	}

	BackendShaderProgram* RenderResources::GetBatchedShader(const BackendShaderProgram* shader)
	{
		auto it = batchedShaders_.find(shader);
		return (it != batchedShaders_.end() ? it->second : nullptr);
	}

	bool RenderResources::RegisterBatchedShader(const BackendShaderProgram* shader, BackendShaderProgram* batchedShader)
	{
		FATAL_ASSERT(shader != nullptr);
		FATAL_ASSERT(batchedShader != nullptr);
		FATAL_ASSERT(shader != batchedShader);

		return batchedShaders_.emplace(shader, batchedShader).second;
	}

	bool RenderResources::UnregisterBatchedShader(const BackendShaderProgram* shader)
	{
		DEATH_ASSERT(shader != nullptr);
		return (batchedShaders_.erase(shader) > 0);
	}

	RenderResources::CameraUniformData* RenderResources::FindCameraUniformData(BackendShaderProgram* shaderProgram)
	{
		auto it = cameraUniformDataMap_.find(shaderProgram);
		return (it != cameraUniformDataMap_.end() ? &it->second : nullptr);
	}

	void RenderResources::InsertCameraUniformData(BackendShaderProgram* shaderProgram, CameraUniformData&& cameraUniformData)
	{
		FATAL_ASSERT(shaderProgram != nullptr);

		//if (cameraUniformDataMap_.loadFactor() >= 0.8f)
		//	cameraUniformDataMap_.rehash(cameraUniformDataMap_.capacity() * 2);

		cameraUniformDataMap_.emplace(shaderProgram, std::move(cameraUniformData));
	}

	bool RenderResources::RemoveCameraUniformData(BackendShaderProgram* shaderProgram)
	{
		return cameraUniformDataMap_.erase(shaderProgram);
	}

	void RenderResources::SetDefaultAttributesParameters(BackendShaderProgram& shaderProgram)
	{
		if (shaderProgram.GetAttributeCount() <= 0) {
			return;
		}

		shaderProgram.DefineDefaultAttributes(Material::PositionAttributeName, Material::TexCoordsAttributeName, Material::MeshIndexAttributeName);
	}

	const char* RenderResources::GetDefaultVertexShaderSource(Shader::DefaultVertex vertex)
	{
#if defined(WITH_EMBEDDED_SHADERS)
#if defined(DEATH_TARGET_IOS)
		return GetDefaultVertexShaderSourceMetal(vertex);
#else
		switch (vertex) {
			case Shader::DefaultVertex::SPRITE: return ShaderStrings::sprite_vs + 1;
			case Shader::DefaultVertex::SPRITE_NOTEXTURE: return ShaderStrings::sprite_notexture_vs + 1;
			case Shader::DefaultVertex::MESHSPRITE: return ShaderStrings::meshsprite_vs + 1;
			case Shader::DefaultVertex::MESHSPRITE_NOTEXTURE: return ShaderStrings::meshsprite_notexture_vs + 1;
			case Shader::DefaultVertex::BATCHED_SPRITES: return ShaderStrings::batched_sprites_vs + 1;
			case Shader::DefaultVertex::BATCHED_SPRITES_NOTEXTURE: return ShaderStrings::batched_sprites_notexture_vs + 1;
			case Shader::DefaultVertex::BATCHED_MESHSPRITES: return ShaderStrings::batched_meshsprites_vs + 1;
			case Shader::DefaultVertex::BATCHED_MESHSPRITES_NOTEXTURE: return ShaderStrings::batched_meshsprites_notexture_vs + 1;
			default: return nullptr;
		}
#endif
#else
		// Logic for loading from file is handled in Shader::LoadFromFile
		return nullptr;
#endif
	}

	const char* RenderResources::GetDefaultFragmentShaderSource(Shader::DefaultFragment fragment)
	{
#if defined(WITH_EMBEDDED_SHADERS)
#if defined(DEATH_TARGET_IOS)
		return GetDefaultFragmentShaderSourceMetal(fragment);
#else
		switch (fragment) {
			case Shader::DefaultFragment::SPRITE: return ShaderStrings::sprite_fs + 1;
			case Shader::DefaultFragment::SPRITE_NOTEXTURE: return ShaderStrings::sprite_notexture_fs + 1;
			default: return nullptr;
		}
#endif
#else
		// Logic for loading from file is handled in Shader::LoadFromFile
		return nullptr;
#endif
	}

#if defined(DEATH_TARGET_IOS)
	const char* RenderResources::GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex vertex)
	{
#if defined(WITH_EMBEDDED_SHADERS)
		switch (vertex) {
			case Shader::DefaultVertex::SPRITE: return ShaderStrings::sprite_vs_metal + 1;
			case Shader::DefaultVertex::SPRITE_NOTEXTURE: return ShaderStrings::sprite_notexture_vs_metal + 1;
			case Shader::DefaultVertex::MESHSPRITE: return ShaderStrings::meshsprite_vs_metal + 1;
			case Shader::DefaultVertex::MESHSPRITE_NOTEXTURE: return ShaderStrings::meshsprite_notexture_vs_metal + 1;
			case Shader::DefaultVertex::BATCHED_SPRITES: return ShaderStrings::batched_sprites_vs_metal + 1;
			case Shader::DefaultVertex::BATCHED_SPRITES_NOTEXTURE: return ShaderStrings::batched_sprites_notexture_vs_metal + 1;
			case Shader::DefaultVertex::BATCHED_MESHSPRITES: return ShaderStrings::batched_meshsprites_vs_metal + 1;
			case Shader::DefaultVertex::BATCHED_MESHSPRITES_NOTEXTURE: return ShaderStrings::batched_meshsprites_notexture_vs_metal + 1;
			default: return nullptr;
		}
#else
		return nullptr;
#endif
	}

	const char* RenderResources::GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment fragment)
	{
#if defined(WITH_EMBEDDED_SHADERS)
		switch (fragment) {
			case Shader::DefaultFragment::SPRITE: return ShaderStrings::sprite_fs_metal + 1;
			case Shader::DefaultFragment::SPRITE_NOTEXTURE: return ShaderStrings::sprite_notexture_fs_metal + 1;
			default: return nullptr;
		}
#else
		return nullptr;
#endif
	}
#endif

	void RenderResources::SetCurrentCamera(Camera* camera)
	{
		currentCamera_ = (camera != nullptr ? camera : defaultCamera_.get());
	}

	void RenderResources::UpdateCameraUniforms()
	{
		// The buffer is shared among every shader program. There is no need to call `setFloatVector()` as `setDirty()` is enough.
		std::memcpy(cameraUniformsBuffer_, currentCamera_->GetProjection().Data(), 64);
		std::memcpy(cameraUniformsBuffer_ + 64, currentCamera_->GetView().Data(), 64);
		for (auto i = cameraUniformDataMap_.begin(); i != cameraUniformDataMap_.end(); ++i) {
			CameraUniformData& cameraUniformData = i->second;

			if (cameraUniformData.camera != currentCamera_) {
				i->second.shaderUniforms.SetDirty(true);
				cameraUniformData.camera = currentCamera_;
			} else {
				if (cameraUniformData.updateFrameProjectionMatrix < currentCamera_->UpdateFrameProjectionMatrix()) {
					i->second.shaderUniforms.GetUniform(Material::ProjectionMatrixUniformName)->SetDirty(true);
				}
				if (cameraUniformData.updateFrameViewMatrix < currentCamera_->UpdateFrameViewMatrix()) {
					i->second.shaderUniforms.GetUniform(Material::ViewMatrixUniformName)->SetDirty(true);
				}
			}

			cameraUniformData.updateFrameProjectionMatrix = currentCamera_->UpdateFrameProjectionMatrix();
			cameraUniformData.updateFrameViewMatrix = currentCamera_->UpdateFrameViewMatrix();
		}
	}

	void RenderResources::SetCurrentViewport(Viewport* viewport)
	{
		FATAL_ASSERT(viewport != nullptr);
		currentViewport_ = viewport;
	}

	void RenderResources::CreateMinimal()
	{
		// `CreateMinimal()` cannot be called after `Create()`
		DEATH_ASSERT(binaryShaderCache_ == nullptr);
		DEATH_ASSERT(buffersManager_ == nullptr);
		DEATH_ASSERT(vaoPool_ == nullptr);
	
		const AppConfiguration& appCfg = theApplication().GetAppConfiguration();
		binaryShaderCache_ = std::make_unique<BinaryShaderCache>(appCfg.shaderCachePath);
#if !defined(DEATH_TARGET_IOS)
		buffersManager_ = std::make_unique<RenderBuffersManager>(appCfg.useBufferMapping, appCfg.vboSize, appCfg.iboSize);
		vaoPool_ = std::make_unique<RenderVaoPool>(appCfg.vaoPoolSize);
#endif
	}
	
	void RenderResources::Create()
	{
		// `Create()` can be called after `CreateMinimal()`

		const AppConfiguration& appCfg = theApplication().GetAppConfiguration();
		if (binaryShaderCache_ == nullptr) {
			binaryShaderCache_ = std::make_unique<BinaryShaderCache>(appCfg.shaderCachePath);
		}
#if !defined(DEATH_TARGET_IOS)
		if (buffersManager_ == nullptr) {
			buffersManager_ = std::make_unique<RenderBuffersManager>(appCfg.useBufferMapping, appCfg.vboSize, appCfg.iboSize);
		}
		if (vaoPool_ == nullptr) {
			vaoPool_ = std::make_unique<RenderVaoPool>(appCfg.vaoPoolSize);
		}
#endif
		renderCommandPool_ = std::make_unique<RenderCommandPool>(appCfg.renderCommandPoolSize);
		renderBatcher_ = std::make_unique<RenderBatcher>();
		defaultCamera_ = std::make_unique<Camera>();
		currentCamera_ = defaultCamera_.get();

#if defined(DEATH_TARGET_IOS)
		ShaderLoad shadersToLoad[] = {
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::Sprite)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::SPRITE), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE), BackendShaderProgram::Introspection::Enabled, "Sprite" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::SpriteNoTexture)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::SPRITE_NOTEXTURE), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE_NOTEXTURE), BackendShaderProgram::Introspection::Enabled, "Sprite_NoTexture" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSprite)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::MESHSPRITE), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE), BackendShaderProgram::Introspection::Enabled, "MeshSprite" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSpriteNoTexture)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::MESHSPRITE_NOTEXTURE), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE_NOTEXTURE), BackendShaderProgram::Introspection::Enabled, "MeshSprite_NoTexture" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSprites)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::BATCHED_SPRITES), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE), BackendShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSpritesNoTexture)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::BATCHED_SPRITES_NOTEXTURE), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE_NOTEXTURE), BackendShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites_NoTexture" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSprites)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::BATCHED_MESHSPRITES), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE), BackendShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSpritesNoTexture)], GetDefaultVertexShaderSourceMetal(Shader::DefaultVertex::BATCHED_MESHSPRITES_NOTEXTURE), GetDefaultFragmentShaderSourceMetal(Shader::DefaultFragment::SPRITE_NOTEXTURE), BackendShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites_NoTexture" },
		};

		for (std::uint32_t i = 0; i < std::uint32_t(arraySize(shadersToLoad)); i++) {
			const ShaderLoad& shaderToLoad = shadersToLoad[i];
			shaderToLoad.shaderProgram = std::make_unique<BackendShaderProgram>(BackendShaderProgram::QueryPhase::Immediate);
			
			// Set batch size for batched shaders
			if (shaderToLoad.introspection == BackendShaderProgram::Introspection::NoUniformsInBlocks) {
				// Use a default batch size for Metal (64KB / 112 bytes ~= 585)
				shaderToLoad.shaderProgram->SetBatchSize(585);
			}

			shaderToLoad.shaderProgram->AttachShaderFromString(0x8B31, shaderToLoad.vertexShader); // Vertex
			shaderToLoad.shaderProgram->AttachShaderFromString(0x8B30, shaderToLoad.fragmentShader); // Fragment
			
			SetDefaultAttributesParameters(*shaderToLoad.shaderProgram);
			shaderToLoad.shaderProgram->Link(shaderToLoad.introspection);
			shaderToLoad.shaderProgram->SetObjectLabel(shaderToLoad.shaderName);
		}

		RegisterDefaultBatchedShaders();
#else
		ShaderLoad shadersToLoad[] = {
#if defined(WITH_EMBEDDED_SHADERS)
			// Skipping the initial new line character of the raw string literal
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::Sprite)], ShaderStrings::sprite_vs + 1, ShaderStrings::sprite_fs + 1, GLShaderProgram::Introspection::Enabled, "Sprite" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::SpriteGray)], ShaderStrings::sprite_vs + 1, ShaderStrings::sprite_gray_fs + 1, GLShaderProgram::Introspection::Enabled, "Sprite_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::SpriteNoTexture)], ShaderStrings::sprite_notexture_vs + 1, ShaderStrings::sprite_notexture_fs + 1, GLShaderProgram::Introspection::Enabled, "Sprite_NoTexture" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSprite)], ShaderStrings::meshsprite_vs + 1, ShaderStrings::sprite_fs + 1, GLShaderProgram::Introspection::Enabled, "MeshSprite" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSpriteGray)], ShaderStrings::meshsprite_vs + 1, ShaderStrings::sprite_gray_fs + 1, GLShaderProgram::Introspection::Enabled, "MeshSprite_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSpriteNoTexture)], ShaderStrings::meshsprite_notexture_vs + 1, ShaderStrings::sprite_notexture_fs + 1, GLShaderProgram::Introspection::Enabled, "MeshSprite_NoTexture" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::TextNodeAlpha)], ShaderStrings::textnode_vs + 1, ShaderStrings::textnode_alpha_fs + 1, GLShaderProgram::Introspection::Enabled, "TextNode_Alpha" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::TextNodeRed)], ShaderStrings::textnode_vs + 1, ShaderStrings::textnode_red_fs + 1, GLShaderProgram::Introspection::Enabled, "TextNode_Red" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSprites)], ShaderStrings::batched_sprites_vs + 1, ShaderStrings::sprite_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSpritesGray)], ShaderStrings::batched_sprites_vs + 1, ShaderStrings::sprite_gray_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSpritesNoTexture)], ShaderStrings::batched_sprites_notexture_vs + 1, ShaderStrings::sprite_notexture_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites_NoTexture" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSprites)], ShaderStrings::batched_meshsprites_vs + 1, ShaderStrings::sprite_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSpritesGray)], ShaderStrings::batched_meshsprites_vs + 1, ShaderStrings::sprite_gray_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSpritesNoTexture)], ShaderStrings::batched_meshsprites_notexture_vs + 1, ShaderStrings::sprite_notexture_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites_NoTexture" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedTextNodesAlpha)], ShaderStrings::batched_textnodes_vs + 1, ShaderStrings::textnode_alpha_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_TextNodes_Alpha" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedTextNodesRed)], ShaderStrings::batched_textnodes_vs + 1, ShaderStrings::textnode_red_fs + 1, GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_TextNodes_Red" }
#else
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::Sprite)], "sprite_vs.glsl", "sprite_fs.glsl", GLShaderProgram::Introspection::Enabled, "Sprite" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::SpriteGray)], "sprite_vs.glsl", "sprite_gray_fs.glsl", GLShaderProgram::Introspection::Enabled, "Sprite_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::SpriteNoTexture)], "sprite_notexture_vs.glsl", "sprite_notexture_fs.glsl", GLShaderProgram::Introspection::Enabled, "Sprite_NoTexture" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSprite)], "meshsprite_vs.glsl", "sprite_fs.glsl", GLShaderProgram::Introspection::Enabled, "MeshSprite" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSpriteGray)], "meshsprite_vs.glsl", "sprite_gray_fs.glsl", GLShaderProgram::Introspection::Enabled, "MeshSprite_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSpriteNoTexture)], "meshsprite_notexture_vs.glsl", "sprite_notexture_fs.glsl", GLShaderProgram::Introspection::Enabled, "MeshSprite_NoTexture" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::TextNodeAlpha)], "textnode_vs.glsl", "textnode_alpha_fs.glsl", GLShaderProgram::Introspection::Enabled, "TextNode_Alpha" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::TextNodeRed)], "textnode_vs.glsl", "textnode_red_fs.glsl", GLShaderProgram::Introspection::Enabled, "TextNode_Red" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSprites)], "batched_sprites_vs.glsl", "sprite_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSpritesGray)], "batched_sprites_vs.glsl", "sprite_gray_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSpritesNoTexture)], "batched_sprites_notexture_vs.glsl", "sprite_notexture_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_Sprites_NoTexture" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSprites)], "batched_meshsprites_vs.glsl", "sprite_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSpritesGray)], "batched_meshsprites_vs.glsl", "sprite_gray_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites_Gray" },
			{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSpritesNoTexture)], "batched_meshsprites_notexture_vs.glsl", "sprite_notexture_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_MeshSprites_NoTexture" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedTextNodesAlpha)], "batched_textnodes_vs.glsl", "textnode_alpha_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_TextNodes_Alpha" },
			//{ RenderResources::defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedTextNodesRed)], "batched_textnodes_vs.glsl", "textnode_red_fs.glsl", GLShaderProgram::Introspection::NoUniformsInBlocks, "Batched_TextNodes_Red" }
#endif
		};

		const IGfxCapabilities& gfxCaps = theServiceLocator().GetGfxCapabilities();
		std::int32_t maxUniformBlockSize = gfxCaps.GetValue(IGfxCapabilities::GLIntValues::MAX_UNIFORM_BLOCK_SIZE_NORMALIZED);

		char sourceString[64];
		StringView vertexStrings[3];

		for (std::uint32_t i = 0; i < std::uint32_t(arraySize(shadersToLoad)); i++) {
			const ShaderLoad& shaderToLoad = shadersToLoad[i];

#if defined(WITH_EMBEDDED_SHADERS)
			constexpr std::uint64_t shaderVersion = EmbeddedShadersVersion;
#else
			String vertexPath = fs::CombinePath({ theApplication().GetDataPath(), "Shaders"_s, StringView(shaderToLoad.vertexShader) });
			String fragmentPath = fs::CombinePath({ theApplication().GetDataPath(), "Shaders"_s, StringView(shaderToLoad.fragmentShader) });
			std::uint64_t shaderVersion = (std::uint64_t)std::max(fs::GetLastModificationTime(vertexPath).ToUnixMilliseconds(), fs::GetLastModificationTime(fragmentPath).ToUnixMilliseconds());
#endif

			shaderToLoad.shaderProgram = std::make_unique<GLShaderProgram>(GLShaderProgram::QueryPhase::Immediate);
			if (binaryShaderCache_->LoadFromCache(shaderToLoad.shaderName, shaderVersion, shaderToLoad.shaderProgram.get(), shaderToLoad.introspection)) {
				// Shader is already compiled and up-to-date
				continue;
			}

			// If the UBO is smaller than 64kb and fixed batch size is disabled, batched shaders need to be compiled twice to determine safe `BATCH_SIZE` define value
			bool compileTwice = false;
			std::int32_t stringsCount = 0;

			if (appCfg.fixedBatchSize > 0 && shaderToLoad.introspection == GLShaderProgram::Introspection::NoUniformsInBlocks) {
				// If fixed batch size is used, it's compiled only once with specified batch size
				shaderToLoad.shaderProgram->SetBatchSize(appCfg.fixedBatchSize);

				std::size_t length = formatInto(sourceString, BatchSizeFormatString, appCfg.fixedBatchSize);
				vertexStrings[stringsCount++] = { sourceString, length };
#if defined(WITH_EMBEDDED_SHADERS)
				vertexStrings[stringsCount++] = shaderToLoad.vertexShader;
#endif
			} else if (shaderToLoad.introspection == GLShaderProgram::Introspection::NoUniformsInBlocks && maxUniformBlockSize < 64 * 1024) {
				compileTwice = true;

				// The first compilation of a batched shader needs a `BATCH_SIZE` defined as 1
				std::size_t length = formatInto(sourceString, BatchSizeFormatString, 1);
				vertexStrings[stringsCount++] = { sourceString, length };
#if defined(WITH_EMBEDDED_SHADERS)
				vertexStrings[stringsCount++] = shaderToLoad.vertexShader;
#endif
			} else {
#if defined(WITH_EMBEDDED_SHADERS)
				vertexStrings[stringsCount++] = shaderToLoad.vertexShader;
#endif
			}
			
#if defined(WITH_EMBEDDED_SHADERS)
			bool vertexCompiled = shaderToLoad.shaderProgram->AttachShaderFromStrings(GL_VERTEX_SHADER, arrayView(vertexStrings, stringsCount));
			bool fragmentCompiled = shaderToLoad.shaderProgram->AttachShaderFromString(GL_FRAGMENT_SHADER, shaderToLoad.fragmentShader);
#else
			bool vertexCompiled = shaderToLoad.shaderProgram->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(vertexStrings, stringsCount), vertexPath);
			bool fragmentCompiled = shaderToLoad.shaderProgram->AttachShaderFromFile(GL_FRAGMENT_SHADER, fragmentPath);
#endif
			DEATH_ASSERT(vertexCompiled);
			DEATH_ASSERT(fragmentCompiled);
			
			shaderToLoad.shaderProgram->SetObjectLabel(shaderToLoad.shaderName);

			if (compileTwice) {
				bool hasLinked = shaderToLoad.shaderProgram->Link(GLShaderProgram::Introspection::Enabled);
				FATAL_ASSERT(hasLinked);

				GLShaderUniformBlocks blocks(shaderToLoad.shaderProgram.get(), Material::InstancesBlockName, nullptr);
				GLUniformBlockCache* block = blocks.GetUniformBlock(Material::InstancesBlockName);
				if (block != nullptr) {
					std::int32_t batchSize = maxUniformBlockSize / block->GetSize();
					LOGI("Shader \"{}\" - block size: {} + {} align bytes, max batch size: {}", shaderToLoad.shaderName,
						block->GetSize() - block->GetAlignAmount(), block->GetAlignAmount(), batchSize);

					bool hasLinkedFinal = false;
					while (batchSize > 0) {
						shaderToLoad.shaderProgram->Reset();
						shaderToLoad.shaderProgram->SetBatchSize(batchSize);
						std::size_t length = formatInto(sourceString, BatchSizeFormatString, batchSize);
						vertexStrings[0] = { sourceString, length };

#if defined(WITH_EMBEDDED_SHADERS)
						bool vertexFinalCompiled = shaderToLoad.shaderProgram->AttachShaderFromStrings(GL_VERTEX_SHADER, arrayView(vertexStrings, stringsCount));
						bool fragmentFinalCompiled = shaderToLoad.shaderProgram->AttachShaderFromString(GL_FRAGMENT_SHADER, shaderToLoad.fragmentShader);
#else
						bool vertexFinalCompiled = shaderToLoad.shaderProgram->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(vertexStrings, stringsCount), vertexPath);
						bool fragmentFinalCompiled = shaderToLoad.shaderProgram->AttachShaderFromFile(GL_FRAGMENT_SHADER, fragmentPath);
#endif
						if (vertexFinalCompiled && fragmentFinalCompiled) {
							hasLinkedFinal = shaderToLoad.shaderProgram->Link(shaderToLoad.introspection);
							if (hasLinkedFinal) {
								break;
							}
						}

						batchSize--;
						LOGW("Failed to compile the shader, recompiling with batch size: {}", batchSize);
					}

					FATAL_ASSERT_MSG(hasLinkedFinal, "Failed to compile shader \"{}\"", shaderToLoad.shaderName);
				}
			} else {
				bool hasLinked = shaderToLoad.shaderProgram->Link(shaderToLoad.introspection);
				FATAL_ASSERT_MSG(hasLinked, "Failed to compile shader \"{}\"", shaderToLoad.shaderName);
			}

			binaryShaderCache_->SaveToCache(shaderToLoad.shaderName, shaderVersion, shaderToLoad.shaderProgram.get());
		}

		RegisterDefaultBatchedShaders();
#endif

		// Calculating a default projection matrix for all shader programs
		auto res = theApplication().GetResolution();
		defaultCamera_->SetOrthoProjection(0.0f, float(res.X), 0.0f, float(res.Y));
	}

	void RenderResources::Dispose()
	{
		for (auto& shaderProgram : defaultShaderPrograms_) {
			shaderProgram.reset(nullptr);
		}

		DEATH_ASSERT(cameraUniformDataMap_.empty());

		defaultCamera_.reset(nullptr);
		renderBatcher_.reset(nullptr);
		renderCommandPool_.reset(nullptr);
		vaoPool_.reset(nullptr);
		buffersManager_.reset(nullptr);

		LOGI("Rendering resources disposed");
	}

	void RenderResources::RegisterDefaultBatchedShaders()
	{
		batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::Sprite)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSprites)].get());
		//batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::SpriteGray)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSpritesGray)].get());
		batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::SpriteNoTexture)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedSpritesNoTexture)].get());
		batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSprite)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSprites)].get());
		//batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSpriteGray)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSpritesGray)].get());
		batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::MeshSpriteNoTexture)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedMeshSpritesNoTexture)].get());
		//batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::TextNodeAlpha)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedTextNodesAlpha)].get());
		//batchedShaders_.emplace(defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::TextNodeRed)].get(), defaultShaderPrograms_[std::int32_t(Material::ShaderProgramType::BatchedTextNodesRed)].get());
	}
}
