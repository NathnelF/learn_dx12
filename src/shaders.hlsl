// shaders
#pragma pack_matrix(column_major)

cbuffer CameraConstants : register(b0)
{
	float4x4 mvp;
}

struct VSInput
{
	float3 position : POSITION;
};

struct VSOutput
{
	float4 position : SV_Position;
	float4 color : COLOR;
};

VSOutput VSMain(VSInput input)
{
	VSOutput output;
	output.position = mul(mvp, float4(input.position, 1.0f));
	output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
	return input.color;
}