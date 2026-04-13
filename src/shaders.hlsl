// shaders


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

	float4 colors[3] = {
		float4(1.0f, 0.0f, 0.0f, 1.0f),
		float4(0.0f, 1.0f, 0.0f, 1.0f),
		float4(0.0f, 0.0f, 1.0f, 1.0f),
	};

	VSOutput output;
	output.position = float4(input.position, 1.0f);
	output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
	return input.color;
}