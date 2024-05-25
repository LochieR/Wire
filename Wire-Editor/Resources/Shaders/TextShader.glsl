#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in int a_FontIndex;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) out flat int v_FontIndex;

layout(push_constant) uniform Camera
{
	mat4 ViewProjection;
} u_Camera;

void main()
{
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
    v_FontIndex = a_FontIndex;
	gl_Position = u_Camera.ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) in flat int v_FontIndex;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_ID;

layout(set = 0, binding = 0) uniform sampler2D u_AtlasSamplers[32];

float ScreenPxRange()
{
    const float pxRange = 2.0;
    vec2 unitRange = vec2(pxRange) / vec2(textureSize(u_AtlasSamplers[v_FontIndex], 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(v_TexCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float Median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
    vec3 msd = texture(u_AtlasSamplers[v_FontIndex], v_TexCoord).rgb;
    float sd = Median(msd.r, msd.g, msd.b);
    float screenPxDistance = ScreenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    if (opacity == 0.0)
    {
        discard;
    }

    o_Color = mix(vec4(0.0), v_Color, opacity);

    if (o_Color.a == 0.0)
    {
        discard;
    }

    o_ID = 8;
}