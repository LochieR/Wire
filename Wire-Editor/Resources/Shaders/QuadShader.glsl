#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in int a_TexIndex;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) out flat int v_TexIndex;

layout(push_constant) uniform Camera
{
	mat4 ViewProjection;
} u_Camera;

void main()
{
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in flat int v_TexIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

layout(set = 0, binding = 0) uniform sampler2D u_TextureSamplers[32];

void main()
{
	vec4 texColor = texture(u_TextureSamplers[v_TexIndex], v_TexCoord);
	texColor *= v_Color;

	o_Color = texColor;

	o_ID = 12;
}
