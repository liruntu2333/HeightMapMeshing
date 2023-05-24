#include "ShaderUtil.hlsli"

static const float4 g_Plane[6] =
{
	float4(-300.0f, 0.0f, -300.0f, 1.0f),
	float4(-300.0f, 0.0f, +300.0f, 1.0f),
	float4(+300.0f, 0.0f, +300.0f, 1.0f),
	float4(-300.0f, 0.0f, -300.0f, 1.0f),
	float4(+300.0f, 0.0f, +300.0f, 1.0f),
	float4(+300.0f, 0.0f, -300.0f, 1.0f),
};

float4 main(uint vi : SV_VertexID) : SV_Position
{
	return mul(g_Plane[vi], g_ViewProjection);
}