#type vertex
#version 450 core

layout(location = 0) in vec3 a_WorldPosition;
layout(location = 1) in vec3 a_LocalPosition;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_Thickness;
layout(location = 4) in float a_Fade;

layout(location = 0) out vec3 v_LocalPosition;
layout(location = 1) out vec4 v_Color;
layout(location = 2) out float v_Thickness;
layout(location = 3) out float v_Fade;

layout(push_constant) uniform Camera
{
	mat4 ViewProjection;
} u_Camera;

void main()
{
	v_LocalPosition = a_LocalPosition;
	v_Color = a_Color;
	v_Thickness = a_Thickness;
	v_Fade = a_Fade;
	gl_Position = u_Camera.ViewProjection * vec4(a_WorldPosition, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in vec3 v_LocalPosition;
layout(location = 1) in vec4 v_Color;
layout(location = 2) in float v_Thickness;
layout(location = 3) in float v_Fade;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

void main()
{
	float dist = 1.0 - length(v_LocalPosition);
	float circle = smoothstep(0.0, v_Fade, dist);
	circle *= smoothstep(v_Thickness + v_Fade, v_Thickness, dist);

	if (circle == 0.0)
		discard;

	vec4 color = v_Color;
	color.a *= circle;

	o_Color = color;

	uint x = uint(gl_FragCoord.x);
	uint y = uint(gl_FragCoord.y);

	o_ID = 11;
}
