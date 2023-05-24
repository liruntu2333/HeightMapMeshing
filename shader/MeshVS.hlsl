#include "ShaderUtil.hlsli"

VertexOut main( VertexIn i )
{
	VertexOut o;
	o.PositionH = mul(float4(i.PositionL, 1.0f), g_ViewProjection);
	return o;
}
