//*************************//
// SKY EFFECT [DEFERRED] //
//*************************//

/*
	- Diffuse Color/Texture
	- Specular Color
	- SpecularLevel Texture (Blinn)
	- Specular Intensity (Shininess)
	- NormalMap Texture
	- Ambient Color
	- Ambient Intensity [0-1]
	- Opacity Texture/Value [0-1]
*/

//GLOBAL MATRICES
//***************
// The World View Projection Matrix
float4x4 gWorldViewProj : WORLDVIEWPROJECTION;
// The ViewInverse Matrix - the third row contains the camera position!
float4x4 gViewInverse : VIEWINVERSE;
// The World Matrix
float4x4 gWorld : WORLD;

//STATES
//******
RasterizerState gRasterizerState
{
	FillMode = SOLID;
	CullMode = NONE;
};

BlendState gBlendState
{
	BlendEnable[0] = FALSE;
};

DepthStencilState gDepthState
{
	DepthEnable = TRUE;
	DepthFunc = LESS_EQUAL;
	DepthWriteMask = ALL;
};

//SAMPLER STATES
//**************
SamplerState gTextureSampler
{
	Filter = MIN_MAG_MIP_LINEAR;
	//Filter = ANISOTROPIC;
	AddressU = WRAP;
	AddressV = WRAP;
	AddressW = WRAP;
};

//LIGHT
//*****
float3 gLightDirection:DIRECTION
<
	string UIName = "Light Direction";
	string Object = "TargetLight";
> = float3(0.577f, 0.577f, 0.577f);

//VS IN & OUT
//***********
struct VS_Input
{
	float3 Position: POSITION;
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float2 TexCoord: TEXCOORD0;
};

struct VS_Output
{
	float4 Position: SV_POSITION;
	float3 Normal: NORMAL;
	float3 Tangent: TANGENT;
	float3 Binormal: BINORMAL;
	float2 TexCoord: TEXCOORD0;
	float3 Position2: TEXCOORD1;
};

struct PS_Output
{
	float4 LightAccumulation : SV_TARGET0;
	float4 Diffuse : SV_TARGET1;
	float4 Specular : SV_TARGET2;
	float4 Normal : SV_TARGET3;
	float4 Bloom : SV_TARGET4;
};

// The main vertex shader
VS_Output MainVS(VS_Input input) {

	VS_Output output = (VS_Output)0;
	
	output.Position = mul(float4(input.Position, 0.0f), gWorldViewProj).xyww;
	output.Position2 = float4(normalize(input.Position), 0.0f);

	output.Normal = normalize(mul(input.Normal, (float3x3)gWorld));
	output.Tangent = normalize(mul(input.Tangent, (float3x3)gWorld));
	output.Binormal = normalize(mul(input.Binormal, (float3x3)gWorld));

	output.TexCoord = input.TexCoord;

	return output;
}

float4 gSunDirection: POSITION;

float turbidity = 2;
float rayleigh = 1;
float mieCoefficient = 0.005;
float mieDirectionalG = 0.8;
static const float3 up = { 0, 1, 0 };

// constants for atmospheric scattering
static const float e = 2.71828182845904523536028747135266249775724709369995957;

// wavelength of used primaries, according to preetham
static const float3 lambda = float3(680E-9, 550E-9, 450E-9);
// this pre-calcuation replaces older TotalRayleigh(float3 lambda) function:
// (8.0 * pow(pi, 3.0) * pow(pow(n, 2.0) - 1.0, 2.0) * (6.0 + 3.0 * pn)) / (3.0 * N * pow(lambda, float3(4.0)) * (6.0 - 7.0 * pn))
static const float3 totalRayleigh = float3(5.804542996261093E-5, 1.3562911419845635E-5, 3.0265902468824876E-5);

// mie stuff
// K coefficient for the primaries
static const float v = 4.0;
static const float3 K = float3(0.686, 0.678, 0.666);
// MieConst = pi * pow( ( 2.0 * pi ) / lambda, float3( v - 2.0 ) ) * K
static const float3 MieConst = float3(2.8399918514433978E14, 2.7798023919660528E14, 2.0790479543861094E14);

// earth shadow hack
// cutoffAngle = pi / 1.95;
static const float cutoffAngle = 1.6110731556870734;
static const float steepness = 1.5;
static const float EE = 1000.0;

float sunIntensity(float zenithAngleCos) {
	zenithAngleCos = clamp(zenithAngleCos, -1.0, 1.0);
	return EE * max(0.0, 1.0 - pow(e, -((cutoffAngle - acos(zenithAngleCos)) / steepness)));
}

float3 totalMie(float T) {
	float c = (0.2 * T) * 10E-18;
	return 0.434 * c * MieConst;
}

static const float pi = 3.141592653589793238462643383279502884197169;

static const float n = 1.0003; // refractive index of air
static const float N = 1.545E25; // number of molecules per unit volume for air at 288.15K and 1013mb (sea level -45 celsius)

// optical length at zenith for molecules
static const float rayleighZenithLength = 8.4E3;
static const float mieZenithLength = 1.25E3;
// 66 arc seconds -> degrees, and the cosine of that
static const float sunAngularDiameterCos = 0.999956676946448443553574619906976478926848692873900859324;

// 3.0 / ( 16.0 * pi )
static const float THREE_OVER_SIXTEENPI = 0.05968310365946075;
// 1.0 / ( 4.0 * pi )
static const float ONE_OVER_FOURPI = 0.07957747154594767;

float rayleighPhase(float cosTheta) {
	return THREE_OVER_SIXTEENPI * (1.0 + pow(cosTheta, 2.0));
}

float hgPhase(float cosTheta, float g) {
	float g2 = pow(g, 2.0);
	float inverse = 1.0 / pow(1.0 - 2.0 * g * cosTheta + g2, 1.5);
	return ONE_OVER_FOURPI * ((1.0 - g2) * inverse);
}

// The main pixel shader
PS_Output MainPS(VS_Output input){

	float3 vSunDirection = normalize(-gSunDirection.xyz);

	float vSunE = sunIntensity(dot(vSunDirection, up));

	float vSunfade = 1.0 - clamp(1.0 - exp((-gSunDirection.y / 450000.0)), 0.0, 1.0);

	float rayleighCoefficient = rayleigh - (1.0 * (1.0 - vSunfade));

	// extinction (absorbtion + out scattering)
	// rayleigh coefficients
	float3 vBetaR = totalRayleigh * rayleighCoefficient;

	// mie coefficients
	float3 vBetaM = totalMie(turbidity) * mieCoefficient;

	float3 direction = normalize(input.Position2).xyz;

	// optical length
	// cutoff angle at 90 to avoid singularity in next formula.
	float zenithAngle = acos(max(0.0, dot(up, direction)));
	float inverse = 1.0 / (cos(zenithAngle) + 0.15 * pow(93.885 - ((zenithAngle * 180.0) / pi), -1.253));
	float sR = rayleighZenithLength * inverse;
	float sM = mieZenithLength * inverse;

	// combined extinction factor
	float3 Fex = exp(-(vBetaR * sR + vBetaM * sM));

	// in scattering
	float cosTheta = dot(direction, vSunDirection);

	float rPhase = rayleighPhase(cosTheta * 0.5 + 0.5);
	float3 betaRTheta = vBetaR * rPhase;

	float mPhase = hgPhase(cosTheta, mieDirectionalG);
	float3 betaMTheta = vBetaM * mPhase;

	float3 Lin = pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * (1.0 - Fex), 1.5);
	Lin *= lerp(float3(1.0, 1.0, 1.0), pow(vSunE * ((betaRTheta + betaMTheta) / (vBetaR + vBetaM)) * Fex, 1.0 / 2.0), clamp(pow(1.0 - dot(up, vSunDirection), 5.0), 0.0, 1.0));

	// nightsky
	float theta = acos(direction.y); // elevation --> y-axis, [-pi/2, pi/2]
	float phi = atan2(direction.z, direction.x); // azimuth --> x-axis [-pi/2, pi/2]
	float2 uv = float2(phi, theta) / float2(2.0 * pi, pi) + float2(0.5, 0.0);
	float3 L0 = 0.1 * Fex;

	// composition + solar disc
	float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
	L0 += (vSunE * 19000.0 * Fex) * sundisk;

	float3 texColor = (Lin + L0) * 0.02 + float3(0.01, 0.0008, 0.00075);

	float3 retColor = pow(texColor, 1.0 / (1.2 + (1.2 * vSunfade)));

	PS_Output output = (PS_Output)0;

	output.LightAccumulation = float4(retColor, 1.0);
    output.Bloom = float4(clamp(retColor - 1.0, 0.0, 10.0), 1.0);
	output.Diffuse = float4(0.0, 0.0, 0.0, 1.0);
	output.Specular = float4(0.0, 0.0, 0.0, 0.0);
	output.Normal = float4(input.Normal, 1.0);

	return output;
}

// Default Technique
technique10 Default {
	pass p0 {
		SetDepthStencilState(gDepthState, 0);
		SetRasterizerState(gRasterizerState);
		SetBlendState(gBlendState, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xFFFFFFFF);

		SetVertexShader(CompileShader(vs_4_0, MainVS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_4_0, MainPS()));
	}
}