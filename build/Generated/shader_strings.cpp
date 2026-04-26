#include "shader_strings.h"

namespace nCine {

char const * const ShaderStrings::batched_meshsprites_notexture_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

struct Instance
{
	mat4 modelMatrix;
	vec4 color;
	vec2 spriteSize;
};

layout (std140) uniform InstancesBlock
{
#ifndef BATCH_SIZE
	#define BATCH_SIZE (682) // 64 Kb / 96 b
#endif
	Instance[BATCH_SIZE] instances;
} block;

in vec2 aPosition;
in uint aMeshIndex;
out vec4 vColor;

#define i block.instances[aMeshIndex]

void main()
{
	vec4 position = vec4(aPosition.x * i.spriteSize.x, aPosition.y * i.spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * i.modelMatrix * position;
	vColor = i.color;
}
)";

char const * const ShaderStrings::batched_meshsprites_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

struct Instance
{
	mat4 modelMatrix;
	vec4 color;
	vec4 texRect;
	vec2 spriteSize;
};

layout (std140) uniform InstancesBlock
{
#ifndef BATCH_SIZE
	#define BATCH_SIZE (585) // 64 Kb / 112 b
#endif
	Instance[BATCH_SIZE] instances;
} block;

in vec2 aPosition;
in vec2 aTexCoords;
in uint aMeshIndex;
out vec2 vTexCoords;
out vec4 vColor;

#define i block.instances[aMeshIndex]

void main()
{
	vec4 position = vec4(aPosition.x * i.spriteSize.x, aPosition.y * i.spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * i.modelMatrix * position;
	vTexCoords = vec2(aTexCoords.x * i.texRect.x + i.texRect.y, aTexCoords.y * i.texRect.z + i.texRect.w);
	vColor = i.color;
}
)";

char const * const ShaderStrings::batched_sprites_notexture_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

struct Instance
{
	mat4 modelMatrix;
	vec4 color;
	vec2 spriteSize;
};

layout (std140) uniform InstancesBlock
{
#ifndef BATCH_SIZE
	#define BATCH_SIZE (682) // 64 Kb / 96 b
#endif
	Instance[BATCH_SIZE] instances;
} block;

out vec4 vColor;

#define i block.instances[gl_VertexID / 6]

void main()
{
	vec2 aPosition = vec2(1.0 - float(((gl_VertexID + 2) / 3) % 2), 1.0 - float(((gl_VertexID + 1) / 3) % 2));
	vec4 position = vec4(aPosition.x * i.spriteSize.x, aPosition.y * i.spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * i.modelMatrix * position;
	vColor = i.color;
}
)";

char const * const ShaderStrings::batched_sprites_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

struct Instance
{
	mat4 modelMatrix;
	vec4 color;
	vec4 texRect;
	vec2 spriteSize;
};

layout (std140) uniform InstancesBlock
{
#ifndef BATCH_SIZE
	#define BATCH_SIZE (585) // 64 Kb / 112 b
#endif
	Instance[BATCH_SIZE] instances;
} block;

out vec2 vTexCoords;
out vec4 vColor;

#define i block.instances[gl_VertexID / 6]

void main()
{
	vec2 aPosition = vec2(1.0 - float(((gl_VertexID + 2) / 3) % 2), 1.0 - float(((gl_VertexID + 1) / 3) % 2));
	vec4 position = vec4(aPosition.x * i.spriteSize.x, aPosition.y * i.spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * i.modelMatrix * position;
	vTexCoords = vec2(aPosition.x * i.texRect.x + i.texRect.y, aPosition.y * i.texRect.z + i.texRect.w);
	vColor = i.color;
}
)";

char const * const ShaderStrings::imgui_fs = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D uTexture;
in vec2 vTexCoords;
in vec4 vColor;
out vec4 fragColor;

void main()
{
	fragColor = vColor * texture(uTexture, vTexCoords);
}
)";

char const * const ShaderStrings::imgui_vs = R"(
uniform mat4 uGuiProjection;
uniform float uDepth;
in vec2 aPosition;
in vec2 aTexCoords;
in vec4 aColor;
out vec2 vTexCoords;
out vec4 vColor;

void main()
{
	gl_Position = uGuiProjection * vec4(aPosition, uDepth, 1.0);
	vTexCoords = aTexCoords;
	vColor = aColor;
}
)";

char const * const ShaderStrings::meshsprite_notexture_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

layout (std140) uniform InstanceBlock
{
	mat4 modelMatrix;
	vec4 color;
	vec2 spriteSize;
};

in vec2 aPosition;
out vec4 vColor;

void main()
{
	vec4 position = vec4(aPosition.x * spriteSize.x, aPosition.y * spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * modelMatrix * position;
	vColor = color;
}
)";

char const * const ShaderStrings::meshsprite_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

layout (std140) uniform InstanceBlock
{
	mat4 modelMatrix;
	vec4 color;
	vec4 texRect;
	vec2 spriteSize;
};

in vec2 aPosition;
in vec2 aTexCoords;
out vec2 vTexCoords;
out vec4 vColor;

void main()
{
	vec4 position = vec4(aPosition.x * spriteSize.x, aPosition.y * spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * modelMatrix * position;
	vTexCoords = vec2(aTexCoords.x * texRect.x + texRect.y, aTexCoords.y * texRect.z + texRect.w);
	vColor = color;
}
)";

char const * const ShaderStrings::sprite_fs = R"(
#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D uTexture;
in vec2 vTexCoords;
in vec4 vColor;
out vec4 fragColor;

void main()
{
	fragColor = texture(uTexture, vTexCoords) * vColor;
}
)";

char const * const ShaderStrings::sprite_notexture_fs = R"(
#ifdef GL_ES
precision mediump float;
#endif

in vec4 vColor;
out vec4 fragColor;

void main()
{
	fragColor = vColor;
}
)";

char const * const ShaderStrings::sprite_notexture_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

layout (std140) uniform InstanceBlock
{
	mat4 modelMatrix;
	vec4 color;
	vec2 spriteSize;
};

out vec4 vColor;

void main()
{
	vec2 aPosition = vec2(1.0 - float(gl_VertexID >> 1), float(gl_VertexID % 2));
	vec4 position = vec4(aPosition.x * spriteSize.x, aPosition.y * spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * modelMatrix * position;
	vColor = color;
}
)";

char const * const ShaderStrings::sprite_vs = R"(
uniform mat4 uProjectionMatrix;
uniform mat4 uViewMatrix;

layout (std140) uniform InstanceBlock
{
	mat4 modelMatrix;
	vec4 color;
	vec4 texRect;
	vec2 spriteSize;
};

out vec2 vTexCoords;
out vec4 vColor;

void main()
{
	vec2 aPosition = vec2(1.0 - float(gl_VertexID >> 1), float(gl_VertexID % 2));
	vec4 position = vec4(aPosition.x * spriteSize.x, aPosition.y * spriteSize.y, 0.0, 1.0);

	gl_Position = uProjectionMatrix * uViewMatrix * modelMatrix * position;
	vTexCoords = vec2(aPosition.x * texRect.x + texRect.y, aPosition.y * texRect.z + texRect.w);
	vColor = color;
}
)";

}
