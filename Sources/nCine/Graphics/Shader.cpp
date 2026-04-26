#define NCINE_INCLUDE_OPENGL
#include "../CommonHeaders.h"

#include "Shader.h"
#if !defined(DEATH_TARGET_IOS)
#	include "GL/GLShaderProgram.h"
#endif
#include "RenderResources.h"
#include "BinaryShaderCache.h"
#include "../Application.h"
#include "../tracy.h"
#include "../../Main.h"

#if defined(WITH_EMBEDDED_SHADERS)
#	include "shader_strings.h"
#else
#	include <IO/FileSystem.h>
using namespace Death::IO;
#endif

using namespace Death::Containers::Literals;

namespace nCine
{
	namespace
	{
		static const char BatchSizeDefine[] = "BATCH_SIZE";
		static const char DefineFormatString[] = "#define {} ({})\n";
		static const char ResetLineString[] = "#line 0\n";
		static const std::int32_t MaxShaderStrings = 8;

		BackendShaderProgram::Introspection shaderToShaderProgramIntrospection(Shader::Introspection introspection)
		{
			switch (introspection) {
				default:
				case Shader::Introspection::Enabled:
					return BackendShaderProgram::Introspection::Enabled;
				case Shader::Introspection::NoUniformsInBlocks:
					return BackendShaderProgram::Introspection::NoUniformsInBlocks;
				case Shader::Introspection::Disabled:
					return BackendShaderProgram::Introspection::Disabled;
			}
		}

		bool isBatchedVertex(Shader::DefaultVertex vertex)
		{
			switch (vertex) {
				case Shader::DefaultVertex::BATCHED_SPRITES:
				case Shader::DefaultVertex::BATCHED_SPRITES_NOTEXTURE:
				case Shader::DefaultVertex::BATCHED_MESHSPRITES:
				case Shader::DefaultVertex::BATCHED_MESHSPRITES_NOTEXTURE:
				//case Shader::DefaultVertex::BATCHED_TEXTNODES:
					return true;
				default:
					return false;
			}
		}

		std::size_t populateShaderStrings(ArrayView<StringView> strings, ArrayView<char> backingStore, const char* content, std::int32_t batchSize, ArrayView<const StringView> defines)
		{
			std::size_t lastOffset = 0, lastIndex = 0;
			if (batchSize > 0 || !strings.empty()) {
				if (batchSize > 0) {
					std::size_t length = formatInto({ &backingStore[lastOffset], backingStore.size() }, DefineFormatString, BatchSizeDefine, batchSize);
					strings[lastIndex++] = { &backingStore[lastOffset], length };
					lastOffset += length + 1;
				}
				for (auto define : defines) {
					std::size_t charsLeft = backingStore.size() - lastOffset;
					if (lastIndex >= arraySize(strings) - 3 && arraySize(DefineFormatString) + define.size() >= charsLeft) {
						break;
					}
					std::size_t length = formatInto({ &backingStore[lastOffset], charsLeft }, DefineFormatString, define, 1);
					strings[lastIndex++] = { &backingStore[lastOffset], length };
					lastOffset += length + 1;
				}
				strings[lastIndex++] = ResetLineString;
			}
			if (content != nullptr) {
				strings[lastIndex++] = content;
			}
			return lastIndex;
		}
	}

	Shader::Shader()
		: Object(ObjectType::Shader), backendShaderProgram_(std::make_unique<BackendShaderProgram>(BackendShaderProgram::QueryPhase::Immediate))
	{
	}

	Shader::Shader(const char* shaderName, LoadMode loadMode, Introspection introspection, const char* vertex, const char* fragment, std::int32_t batchSize)
		: Shader()
	{
		const bool hasLoaded = loadMode == LoadMode::String
			? LoadFromMemory(shaderName, introspection, vertex, fragment, batchSize)
			: LoadFromFile(shaderName, introspection, vertex, fragment, batchSize);

		if (!hasLoaded) {
			LOGE("Shader \"{}\" cannot be loaded", shaderName);
		}
	}

	Shader::Shader(const char* shaderName, LoadMode loadMode, const char* vertex, const char* fragment, std::int32_t batchSize)
		: Shader()
	{
		const bool hasLoaded = loadMode == LoadMode::String
			? LoadFromMemory(shaderName, vertex, fragment, batchSize)
			: LoadFromFile(shaderName, vertex, fragment, batchSize);

		if (!hasLoaded) {
			LOGE("Shader \"{}\" cannot be loaded", shaderName);
		}
	}

	Shader::Shader(LoadMode loadMode, const char* vertex, const char* fragment, std::int32_t batchSize)
		: Shader(nullptr, loadMode, vertex, fragment, batchSize)
	{
	}

	Shader::Shader(const char* shaderName, LoadMode loadMode, Introspection introspection, DefaultVertex vertex, const char* fragment, std::int32_t batchSize)
		: Shader()
	{
		const bool hasLoaded = loadMode == LoadMode::String
			? LoadFromMemory(shaderName, introspection, vertex, fragment, batchSize)
			: LoadFromFile(shaderName, introspection, vertex, fragment, batchSize);

		if (!hasLoaded) {
			LOGE("Shader \"{}\" cannot be loaded", shaderName);
		}
	}

	Shader::Shader(const char* shaderName, LoadMode loadMode, DefaultVertex vertex, const char* fragment, std::int32_t batchSize)
		: Shader()
	{
		const bool hasLoaded = loadMode == LoadMode::String
			? LoadFromMemory(shaderName, vertex, fragment, batchSize)
			: LoadFromFile(shaderName, vertex, fragment, batchSize);

		if (!hasLoaded) {
			LOGE("Shader \"{}\" cannot be loaded", shaderName);
		}
	}

	Shader::Shader(LoadMode loadMode, DefaultVertex vertex, const char* fragment, int batchSize)
		: Shader(nullptr, loadMode, vertex, fragment, batchSize)
	{
	}

	Shader::Shader(const char* shaderName, LoadMode loadMode, Introspection introspection, const char* vertex, DefaultFragment fragment, std::int32_t batchSize)
		: Shader()
	{
		const bool hasLoaded = loadMode == LoadMode::String
			? LoadFromMemory(shaderName, introspection, vertex, fragment, batchSize)
			: LoadFromFile(shaderName, introspection, vertex, fragment, batchSize);

		if (!hasLoaded) {
			LOGE("Shader \"{}\" cannot be loaded", shaderName);
		}
	}

	Shader::Shader(const char* shaderName, LoadMode loadMode, const char* vertex, DefaultFragment fragment, std::int32_t batchSize)
		: Shader()
	{
		const bool hasLoaded = loadMode == LoadMode::String
			? LoadFromMemory(shaderName, vertex, fragment, batchSize)
			: LoadFromFile(shaderName, vertex, fragment, batchSize);

		if (!hasLoaded) {
			LOGE("Shader \"{}\" cannot be loaded", shaderName);
		}
	}

	Shader::Shader(LoadMode loadMode, const char* vertex, DefaultFragment fragment, std::int32_t batchSize)
		: Shader(nullptr, loadMode, vertex, fragment, batchSize)
	{
	}

	Shader::~Shader()
	{
		RenderResources::UnregisterBatchedShader(backendShaderProgram_.get());
	}

	bool Shader::LoadFromMemory(const char* shaderName, Introspection introspection, const char* vertex, const char* fragment, std::int32_t batchSize, ArrayView<const StringView> defines)
	{
		ZoneScopedC(0x81A861);
		if (shaderName != nullptr) {
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(shaderName, std::strlen(shaderName));
		}

		backendShaderProgram_->Reset(); // reset before attaching new shaders
		backendShaderProgram_->SetBatchSize(batchSize);
		backendShaderProgram_->SetObjectLabel(shaderName);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];

		stringsCount = populateShaderStrings(strings, backingStore, vertex, batchSize, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(strings, stringsCount), {});

		stringsCount = populateShaderStrings(strings, backingStore, fragment, -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_FRAGMENT_SHADER, arrayView(strings, stringsCount), {});

		backendShaderProgram_->Link(shaderToShaderProgramIntrospection(introspection));

		return backendShaderProgram_->IsLinked();
	}

	bool Shader::LoadFromMemory(const char* shaderName, const char* vertex, const char* fragment, std::int32_t batchSize)
	{
		return LoadFromMemory(shaderName, Introspection::Enabled, vertex, fragment, batchSize);
	}

	bool Shader::LoadFromMemory(const char* vertex, const char* fragment, std::int32_t batchSize)
	{
		return LoadFromMemory(nullptr, Introspection::Enabled, vertex, fragment, batchSize);
	}

	bool Shader::LoadFromMemory(const char* shaderName, Introspection introspection, DefaultVertex vertex, const char* fragment, std::int32_t batchSize, ArrayView<const StringView> defines)
	{
		ZoneScopedC(0x81A861);
		if (shaderName != nullptr) {
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(shaderName, std::strlen(shaderName));
		}

		backendShaderProgram_->Reset(); // reset before attaching new shaders
		backendShaderProgram_->SetBatchSize(batchSize);
		backendShaderProgram_->SetObjectLabel(shaderName);

		const bool isBatched = isBatchedVertex(vertex);
		const char* vertexSource = RenderResources::GetDefaultVertexShaderSource(vertex);
		FATAL_ASSERT(vertexSource);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];

		stringsCount = populateShaderStrings(strings, backingStore, vertexSource, isBatched ? batchSize : -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(strings, stringsCount), {});

		stringsCount = populateShaderStrings(strings, backingStore, fragment, -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_FRAGMENT_SHADER, arrayView(strings, stringsCount), {});

		backendShaderProgram_->Link(shaderToShaderProgramIntrospection(introspection));

		return backendShaderProgram_->IsLinked();
	}

	bool Shader::LoadFromMemory(const char* shaderName, DefaultVertex vertex, const char* fragment, std::int32_t batchSize)
	{
		const Introspection introspection = (isBatchedVertex(vertex) ? Introspection::NoUniformsInBlocks : Introspection::Enabled);
		return LoadFromMemory(shaderName, introspection, vertex, fragment, batchSize);
	}

	bool Shader::LoadFromMemory(DefaultVertex vertex, const char* fragment, std::int32_t batchSize)
	{
		return LoadFromMemory(nullptr, vertex, fragment, batchSize);
	}

	bool Shader::LoadFromMemory(const char* shaderName, Introspection introspection, const char* vertex, DefaultFragment fragment, std::int32_t batchSize, ArrayView<const StringView> defines)
	{
		ZoneScopedC(0x81A861);
		if (shaderName != nullptr) {
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(shaderName, std::strlen(shaderName));
		}

		backendShaderProgram_->Reset(); // reset before attaching new shaders
		backendShaderProgram_->SetBatchSize(batchSize);
		backendShaderProgram_->SetObjectLabel(shaderName);

		const char* fragmentSource = RenderResources::GetDefaultFragmentShaderSource(fragment);
		FATAL_ASSERT(fragmentSource);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];

		stringsCount = populateShaderStrings(strings, backingStore, vertex, batchSize, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(strings, stringsCount), {});

		stringsCount = populateShaderStrings(strings, backingStore, fragmentSource, -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_FRAGMENT_SHADER, arrayView(strings, stringsCount), {});

		backendShaderProgram_->Link(shaderToShaderProgramIntrospection(introspection));

		return backendShaderProgram_->IsLinked();
	}

	bool Shader::LoadFromMemory(const char* shaderName, const char* vertex, DefaultFragment fragment, std::int32_t batchSize)
	{
		return LoadFromMemory(shaderName, Introspection::Enabled, vertex, fragment, batchSize);
	}

	bool Shader::LoadFromMemory(const char* vertex, DefaultFragment fragment, std::int32_t batchSize)
	{
		return LoadFromMemory(nullptr, vertex, fragment, batchSize);
	}

	bool Shader::LoadFromFile(const char* shaderName, Introspection introspection, StringView vertexPath, StringView fragmentPath, std::int32_t batchSize, ArrayView<const StringView> defines)
	{
		ZoneScopedC(0x81A861);
		if (shaderName != nullptr) {
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(shaderName, std::strlen(shaderName));
		}

		backendShaderProgram_->Reset(); // reset before attaching new shaders
		backendShaderProgram_->SetBatchSize(batchSize);
		backendShaderProgram_->SetObjectLabel(shaderName);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];

		stringsCount = populateShaderStrings(strings, backingStore, nullptr, batchSize, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(strings, stringsCount), vertexPath);

		stringsCount = populateShaderStrings(strings, backingStore, nullptr, -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_FRAGMENT_SHADER, arrayView(strings, stringsCount), fragmentPath);

		backendShaderProgram_->Link(shaderToShaderProgramIntrospection(introspection));

		return backendShaderProgram_->IsLinked();
	}

	bool Shader::LoadFromFile(const char* shaderName, StringView vertexPath, StringView fragmentPath, std::int32_t batchSize)
	{
		return LoadFromFile(shaderName, Introspection::Enabled, vertexPath, fragmentPath, batchSize);
	}

	bool Shader::LoadFromFile(StringView vertexPath, StringView fragmentPath, std::int32_t batchSize)
	{
		return LoadFromFile(nullptr, vertexPath, fragmentPath, batchSize);
	}

	bool Shader::LoadFromFile(const char* shaderName, Introspection introspection, DefaultVertex vertex, StringView fragmentPath, std::int32_t batchSize, ArrayView<const StringView> defines)
	{
		ZoneScopedC(0x81A861);
		if (shaderName != nullptr) {
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(shaderName, std::strlen(shaderName));
		}

		backendShaderProgram_->Reset(); // reset before attaching new shaders
		backendShaderProgram_->SetBatchSize(batchSize);
		backendShaderProgram_->SetObjectLabel(shaderName);

		const bool isBatched = isBatchedVertex(vertex);
		const char* vertexSource = RenderResources::GetDefaultVertexShaderSource(vertex);
		FATAL_ASSERT(vertexSource);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];

		stringsCount = populateShaderStrings(strings, backingStore, vertexSource, isBatched ? batchSize : -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(strings, stringsCount), {});

		stringsCount = populateShaderStrings(strings, backingStore, nullptr, -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_FRAGMENT_SHADER, arrayView(strings, stringsCount), fragmentPath);

		backendShaderProgram_->Link(shaderToShaderProgramIntrospection(introspection));

		return backendShaderProgram_->IsLinked();
	}

	bool Shader::LoadFromFile(const char* shaderName, DefaultVertex vertex, StringView fragmentPath, std::int32_t batchSize)
	{
		const Introspection introspection = (isBatchedVertex(vertex) ? Introspection::NoUniformsInBlocks : Introspection::Enabled);
		return LoadFromFile(shaderName, introspection, vertex, fragmentPath, batchSize);
	}

	bool Shader::LoadFromFile(DefaultVertex vertex, StringView fragmentPath, std::int32_t batchSize)
	{
		return LoadFromFile(nullptr, vertex, fragmentPath, batchSize);
	}

	bool Shader::LoadFromFile(const char* shaderName, Introspection introspection, StringView vertexPath, DefaultFragment fragment, std::int32_t batchSize, ArrayView<const StringView> defines)
	{
		ZoneScopedC(0x81A861);
		if (shaderName != nullptr) {
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(shaderName, std::strlen(shaderName));
		}

		backendShaderProgram_->Reset(); // reset before attaching new shaders
		backendShaderProgram_->SetBatchSize(batchSize);
		backendShaderProgram_->SetObjectLabel(shaderName);

		const char* fragmentSource = RenderResources::GetDefaultFragmentShaderSource(fragment);
		FATAL_ASSERT(fragmentSource);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];

		stringsCount = populateShaderStrings(strings, backingStore, nullptr, batchSize, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(strings, stringsCount), vertexPath);

		stringsCount = populateShaderStrings(strings, backingStore, fragmentSource, -1, defines);
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_FRAGMENT_SHADER, arrayView(strings, stringsCount), {});

		backendShaderProgram_->Link(shaderToShaderProgramIntrospection(introspection));

		return backendShaderProgram_->IsLinked();
	}

	bool Shader::LoadFromFile(const char* shaderName, StringView vertexPath, DefaultFragment fragment, std::int32_t batchSize)
	{
		return LoadFromFile(shaderName, Introspection::Enabled, vertexPath, fragment, batchSize);
	}

	bool Shader::LoadFromFile(StringView vertexPath, DefaultFragment fragment, std::int32_t batchSize)
	{
		return LoadFromFile(nullptr, vertexPath, fragment, batchSize);
	}

	bool Shader::LoadFromCache(const char* shaderName, std::uint64_t shaderVersion, Introspection introspection)
	{
#if defined(DEATH_TARGET_IOS)
		(void)shaderName;
		(void)shaderVersion;
		(void)introspection;
		return false;
#else
		ZoneScopedC(0x81A861);
		if (shaderName != nullptr) {
			// When Tracy is disabled the statement body is empty and braces are needed
			ZoneText(shaderName, std::strlen(shaderName));
		}

		const bool hasLoaded = RenderResources::GetBinaryShaderCache().LoadFromCache(shaderName, shaderVersion,
			static_cast<GLShaderProgram*>(backendShaderProgram_.get()), shaderToShaderProgramIntrospection(introspection));
		if (hasLoaded) {
			backendShaderProgram_->Link(shaderToShaderProgramIntrospection(introspection));
		}

		return hasLoaded && backendShaderProgram_->IsLinked();
#endif
	}

	bool Shader::SaveToCache(const char* shaderName, std::uint64_t shaderVersion) const
	{
#if defined(DEATH_TARGET_IOS)
		(void)shaderName;
		(void)shaderVersion;
		return false;
#else
		return RenderResources::GetBinaryShaderCache().SaveToCache(shaderName, shaderVersion, static_cast<GLShaderProgram*>(backendShaderProgram_.get()));
#endif
	}

	bool Shader::SetAttribute(const char* name, std::int32_t stride, void* pointer)
	{
		return backendShaderProgram_->DefineAttribute(name, stride, pointer);
	}

	bool Shader::IsLinked() const
	{
		return backendShaderProgram_->IsLinked();
	}

	unsigned int Shader::RetrieveInfoLogLength() const
	{
		return backendShaderProgram_->RetrieveInfoLogLength();
	}

	void Shader::RetrieveInfoLog(std::string& infoLog) const
	{
		backendShaderProgram_->RetrieveInfoLog(infoLog);
	}

	bool Shader::GetLogOnErrors() const
	{
		return backendShaderProgram_->GetLogOnErrors();
	}

	void Shader::SetLogOnErrors(bool shouldLogOnErrors)
	{
		backendShaderProgram_->SetLogOnErrors(shouldLogOnErrors);
	}

	void Shader::SetBackendShaderProgramLabel(const char* label)
	{
		backendShaderProgram_->SetObjectLabel(label);
	}

	void Shader::RegisterBatchedShader(Shader& batchedShader)
	{
		RenderResources::RegisterBatchedShader(backendShaderProgram_.get(), batchedShader.backendShaderProgram_.get());
	}

	bool Shader::LoadDefaultShader(DefaultVertex vertex, int batchSize)
	{
		const bool isBatched = isBatchedVertex(vertex);
		const char* vertexSource = RenderResources::GetDefaultVertexShaderSource(vertex);
		FATAL_ASSERT(vertexSource);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];
		stringsCount = populateShaderStrings(strings, backingStore, vertexSource, isBatched ? batchSize : -1, {});
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_VERTEX_SHADER, arrayView(strings, stringsCount), {});

		return true;
	}

	bool Shader::LoadDefaultShader(DefaultFragment fragment)
	{
		const char* fragmentSource = RenderResources::GetDefaultFragmentShaderSource(fragment);
		FATAL_ASSERT(fragmentSource);

		StringView strings[MaxShaderStrings]; std::size_t stringsCount; char backingStore[256];
		stringsCount = populateShaderStrings(strings, backingStore, fragmentSource, -1, {});
		backendShaderProgram_->AttachShaderFromStringsAndFile(GL_FRAGMENT_SHADER, arrayView(strings, stringsCount), {});

		return true;
	}
}