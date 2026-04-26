# Metal Backend Audit (OpenGL Render Stack)

This document captures the current render call chain and the exact places the engine is coupled to OpenGL (direct `gl*` calls and OpenGL-specific helper classes). It is intended to be used as the source-of-truth when replacing the render stack with a native Metal backend.

## 1. Engine render call chain (scenegraph path)

On iOS/Metal we want the same high-level flow to exist, but with all GPU work done via Metal instead of OpenGL.

### 1.1 Initialization

When `withGraphics` and `withScenegraph` are enabled, initialization happens in:

- `[Sources/nCine/Application.cpp](Sources/nCine/Application.cpp)`:
  - Registers `GfxCapabilities` (currently OpenGL-based): `theServiceLocator().RegisterGfxCapabilities(std::make_unique<GfxCapabilities>())`.
  - Initializes GL state via `gfxDevice_->setupGL()` (calls OpenGL functions; see “OpenGL coupling points”).
  - Creates render resources: `RenderResources::Create()` (allocates GL shader/program related objects).
  - Creates `SceneNode` and `ScreenViewport`.

### 1.2 Per-frame update + draw

Per frame, when `Application::Step()` runs with scenegraph:

- `[Sources/nCine/Application.cpp](Sources/nCine/Application.cpp)`:
  - `screenViewport_->Update();`
  - `screenViewport_->Visit();`
  - `screenViewport_->SortAndCommitQueue();`
  - `screenViewport_->Draw();`

The draw call chain is:

- `[Sources/nCine/Graphics/ScreenViewport.cpp](Sources/nCine/Graphics/ScreenViewport.cpp)`:
  - `Viewport::Draw(0);`
  - clears queues / remaps buffers after drawing.
- `[Sources/nCine/Graphics/Viewport.cpp](Sources/nCine/Graphics/Viewport.cpp)`:
  - `Viewport::Draw(nextIndex)` recursion across viewport chains.
  - Clears the target via `glClear(...)` (OpenGL path).
  - Sets viewport/scissor via `GLViewport` / `GLScissorTest`.
  - Calls `renderQueue_.Draw();`
  - Uses `GLFramebuffer` for FBO binding and MRT draw buffers.
- `[Sources/nCine/Graphics/RenderQueue.cpp](Sources/nCine/Graphics/RenderQueue.cpp)`:
  - Sorts queues and commits GPU buffer uploads: `RenderQueue::SortAndCommitQueue()`.
  - Calls `RenderCommand::Issue()` for opaque and transparent commands.
  - Sets render state (blending, depth mask/test, scissor enable/disable) via `GLBlending`, `GLDepthTest`, `GLScissorTest`.
- `[Sources/nCine/Graphics/RenderCommand.cpp](Sources/nCine/Graphics/RenderCommand.cpp)`:
  - Early exit if geometry is empty.
  - `material_.Bind();`
  - `material_.CommitUniforms();`
  - `GLScissorTest::Enable(...)` if needed.
  - `material_.DefineVertexFormat(...)`
  - `geometry_.Bind(); geometry_.Draw(numInstances_);`

### 1.3 GPU buffer remap/flush lifecycle

Important for implementing Metal resource updates:

- `[Sources/nCine/Graphics/RenderBuffersManager.cpp](Sources/nCine/Graphics/RenderBuffersManager.cpp)`:
  - Manages mapped buffer memory in GL (UBOs/VBOs/IBOs).
  - `FlushUnmap()` uploads/flushes modified buffers.
  - `Remap()` re-maps buffers for the next frame.
- `[Sources/nCine/Graphics/ScreenViewport.cpp](Sources/nCine/Graphics/ScreenViewport.cpp)`:
  - After drawing:
    - `RenderResources::GetBuffersManager().Remap();`
    - `RenderResources::GetRenderCommandPool().Reset();`

## 2. OpenGL coupling points (must be replaced)

Metal backend work must remove/replace both:
- direct OpenGL calls (`glClear`, `glDraw*`, etc.)
- OpenGL-specific helper classes that encapsulate GPU state and resource binding.

### 2.1 Viewport clear + target binding + viewport/scissor

In `[Sources/nCine/Graphics/Viewport.cpp](Sources/nCine/Graphics/Viewport.cpp)`:

- `fbo_->Bind(GL_DRAW_FRAMEBUFFER);`
- `fbo_->DrawBuffers(numColorAttachments_);`
- `glClear(GL_COLOR_BUFFER_BIT | ...)`
- `GLViewport::SetRect(...)` + `GLViewport::SetState(...)`
- `GLScissorTest::{Enable,GetState,SetState,Disable}`

These correspond to Metal equivalents:
- Metal render pass descriptor clear actions
- per-viewport scissor rect
- render target binding (drawable vs FBO textures)

### 2.2 Render state setup (blending/depth/scissor)

In `[Sources/nCine/Graphics/RenderQueue.cpp](Sources/nCine/Graphics/RenderQueue.cpp)`:

- `GLBlending::Enable();`
- `GLBlending::SetBlendFunc(...)`
- `GLDepthTest::DisableDepthMask();`
- `GLDepthTest::EnableDepthMask();`
- `GLScissorTest::Disable();`

These correspond to:
- `MTLRenderPipelineState` settings + encoder state calls
- scissor enable/disable on the Metal render encoder

### 2.3 Command issue path (Material + Geometry)

In `[Sources/nCine/Graphics/RenderCommand.cpp](Sources/nCine/Graphics/RenderCommand.cpp)`:

- `material_.Bind();`
- `material_.CommitUniforms();`
- `GLScissorTest::Enable(scissorRect_...)`
- `material_.DefineVertexFormat(...)`
- `geometry_.Bind(); geometry_.Draw(...)`

All of those are OpenGL-backed today:
- `Material` hardcodes `GLShaderUniforms`, `GLShaderUniformBlocks`, and `GLTexture`
- `Shader` stores a `GLShaderProgram`
- `Geometry` draws via OpenGL `glDraw*` and binds GL buffer objects

### 2.4 GL resource classes embedded in engine-level types

Engine-level classes currently embed GL types:

- `Material` embeds:
  - `GLShaderUniforms`
  - `GLShaderUniformBlocks`
  - `GLTexture*` texture handles
- `Shader` embeds:
  - `GLShaderProgram`
- `Texture` embeds:
  - `GLTexture`
- `Geometry` embeds:
  - `GLBufferObject` for VBO/IBO

This implies the Metal port needs either:
- backend-agnostic abstractions replacing embedded GL types, OR
- platform-specific implementations that keep the same engine interfaces.

### 2.5 GfxCapabilities is OpenGL-based

`GfxCapabilities` in:

- `[Sources/nCine/Graphics/GfxCapabilities.cpp](Sources/nCine/Graphics/GfxCapabilities.cpp)`

Uses `glGetString`, `glGetIntegerv(...)`, and extension queries.

Metal backend needs:
- `MetalGfxCapabilities` (or an updated interface) to provide alignment/limits used by buffer allocation and batching.

## 3. Shader pipeline coupling

Shader sources are stored as GLSL under:
- `[Sources/nCine/Shaders/*](Sources/nCine/Shaders/)`

And embedded into C strings during CMake generation:
- `[cmake/ncine_generated_sources.cmake](cmake/ncine_generated_sources.cmake)`

`RenderResources` and `Shader` then compile/link OpenGL shader programs:
- `RenderResources::Create()` loads default shaders
- `Shader::LoadFromMemory()` uses `GLShaderProgram` to compile/attach/link

Metal backend must produce:
- MSL (or precompiled Metal library) equivalents of these shaders
- correct uniform/attribute layout that matches the engine’s uniform block + vertex format expectations

## 4. Minimal list of files to touch first

If you’re starting the Metal port, the first replacements should be implemented in this order:

1. `[Sources/nCine/Graphics/Viewport.cpp](Sources/nCine/Graphics/Viewport.cpp)` (clear + scissor + target binding)
2. `[Sources/nCine/Graphics/RenderQueue.cpp](Sources/nCine/Graphics/RenderQueue.cpp)` (blend/depth state)
3. `[Sources/nCine/Graphics/RenderCommand.cpp](Sources/nCine/Graphics/RenderCommand.cpp)` (Material/Geometry bind + draw)
4. Engine-level render resource types:
   - `[Sources/nCine/Graphics/Material.h](Sources/nCine/Graphics/Material.h)`
   - `[Sources/nCine/Graphics/Shader.h](Sources/nCine/Graphics/Shader.h)`
   - `[Sources/nCine/Graphics/Texture.h](Sources/nCine/Graphics/Texture.h)`
   - `[Sources/nCine/Graphics/Geometry.h](Sources/nCine/Graphics/Geometry.h)`

Then implement Metal equivalents of:
- “GPU state helpers”: blending/depth/scissor
- “GPU objects”: shader pipeline, textures, vertex/index buffers, uniform buffers

