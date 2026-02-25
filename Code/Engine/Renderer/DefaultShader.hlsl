cbuffer CameraConstants : register(b2)
{
	float OrthoMinX;
	float OrthoMinY;
	float OrthoMinZ;
	float OrthoMaxX;
	float OrthoMaxY;
	float OrthoMaxZ;
	float pad0;
	float pad1;
};

struct VS_INPUT
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

float Interpolate(float start, float end, float fraction)
{
	return start * (1.0f - fraction) + end * fraction;
}

float GetFractionWithinRange(float value, float start, float end)
{
	return (value - start) / (end - start);
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float fraction = GetFractionWithinRange(inValue, inStart, inEnd);
	return Interpolate(outStart, outEnd, fraction);
}

VS_OUTPUT VertexMain(VS_INPUT input)
{
	float4 localPosition = float4(input.localPosition, 1);

	float4 clipPosition;
	clipPosition.x = RangeMap(localPosition.x, OrthoMinX, OrthoMaxX, -1.0f, 1.0f);
	clipPosition.y = RangeMap(localPosition.y, OrthoMinY, OrthoMaxY, -1.0f, 1.0f);
	clipPosition.z = RangeMap(localPosition.z, OrthoMinZ, OrthoMaxZ, 0.0f, 1.0f);
	clipPosition.w = localPosition.w;

	VS_OUTPUT o;
	o.position = clipPosition;
	o.color = input.color;
	o.uv = input.uv;
	return o;
}


float4 PixelMain(VS_OUTPUT input) : SV_Target
{
	return float4(1.f, 1.f, 1.f, 1.f);
}