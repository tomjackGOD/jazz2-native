#include "MetalShaderProgram.h"
#include "MetalGfxDevice.h"
#include "../../Graphics/Material.h"
#include "../../Graphics/RenderResources.h"
#include "../../tracy.h"

#import <Metal/Metal.h>

namespace nCine
{
	namespace
	{
		MTLBlendFactor GetMetalBlendFactor(BlendingFactor factor)
		{
			switch (factor) {
				case BlendingFactor::Zero: return MTLBlendFactorZero;
				case BlendingFactor::One: return MTLBlendFactorOne;
				case BlendingFactor::SrcColor: return MTLBlendFactorSourceColor;
				case BlendingFactor::OneMinusSrcColor: return MTLBlendFactorOneMinusSourceColor;
				case BlendingFactor::DstColor: return MTLBlendFactorDestinationColor;
				case BlendingFactor::OneMinusDstColor: return MTLBlendFactorOneMinusDestinationColor;
				case BlendingFactor::SrcAlpha: return MTLBlendFactorSourceAlpha;
				case BlendingFactor::OneMinusSrcAlpha: return MTLBlendFactorOneMinusSourceAlpha;
				case BlendingFactor::DstAlpha: return MTLBlendFactorDestinationAlpha;
				case BlendingFactor::OneMinusDstAlpha: return MTLBlendFactorOneMinusDestinationAlpha;
				// Metal on iOS does not expose OpenGL constant blend-color factors.
				case BlendingFactor::ConstantColor: return MTLBlendFactorOne;
				case BlendingFactor::OneMinusConstantColor: return MTLBlendFactorZero;
				case BlendingFactor::ConstantAlpha: return MTLBlendFactorOne;
				case BlendingFactor::OneMinusConstantAlpha: return MTLBlendFactorZero;
				case BlendingFactor::SrcAlphaSaturate: return MTLBlendFactorSourceAlphaSaturated;
				default: return MTLBlendFactorOne;
			}
		}
	}

	void MetalShaderUniformBlocks::SetProgram(MetalShaderProgram* shaderProgram)
	{
		shaderProgram_ = shaderProgram;
		uniformBlocks_.clear();

		// Add standard engine uniform blocks
		// For sprites, we expect InstanceBlock or InstancesBlock (for batching)
		
		MetalUniformBlockCache instanceBlock;
		instanceBlock.SetName(Material::InstanceBlockName);
		instanceBlock.AddUniform(Material::ModelMatrixUniformName, 0);
		uniformBlocks_[Material::InstanceBlockName] = instanceBlock;

		MetalUniformBlockCache instancesBlock;
		instancesBlock.SetName(Material::InstancesBlockName);
		uniformBlocks_[Material::InstancesBlockName] = instancesBlock;

		// Set sizes for Material to allocate host buffer
		// Standard sprite InstanceBlock is 112 bytes (std140 padded)
		// struct Instance { mat4 modelMatrix; vec4 color; vec4 texRect; vec2 spriteSize; };
		// 64 + 16 + 16 + 8 = 104, padded to 112
		const std::uint32_t instanceSize = 112;

		shaderProgram_->SetUniformsSize(0);
		if (shaderProgram_->GetBatchSize() > 0) {
			shaderProgram_->SetUniformBlocksSize(shaderProgram_->GetBatchSize() * instanceSize);
		} else {
			shaderProgram_->SetUniformBlocksSize(instanceSize);
		}
	}

	void MetalShaderUniformBlocks::SetUniformsDataPointer(std::uint8_t* dataPointer)
	{
		// Map the blocks to the data pointer provided by Material
		auto it = uniformBlocks_.find(Material::InstanceBlockName);
		if (it != uniformBlocks_.end()) {
			it->second.SetDataPointer(dataPointer, 112);
		}
		
		it = uniformBlocks_.find(Material::InstancesBlockName);
		if (it != uniformBlocks_.end()) {
			it->second.SetDataPointer(dataPointer, shaderProgram_->GetUniformBlocksSize());
		}
	}

	void MetalShaderUniforms::SetProgram(MetalShaderProgram* shaderProgram, const char* includeOnly, const char* exclude)
	{
		shaderProgram_ = shaderProgram;
		uniformCaches_.clear();

		// For now, we only support a few common uniforms that aren't in blocks
		// (e.g. uGuiProjection, uDepth for ImGui)
		
		// Note: uTexture is usually handled separately via textures_[0]
		
		// We'll hardcode ImGui uniforms for now if the program seems to be ImGui
		// In a real implementation, we would use introspection on the MSL source
		
		// ImGui uniforms
		const std::uint32_t guiProjectionOffset = 0;
		const std::uint32_t depthOffset = 64;
		const std::uint32_t imguiUniformsSize = 68;

		// Check if it's ImGui (heuristic)
		// We don't have the shader name easily here, but we can check the exclude list
		// or just always add them if they don't clash.
		
		MetalUniformCache guiProjCache;
		uniformCaches_[Material::GuiProjectionMatrixUniformName] = guiProjCache;
		
		MetalUniformCache depthCache;
		uniformCaches_[Material::DepthUniformName] = depthCache;

		shaderProgram_->SetUniformsSize(imguiUniformsSize);
	}

	void MetalShaderUniforms::SetUniformsDataPointer(std::uint8_t* dataPointer)
	{
		auto it = uniformCaches_.find(Material::GuiProjectionMatrixUniformName);
		if (it != uniformCaches_.end()) {
			it->second.SetDataPointer(dataPointer + 0);
		}
		
		it = uniformCaches_.find(Material::DepthUniformName);
		if (it != uniformCaches_.end()) {
			it->second.SetDataPointer(dataPointer + 64);
		}
	}

	void MetalShaderUniforms::SetDirty(bool isDirty)
	{
		for (auto& it : uniformCaches_) {
			it.second.SetDirty(isDirty);
		}
	}

	void MetalShaderUniforms::CommitUniforms()
	{
		for (auto& it : uniformCaches_) {
			it.second.CommitValue();
		}
	}

	void MetalShaderUniformBlocks::Bind()
	{
		// Metal uniform binding happens in RenderCommand::Issue
	}

	MetalShaderProgram::MetalShaderProgram(QueryPhase queryPhase)
		: metalHandle_(nullptr), vertexFunction_(nullptr), fragmentFunction_(nullptr), vertexDescriptor_(nullptr),
		  status_(Status::NotLinked), batchSize_(0), uniformsSize_(0), uniformBlocksSize_(0)
	{
	}

	MetalShaderProgram::~MetalShaderProgram()
	{
		Reset();
	}

	bool MetalShaderProgram::AttachShaderFromString(int type, Death::Containers::StringView source)
	{
		return AttachShaderFromStringsAndFile(type, Death::Containers::arrayView(&source, 1), {});
	}

	bool MetalShaderProgram::AttachShaderFromFile(int type, Death::Containers::StringView filename)
	{
		// Metal shaders on iOS are typically precompiled into the default.metallib.
		// However, for development/compatibility, we could load from file.
		// For now, we'll just log an error or return false as we expect embedded MSL.
		LOGE("Loading Metal shader from file is not yet supported: %s", filename.data());
		return false;
	}

	bool MetalShaderProgram::AttachShaderFromStringsAndFile(int type, Death::Containers::ArrayView<const Death::Containers::StringView> strings, Death::Containers::StringView filename)
	{
		// type 0x8B31 is GL_VERTEX_SHADER, 0x8B30 is GL_FRAGMENT_SHADER
		
		id<MTLDevice> device = (__bridge id<MTLDevice>)Backends::MetalGfxDevice::device();
		if (device == nil) {
			return false;
		}

		// Concatenate strings into a single MSL source
		std::string source;
		for (const auto& s : strings) {
			source.append(s.data(), s.size());
		}

		NSError* error = nil;
		id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()] options:nil error:&error];
		if (library == nil) {
			LOGE("Metal shader compilation failed: %s", [[error localizedDescription] UTF8String]);
			status_ = Status::CompilationFailed;
			return false;
		}

		if (type == 0x8B31) { // GL_VERTEX_SHADER
			vertexFunction_ = (__bridge_retained void*)[library newFunctionWithName:@"v_main"];
		} else if (type == 0x8B30) { // GL_FRAGMENT_SHADER
			fragmentFunction_ = (__bridge_retained void*)[library newFunctionWithName:@"f_main"];
		}
		
		if (metalHandle_ != nullptr) {
			id<MTLLibrary> oldLibrary = (__bridge_transfer id<MTLLibrary>)metalHandle_;
			oldLibrary = nil;
		}
		metalHandle_ = (__bridge_retained void*)library;
		
		return true;
	}

	bool MetalShaderProgram::Link(Introspection introspection)
	{
		if (vertexFunction_ == nullptr || fragmentFunction_ == nullptr) {
			status_ = Status::LinkingFailed;
			return false;
		}

		MTLVertexDescriptor* vertexDesc = [MTLVertexDescriptor vertexDescriptor];
		int maxStride = 0;

		for (auto it = attributes_.begin(); it != attributes_.end(); ++it) {
			const AttributeInfo& info = it->second;
			MTLVertexFormat format = MTLVertexFormatFloat2;
			
			if (it->first == Material::MeshIndexAttributeName) {
				format = MTLVertexFormatUInt;
			} else {
				if (info.components == 1) format = MTLVertexFormatFloat;
				else if (info.components == 2) format = MTLVertexFormatFloat2;
				else if (info.components == 3) format = MTLVertexFormatFloat3;
				else if (info.components == 4) format = MTLVertexFormatFloat4;
			}

			vertexDesc.attributes[info.slot].format = format;
			vertexDesc.attributes[info.slot].offset = static_cast<NSUInteger>(info.offset);
			vertexDesc.attributes[info.slot].bufferIndex = 0;

			if (info.stride > maxStride) {
				maxStride = info.stride;
			}
		}

		if (maxStride > 0) {
			vertexDesc.layouts[0].stride = static_cast<NSUInteger>(maxStride);
			vertexDesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
		}

		vertexDescriptor_ = (__bridge_retained void*)vertexDesc;
		status_ = Status::Linked;
		return true;
	}

	void MetalShaderProgram::Use()
	{
		// Metal doesn't have a global "use program" state.
		// Pipeline states are set on a render command encoder.
	}

	void* MetalShaderProgram::GetPipelineState(bool blendingEnabled, BlendingFactor srcFactor, BlendingFactor destFactor)
	{
		PipelineStateKey key = { blendingEnabled, srcFactor, destFactor };
		auto it = pipelineStates_.find(key);
		if (it != pipelineStates_.end()) {
			return it->second;
		}

		id<MTLDevice> device = (__bridge id<MTLDevice>)Backends::MetalGfxDevice::device();
		if (device == nil) {
			return nullptr;
		}

		MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
		pipelineDesc.vertexFunction = (__bridge id<MTLFunction>)vertexFunction_;
		pipelineDesc.fragmentFunction = (__bridge id<MTLFunction>)fragmentFunction_;
		pipelineDesc.vertexDescriptor = (__bridge MTLVertexDescriptor*)vertexDescriptor_;
		pipelineDesc.colorAttachments[0].pixelFormat = (MTLPixelFormat)Backends::MetalGfxDevice::pixelFormat();

		if (blendingEnabled) {
			pipelineDesc.colorAttachments[0].blendingEnabled = YES;
			pipelineDesc.colorAttachments[0].sourceRGBBlendFactor = GetMetalBlendFactor(srcFactor);
			pipelineDesc.colorAttachments[0].destinationRGBBlendFactor = GetMetalBlendFactor(destFactor);
			pipelineDesc.colorAttachments[0].sourceAlphaBlendFactor = GetMetalBlendFactor(srcFactor);
			pipelineDesc.colorAttachments[0].destinationAlphaBlendFactor = GetMetalBlendFactor(destFactor);
		}

		NSError* error = nil;
		id<MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
		if (pipelineState == nil) {
			LOGE("Metal pipeline state creation failed: %s", [[error localizedDescription] UTF8String]);
			return nullptr;
		}

		void* handle = (__bridge_retained void*)pipelineState;
		pipelineStates_[key] = handle;
		return handle;
	}

	void MetalShaderProgram::Reset()
	{
		if (vertexFunction_ != nullptr) {
			id<MTLFunction> func = (__bridge_transfer id<MTLFunction>)vertexFunction_;
			func = nil;
			vertexFunction_ = nullptr;
		}
		if (fragmentFunction_ != nullptr) {
			id<MTLFunction> func = (__bridge_transfer id<MTLFunction>)fragmentFunction_;
			func = nil;
			fragmentFunction_ = nullptr;
		}
		if (vertexDescriptor_ != nullptr) {
			MTLVertexDescriptor* desc = (__bridge_transfer MTLVertexDescriptor*)vertexDescriptor_;
			desc = nil;
			vertexDescriptor_ = nullptr;
		}
		if (metalHandle_ != nullptr) {
			id<MTLLibrary> library = (__bridge_transfer id<MTLLibrary>)metalHandle_;
			library = nil;
			metalHandle_ = nullptr;
		}
		status_ = Status::NotLinked;
	}

	bool MetalShaderProgram::DefineAttribute(const char* name, std::int32_t stride, void* pointer)
	{
		// Metal uses fixed attribute slots defined in MSL.
		// We'll map standard names to slots.
		int slot = -1;
		int components = 2;
		if (strcmp(name, "aPosition") == 0) { slot = 0; components = 2; }
		else if (strcmp(name, "aTexCoords") == 0) { slot = 1; components = 2; }
		else if (strcmp(name, "aColor") == 0) { slot = 2; components = 4; }
		else if (strcmp(name, "aMeshIndex") == 0) { slot = 3; components = 1; }
		
		if (slot != -1) {
			attributes_[name] = { slot, static_cast<int>(reinterpret_cast<std::uintptr_t>(pointer)), stride, components };
			return true;
		}
		return false;
	}

	void MetalShaderProgram::DefineDefaultAttributes(const char* posName, const char* texName, const char* indexName)
	{
		// Standard sprite vertex format:
		// struct VertexFormatPos2Tex2 { float position[2]; float texcoords[2]; };
		// struct VertexFormatPos2Tex2Index { float position[2]; float texcoords[2]; int drawindex; };
		
		DefineAttribute(posName, sizeof(RenderResources::VertexFormatPos2Tex2), reinterpret_cast<void*>(offsetof(RenderResources::VertexFormatPos2Tex2, position)));
		DefineAttribute(texName, sizeof(RenderResources::VertexFormatPos2Tex2), reinterpret_cast<void*>(offsetof(RenderResources::VertexFormatPos2Tex2, texcoords)));
		
		if (indexName != nullptr) {
			DefineAttribute(indexName, sizeof(RenderResources::VertexFormatPos2Tex2Index), reinterpret_cast<void*>(offsetof(RenderResources::VertexFormatPos2Tex2Index, drawindex)));
		}
	}

	void MetalShaderProgram::DefineVertexFormat(const MetalBufferObject* vbo, const MetalBufferObject* ibo, std::uint32_t vboOffset)
	{
		// In Metal, vertex format is part of the Pipeline State Object.
	}

	void MetalShaderProgram::SetObjectLabel(Death::Containers::StringView label)
	{
		label_ = label;
		if (metalHandle_ != nullptr) {
			id<MTLLibrary> library = (__bridge id<MTLLibrary>)metalHandle_;
			library.label = [NSString stringWithUTF8String:label.data()];
		}
	}
}
