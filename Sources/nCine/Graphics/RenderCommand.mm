#include "RenderCommand.h"
#if !defined(DEATH_TARGET_IOS)
#	include "GL/GLShaderProgram.h"
#endif
#include "RenderResources.h"
#include "Camera.h"
#include "DrawableNode.h"
#include "../tracy.h"
#include "Backend/BackendRenderState.h"
#include <cstring>
#include <chrono>
#include <fstream>
#include <string>

namespace nCine
{
	namespace
	{
		inline void DebugLog(const char* hypothesisId, const char* location, const char* message, const std::string& dataJson)
		{
			const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
			std::ofstream debugFile("/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/.cursor/debug-7a1d37.log", std::ios::app);
			if (!debugFile.is_open()) {
				return;
			}
			debugFile << "{\"sessionId\":\"7a1d37\",\"runId\":\"initial\",\"hypothesisId\":\"" << hypothesisId
				<< "\",\"location\":\"" << location << "\",\"message\":\"" << message << "\",\"data\":"
				<< dataJson << ",\"timestamp\":" << nowMs << "}\n";
		}
	}

	RenderCommand::RenderCommand(Type type)
		: materialSortKey_(0), layer_(0), numInstances_(0), batchSize_(0), transformationCommitted_(false), modelMatrix_(Matrix4x4f::Identity)
#if defined(NCINE_PROFILING)
			, type_(type)
#endif
	{
	}

	RenderCommand::RenderCommand()
		: RenderCommand(Type::Unspecified)
	{
	}

	void RenderCommand::CalculateMaterialSortKey()
	{
		const std::uint64_t upper = std::uint64_t(GetLayerSortKey()) << 32;
		const std::uint32_t lower = material_.GetSortKey();
		materialSortKey_ = upper | lower;
	}

	void RenderCommand::Issue()
	{
		ZoneScopedC(0x81A861);
		// #region agent log
		DebugLog("H4", "RenderCommand.mm:56", "RenderCommand::Issue entered",
			"{\"numVertices\":" + std::to_string(geometry_.numVertices_) +
			",\"numIndices\":" + std::to_string(geometry_.numIndices_) + "}");
		// #endregion

#if defined(DEATH_TARGET_IOS)
		if (geometry_.numVertices_ == 0 && geometry_.numIndices_ == 0) {
			// #region agent log
			DebugLog("H4", "RenderCommand.mm:64", "iOS early return: empty geometry", "{}");
			// #endregion
			return;
		}

		id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)Backends::MetalRenderState::currentEncoder();
		if (encoder == nil) {
			// #region agent log
			DebugLog("H4", "RenderCommand.mm:72", "iOS early return: encoder nil", "{}");
			// #endregion
			return;
		}

		BackendShaderProgram* program = material_.shaderProgram_;
		if (program == nullptr || !program->IsLinked()) {
			// #region agent log
			DebugLog("H4", "RenderCommand.mm:81", "iOS early return: program missing or unlinked",
				"{\"programNull\":" + std::to_string(program == nullptr ? 1 : 0) + "}");
			// #endregion
			return;
		}

		// Validation: catch mismatched uniform buffer sizing early
		const std::uint32_t expectedUniformBytes = program->GetUniformsSize() + program->GetUniformBlocksSize();
		// #region agent log
		DebugLog("H1", "RenderCommand.mm:74", "Uniform validation state before mismatch check",
			"{\"materialUniformBytes\":" + std::to_string(material_.uniformsDataSize_) +
			",\"expectedUniformBytes\":" + std::to_string(expectedUniformBytes) +
			",\"hasUniformPointer\":" + std::to_string(material_.uniformsDataPointer_ != nullptr ? 1 : 0) + "}");
		// #endregion
		if (material_.uniformsDataPointer_ != nullptr && expectedUniformBytes > 0 && material_.uniformsDataSize_ != expectedUniformBytes) {
			// #region agent log
			DebugLog("H2", "RenderCommand.mm:81", "Entering LOGW mismatch path",
				"{\"materialUniformBytes\":" + std::to_string(material_.uniformsDataSize_) +
				",\"expectedUniformBytes\":" + std::to_string(expectedUniformBytes) + "}");
			// #endregion
			LOGW("Metal uniforms size mismatch");
			// #region agent log
			DebugLog("H3", "RenderCommand.mm:87", "LOGW mismatch call returned",
				"{\"materialUniformBytes\":" + std::to_string(material_.uniformsDataSize_) +
				",\"expectedUniformBytes\":" + std::to_string(expectedUniformBytes) + "}");
			// #endregion
		}

		// Set pipeline state
		id<MTLRenderPipelineState> pipelineState = (__bridge id<MTLRenderPipelineState>)program->GetPipelineState(
			material_.IsBlendingEnabled(), material_.GetSrcBlendingFactor(), material_.GetDestBlendingFactor());
		if (pipelineState != nil) {
			[encoder setRenderPipelineState:pipelineState];
		}

		// Set depth stencil state
		id<MTLDepthStencilState> depthStencilState = (__bridge id<MTLDepthStencilState>)Backends::MetalRenderState::getDepthStencilState();
		if (depthStencilState != nil) {
			[encoder setDepthStencilState:depthStencilState];
		}

		// Set viewport and scissor
		Backend::ViewportState viewportState = Backend::GetViewportState();
		MTLViewport mtlViewport = {
			static_cast<double>(viewportState.x),
			static_cast<double>(viewportState.y),
			static_cast<double>(viewportState.w),
			static_cast<double>(viewportState.h),
			0.0, 1.0
		};
		[encoder setViewport:mtlViewport];

		if (scissorRect_.W > 0 && scissorRect_.H > 0) {
			MTLScissorRect mtlScissor = {
				static_cast<NSUInteger>(scissorRect_.X),
				static_cast<NSUInteger>(scissorRect_.Y),
				static_cast<NSUInteger>(scissorRect_.W),
				static_cast<NSUInteger>(scissorRect_.H)
			};
			[encoder setScissorRect:mtlScissor];
		} else {
			MTLScissorRect mtlScissor = {
				static_cast<NSUInteger>(viewportState.x),
				static_cast<NSUInteger>(viewportState.y),
				static_cast<NSUInteger>(viewportState.w),
				static_cast<NSUInteger>(viewportState.h)
			};
			[encoder setScissorRect:mtlScissor];
		}

		// Bind textures
		for (std::uint32_t i = 0; i < BackendTexture::MaxTextureUnits; i++) {
			const BackendTexture* texture = material_.GetTexture(i);
			if (texture != nullptr) {
				[encoder setFragmentTexture:(__bridge id<MTLTexture>)texture->GetMetalHandle() atIndex:i];
				[encoder setFragmentSamplerState:(__bridge id<MTLSamplerState>)texture->GetSamplerHandle() atIndex:i];
			}
		}

		// Resource binding layout (shared by default Metal shaders):
		// - vertex buffer: index 0
		// - camera constants (uProjectionMatrix/uViewMatrix): index 1
		// - instance constants (InstanceBlock/InstancesBlock): index 2

		// Bind vertex buffer (if any). Sprite shaders can use `vertex_id` and not require a buffer.
		const BackendBufferObject* vbo = geometry_.GetVboParams().object;
		if (vbo != nullptr) {
			[encoder setVertexBuffer:(__bridge id<MTLBuffer>)vbo->GetMetalHandle() offset:geometry_.GetVboParams().offset atIndex:0];
		}

		// Camera uniforms (2 mat4)
		[encoder setVertexBytes:RenderResources::GetCameraUniformsBuffer() length:128 atIndex:1];
		[encoder setFragmentBytes:RenderResources::GetCameraUniformsBuffer() length:128 atIndex:1];

		// Instance uniforms (InstanceBlock / InstancesBlock data pointer from Material)
		if (material_.uniformsDataPointer_ != nullptr && material_.uniformsDataSize_ > 0) {
			if (material_.uniformsDataSize_ <= 4096) {
				[encoder setVertexBytes:material_.uniformsDataPointer_ length:material_.uniformsDataSize_ atIndex:2];
				[encoder setFragmentBytes:material_.uniformsDataPointer_ length:material_.uniformsDataSize_ atIndex:2];
			} else {
				std::uint32_t offset = 0;
				id<MTLBuffer> buffer = (__bridge id<MTLBuffer>)Backends::MetalRenderState::acquireTransientBuffer(material_.uniformsDataSize_, offset);
				if (buffer != nil) {
					std::memcpy((std::uint8_t*)[buffer contents] + offset, material_.uniformsDataPointer_, material_.uniformsDataSize_);
					[encoder setVertexBuffer:buffer offset:offset atIndex:2];
					[encoder setFragmentBuffer:buffer offset:offset atIndex:2];
				}
			}
		}

		// Draw
		MTLPrimitiveType primitiveType = MTLPrimitiveTypeTriangle;
		switch (geometry_.primitiveType_) {
			case PrimitiveType::Points: primitiveType = MTLPrimitiveTypePoint; break;
			case PrimitiveType::Lines: primitiveType = MTLPrimitiveTypeLine; break;
			case PrimitiveType::LineStrip: primitiveType = MTLPrimitiveTypeLineStrip; break;
			case PrimitiveType::Triangles: primitiveType = MTLPrimitiveTypeTriangle; break;
			case PrimitiveType::TriangleStrip: primitiveType = MTLPrimitiveTypeTriangleStrip; break;
			default: break;
		}

		if (geometry_.numIndices_ > 0) {
			const BackendBufferObject* ibo = geometry_.GetIboParams().object;
			if (ibo != nullptr) {
				[encoder drawIndexedPrimitives:primitiveType
									indexCount:geometry_.numIndices_
									 indexType:MTLIndexTypeUInt16
								   indexBuffer:(__bridge id<MTLBuffer>)ibo->GetMetalHandle()
							 indexBufferOffset:geometry_.GetIboParams().offset + geometry_.firstIndex_ * sizeof(std::uint16_t)
								 instanceCount:numInstances_ > 0 ? numInstances_ : 1];
			}
		} else {
			[encoder drawPrimitives:primitiveType
						vertexStart:geometry_.firstVertex_
						vertexCount:geometry_.numVertices_
					  instanceCount:numInstances_ > 0 ? numInstances_ : 1];
		}

		return;
#endif

		if (geometry_.numVertices_ == 0 && geometry_.numIndices_ == 0) {
			return;
		}

		material_.Bind();
		material_.CommitUniforms();

		Backend::ScissorState scissorTestState = Backend::GetScissorState();
		if (scissorRect_.W > 0 && scissorRect_.H > 0) {
			Backend::EnableScissor(scissorRect_.X, scissorRect_.Y, scissorRect_.W, scissorRect_.H);
		}

		std::uint32_t offset = 0;
#if (defined(WITH_OPENGLES) && !GL_ES_VERSION_3_2) || defined(DEATH_TARGET_EMSCRIPTEN)
		// Simulating missing `glDrawElementsBaseVertex()` on OpenGL ES 3.0
		if (geometry_.numIndices_ > 0) {
			offset = geometry_.GetVboParams().offset + (geometry_.firstVertex_ * geometry_.numElementsPerVertex_ * sizeof(GLfloat));
		}
#endif
		material_.DefineVertexFormat(geometry_.GetVboParams().object, geometry_.GetIboParams().object, offset);
		geometry_.Bind();
		geometry_.Draw(numInstances_);

		Backend::SetScissorState(scissorTestState);
	}

	void RenderCommand::SetScissor(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
	{
		scissorRect_.Set(x, y, width, height);
	}

	void RenderCommand::SetTransformation(const Matrix4x4f& modelMatrix)
	{
		modelMatrix_ = modelMatrix;
		transformationCommitted_ = false;
	}

	void RenderCommand::CommitNodeTransformation()
	{
		if (transformationCommitted_) {
			return;
		}

		ZoneScopedC(0x81A861);

		const Camera::ProjectionValues cameraValues = RenderResources::GetCurrentCamera()->GetProjectionValues();
		modelMatrix_[3][2] = CalculateDepth(layer_, cameraValues.nearClip, cameraValues.farClip);

		if (material_.shaderProgram_ && material_.shaderProgram_->IsLinked()) {
			BackendUniformBlockCache* instanceBlock = material_.UniformBlock(Material::InstanceBlockName);
			BackendUniformCache* matrixUniform = instanceBlock
				? instanceBlock->GetUniform(Material::ModelMatrixUniformName)
				: material_.Uniform(Material::ModelMatrixUniformName);
			if (matrixUniform) {
				//ZoneScopedNC("Set model matrix", 0x81A861);
				matrixUniform->SetFloatVector(modelMatrix_.Data());
			}
		}

		transformationCommitted_ = true;
	}

	void RenderCommand::CommitCameraTransformation()
	{
		ZoneScopedC(0x81A861);

		RenderResources::CameraUniformData* cameraUniformData = RenderResources::FindCameraUniformData(material_.shaderProgram_);
		if (cameraUniformData == nullptr) {
			RenderResources::CameraUniformData newCameraUniformData;
			newCameraUniformData.shaderUniforms.SetProgram(material_.shaderProgram_, Material::ProjectionViewMatrixExcludeString, nullptr);
			if (newCameraUniformData.shaderUniforms.GetUniformCount() == 2) {
				newCameraUniformData.shaderUniforms.SetUniformsDataPointer(RenderResources::GetCameraUniformsBuffer());
				newCameraUniformData.shaderUniforms.GetUniform(Material::ProjectionMatrixUniformName)->SetDirty(true);
				newCameraUniformData.shaderUniforms.GetUniform(Material::ViewMatrixUniformName)->SetDirty(true);
				newCameraUniformData.shaderUniforms.CommitUniforms();

				RenderResources::InsertCameraUniformData(material_.shaderProgram_, std::move(newCameraUniformData));
			}
		} else {
			cameraUniformData->shaderUniforms.CommitUniforms();
		}
	}

	void RenderCommand::CommitAll()
	{
		// Copy the vertices and indices stored in host memory to video memory
		// This step is not needed if the command uses a custom VBO or IBO or directly writes into the common one
		geometry_.CommitVertices();
		geometry_.CommitIndices();

		// The model matrix should always be updated before committing uniform blocks
		CommitNodeTransformation();

		// Commits all the uniform blocks of command's shader program
		material_.CommitUniformBlocks();
	}

	float RenderCommand::CalculateDepth(std::uint16_t layer, float nearClip, float farClip)
	{
		// The layer translates to depth, from near to far
		return nearClip + LayerStep + (farClip - nearClip - LayerStep) * layer * LayerStep;
	}
}
