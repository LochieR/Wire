#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;

layout(location = 0) out vec4 v_Color;

layout(push_constant) uniform Camera
{
	mat4 ViewProjection;
} u_Camera;

void main()
{
	v_Color = a_Color;
	gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in vec4 v_Color;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

void main()
{
	o_Color = v_Color;

	uint x = uint(gl_FragCoord.x);
	uint y = uint(gl_FragCoord.y);

	o_ID = 10;
}
