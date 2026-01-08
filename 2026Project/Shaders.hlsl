struct MATERIAL
{
	float4					m_cAmbient;
	float4					m_cDiffuse;
	float4					m_cSpecular; //a = power
	float4					m_cEmissive;
};

cbuffer cbCameraInfo : register(b1)
{
	matrix					gmtxView : packoffset(c0);
	matrix					gmtxProjection : packoffset(c4);
	float3					gvCameraPosition : packoffset(c8);
};

cbuffer cbGameObjectInfo : register(b2)
{
	matrix					gmtxGameObject : packoffset(c0);
	MATERIAL				gMaterial : packoffset(c4);
};

#include "Light.hlsl"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

//#define _WITH_VERTEX_LIGHTING
Texture2D gBillboardTexture : register(t0); // 나무/수풀 텍스처

SamplerState gSampler : register(s0); // 텍스처 샘플러
SamplerState gShadowSampler : register(s1);

Texture2D gShadowMap : register(t4);

cbuffer cbShadowLightInfo : register(b5)
{
    matrix gmtxLightViewProj : packoffset(c0);
};

struct VS_LIGHTING_INPUT
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct VS_LIGHTING_OUTPUT
{
	float4 position : SV_POSITION;
	float3 positionW : POSITION;
	float3 normalW : NORMAL;
    
    float4 positionLight : TEXCOORD1;
};

struct VS_SHADOW_INPUT
{
    float3 position : POSITION;
};

struct VS_SHADOW_OUTPUT
{
    float4 position : SV_POSITION; 
};

VS_SHADOW_OUTPUT VS_Shadow(VS_SHADOW_INPUT input)
{
    VS_SHADOW_OUTPUT output;
    
    float4 worldPos = mul(float4(input.position, 1.0f), gmtxGameObject);
    
    output.position = mul(worldPos, gmtxLightViewProj);

    return output;
}

VS_LIGHTING_OUTPUT VSLighting(VS_LIGHTING_INPUT input)
{
	VS_LIGHTING_OUTPUT output;

	output.normalW = mul(input.normal, (float3x3)gmtxGameObject);
	output.positionW = (float3)mul(float4(input.position, 1.0f), gmtxGameObject);
	output.position = mul(mul(float4(output.positionW, 1.0f), gmtxView), gmtxProjection);
#ifdef _WITH_VERTEX_LIGHTING
	output.normalW = normalize(output.normalW);
	output.color = Lighting(output.positionW, output.normalW);
#endif
    matrix T = matrix(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    );
    output.positionLight = mul(float4(output.positionW, 1.0f), mul(gmtxLightViewProj, T));
    
	return(output);
}

float4 PSLighting(VS_LIGHTING_OUTPUT input) : SV_TARGET
{
    input.normalW = normalize(input.normalW);
    float4 color = Lighting(input.positionW, input.normalW);

    float3 shadowPos = input.positionLight.xyz / input.positionLight.w;
    float shadowDepth = gShadowMap.Sample(gShadowSampler, shadowPos.xy).r;
    float currentDepth = shadowPos.z;

    float bias = 0.001f;

    float shadowFactor = 1.0f;
    
    if (shadowPos.x >= 0.0f && shadowPos.x <= 1.0f && shadowPos.y >= 0.0f && shadowPos.y <= 1.0f)
    {
        if (currentDepth - bias > shadowDepth)
        {
            shadowFactor = 0.4f;
        }
    }
    return float4(color.rgb * shadowFactor, color.a);
}

struct VS_DIFFUSED_INPUT
{
    float3 position : POSITION;
};


struct VS_DIFFUSED_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR; 
};


VS_DIFFUSED_OUTPUT VSDiffused(VS_DIFFUSED_INPUT input)
{
    VS_DIFFUSED_OUTPUT output;
    
    float3 positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);

    output.position = mul(mul(float4(positionW, 1.0f), gmtxView), gmtxProjection);

    output.color = gMaterial.m_cDiffuse;
    
    return (output);
}


float4 PSDiffused(VS_DIFFUSED_OUTPUT input) : SV_TARGET
{
    return (input.color);
}

///////////////////////////////////////////////////

struct VS_IN
{
    float3 posW : POSITION;
    float2 sizeW : SIZE;
};

struct VS_OUT
{
    float3 centerW : POSITION;
    float2 sizeW : SIZE;
};

struct GS_OUT
{
    float4 posH : SV_Position;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 uv : TEXCOORD;
};

VS_OUT VS_Billboard(VS_IN input)
{
    VS_OUT output;
    output.centerW = (float3) mul(float4(input.posW, 1.0f), gmtxGameObject);
    output.sizeW = input.sizeW;
    
    return output;
}

[maxvertexcount(4)]
void GS(point VS_OUT input[1], inout TriangleStream<GS_OUT> outStream)
{
    float3 vUp = float3(0, 1, 0);
    float3 vLook = gvCameraPosition.xyz - input[0].centerW;
    
    vLook = normalize(vLook);
    
    float3 vRight = cross(vUp, vLook);
    
    float fHalfW = input[0].sizeW.x * 0.5f;
    float fHalfH = input[0].sizeW.y * 0.5f;
    
    float4 pVertices[4];
    
    pVertices[0] = float4(input[0].centerW + fHalfW * vRight - fHalfH * vUp, 1.0f);
    pVertices[1] = float4(input[0].centerW + fHalfW * vRight + fHalfH * vUp, 1.0f);
    pVertices[2] = float4(input[0].centerW - fHalfW * vRight - fHalfH * vUp, 1.0f);
    pVertices[3] = float4(input[0].centerW - fHalfW * vRight + fHalfH * vUp, 1.0f);
    
    float2 pUVs[4] = { float2(0.0f, 1.0f), float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(1.0f, 0.0f) };
    
    GS_OUT output;
    
    for (int i = 0; i < 4; ++i){
        output.posW = pVertices[i].xyz;
        output.posH = mul(mul(pVertices[i], gmtxView), gmtxProjection);
        output.normalW = vLook;
        output.uv = pUVs[i];
        outStream.Append(output);    
    }
}

float4 PS_Billboard(GS_OUT input) : SV_Target
{
    float4 vDiffuse = gBillboardTexture.Sample(gSampler, input.uv);
    
    clip(vDiffuse.a - 0.5f);
    
    return vDiffuse;
}

struct VSParticle_IN
{
    float3 posW : POSITION;
    float2 sizeW : SIZE;
};

struct VSParticle_OUT
{
    float3 centerW : POSITION;
    float2 sizeW : SIZE;
};

VSParticle_OUT VSParticle(VSParticle_IN input)
{
    VSParticle_OUT output;
    
    output.centerW = input.posW;
    output.sizeW = input.sizeW;
    
    return output;
}

RWTexture2D<float4> gOutputColor : register(u0);

Texture2D<float4> gInputColor : register(t0);

cbuffer cbBlurParams : register(b0)
{
    float4 gBlurParams;
};

[numthreads(32, 32, 1)]
void CS_RadialBlur(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    
    uint width, height;
    gOutputColor.GetDimensions(width, height);
    
    if (pixelCoord.x >= width || pixelCoord.y >= height)
        return;
    
    float strength = gBlurParams.x;
    float2 center = gBlurParams.yz;
    float aspectRatio = gBlurParams.w;
    
    float2 uv = float2(pixelCoord.x / (float) width, pixelCoord.y / (float) height);
    
    float2 uvCorrected = uv;
    float2 centerCorrected = center;
    
    uvCorrected.x *= aspectRatio;
    centerCorrected.x *= aspectRatio;
    
    float2 dir = uv - center;
    float dist = length(uvCorrected - centerCorrected);
    
    float4 color = float4(0, 0, 0, 0);
    int nSamples = 12;
    
    for (int i = 0; i < nSamples; ++i)
    {
        float scale = strength * (i / (float) (nSamples - 1)) * dist;
        
        float2 sampleUV = uv - (dir * scale);
        
        int2 sampleCoord = int2(sampleUV.x * width, sampleUV.y * height);
        sampleCoord = clamp(sampleCoord, int2(0, 0), int2(width - 1, height - 1));
        
        color += gInputColor.Load(int3(sampleCoord, 0));
    }

    color /= nSamples;
    gOutputColor[pixelCoord] = color;
}

struct VS_TESS_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VS_TESS_OUTPUT
{
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
};

VS_TESS_OUTPUT VS_Tess(VS_TESS_INPUT input)
{
    VS_TESS_OUTPUT output;
    
    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    return output;
}

struct HS_CONSTANT_DATA_OUTPUT
{
    float EdgeTessFactor[3] : SV_TessFactor;
    float InsideTessFactor : SV_InsideTessFactor;
};

struct HS_CONTROL_POINT_OUTPUT
{
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
};

HS_CONSTANT_DATA_OUTPUT CalHSPatchConstants( InputPatch<VS_TESS_OUTPUT, 3> input, uint PatchID : SV_PrimitiveID)
{
    HS_CONSTANT_DATA_OUTPUT Output;
    
    float3 eyePos = gvCameraPosition;
    
    float3 center = (input[0].positionW + input[1].positionW + input[2].positionW) / 3.0f;
    
    float d = distance(center, eyePos);
    
    const float d0 = 100.0f;
    const float d1 = 500.0f;
    
    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));
    tess = max(1.0f, tess); 

    Output.EdgeTessFactor[0] = tess;
    Output.EdgeTessFactor[1] = tess;
    Output.EdgeTessFactor[2] = tess;
    Output.InsideTessFactor = tess;

    return Output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("CalHSPatchConstants")]
[maxtessfactor(64.0f)]
HS_CONTROL_POINT_OUTPUT HS_Tess(InputPatch<VS_TESS_OUTPUT, 3> input, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID)
{
    HS_CONTROL_POINT_OUTPUT Output;
    
    Output.positionW = input[i].positionW;
    Output.normalW = input[i].normalW;
    
    return Output;
}

struct DS_OUTPUT
{
    float4 position : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    
    float4 positionLight : TEXCOORD1;
};

[domain("tri")]
void DS_Tess(HS_CONSTANT_DATA_OUTPUT input, const OutputPatch<HS_CONTROL_POINT_OUTPUT, 3> patch, float3 bary : SV_DomainLocation, out DS_OUTPUT output)
{
    float3 posW = patch[0].positionW * bary.x + patch[1].positionW * bary.y + patch[2].positionW * bary.z;
    float3 normW = patch[0].normalW * bary.x + patch[1].normalW * bary.y + patch[2].normalW * bary.z;

    output.positionW = posW;
    output.normalW = normalize(normW);
    output.position = mul(mul(float4(posW, 1.0f), gmtxView), gmtxProjection);
    
    matrix T = matrix(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f
    );
    
    output.positionLight = mul(float4(posW, 1.0f), mul(gmtxLightViewProj, T));
}

float4 PS_Tess(DS_OUTPUT input) : SV_TARGET
{
    input.normalW = normalize(input.normalW);

    float4 color = Lighting(input.positionW, input.normalW);
    
    float3 shadowPos = input.positionLight.xyz / input.positionLight.w;
    float shadowDepth = gShadowMap.Sample(gShadowSampler, shadowPos.xy).r;
    float currentDepth = shadowPos.z;

    float bias = 0.001f;
    float shadowFactor = 1.0f;
    
    if (shadowPos.x >= 0.0f && shadowPos.x <= 1.0f && shadowPos.y >= 0.0f && shadowPos.y <= 1.0f)
    {
        if (currentDepth - bias > shadowDepth)
        {
            shadowFactor = 0.4f; 
        }
    }
    
    return float4(color.rgb * shadowFactor, color.a);
}

// ================================================================================
// [추가] 파티클 시스템 전용 셰이더 (독립 변수 사용)
// ================================================================================

// 1. 파티클 전용 상수 버퍼 및 리소스 정의
// (기존 변수와 이름이 겹치지 않게 'Particle' 접두어를 붙였습니다)

// [b1] 카메라 정보 (EffectLibrary에서 b1에 View/Proj를 보냄)
cbuffer cbParticleCamera : register(b6)
{
    matrix gmtxParticleView;
    matrix gmtxParticleProjection;
};

// [b2] 월드 행렬 (EffectLibrary에서 b2에 World를 보냄)
cbuffer cbParticleWorld : register(b7)
{
    matrix gmtxParticleWorld;
};

// [t0] 텍스처 (EffectLibrary에서 t0에 텍스처를 바인딩함)
Texture2D gParticleTexture : register(t0);

// [s0] 샘플러 (EffectLibrary에서 s0에 정적 샘플러를 바인딩함)
SamplerState gParticleSampler : register(s0);

// 2. 구조체 정의
struct VS_PARTICLE_INPUT
{
    float3 position : POSITION;
    float2 size : TEXCOORD;
};

struct VS_PARTICLE_OUTPUT
{
    float3 positionW : POSITION;
    float2 size : TEXCOORD;
};

struct GS_PARTICLE_OUTPUT
{
    float4 positionH : SV_POSITION;
    float2 uv : TEXCOORD;
    uint primID : SV_PrimitiveID;
};

// 3. Vertex Shader (VS_Particle)
VS_PARTICLE_OUTPUT VS_Particle(VS_PARTICLE_INPUT input)
{
    VS_PARTICLE_OUTPUT output;
    
    // 로컬 -> 월드 변환 (gmtxParticleWorld 사용)
    output.positionW = mul(float4(input.position, 1.0f), gmtxParticleWorld).xyz;
    output.size = input.size;
    
    return output;
}

// 4. Geometry Shader (GS_Particle)
[maxvertexcount(4)]
void GS_Particle(point VS_PARTICLE_OUTPUT input[1], inout TriangleStream<GS_PARTICLE_OUTPUT> outStream)
{
    // 빌보드 처리를 위해 카메라의 Right/Up 벡터 추출 (View 행렬 활용)
    float3 vRight = float3(gmtxParticleView._11, gmtxParticleView._12, gmtxParticleView._13);
    float3 vUp = float3(gmtxParticleView._21, gmtxParticleView._22, gmtxParticleView._23);

    float3 p = input[0].positionW;
    float2 halfSize = input[0].size * 0.5f;

    // 사각형 꼭지점 생성
    float4 v[4];
    v[0] = float4(p + halfSize.x * vRight - halfSize.y * vUp, 1.0f);
    v[1] = float4(p + halfSize.x * vRight + halfSize.y * vUp, 1.0f);
    v[2] = float4(p - halfSize.x * vRight - halfSize.y * vUp, 1.0f);
    v[3] = float4(p - halfSize.x * vRight + halfSize.y * vUp, 1.0f);

    float2 uv[4] =
    {
        float2(1.0f, 1.0f), float2(1.0f, 0.0f),
        float2(0.0f, 1.0f), float2(0.0f, 0.0f)
    };

    GS_PARTICLE_OUTPUT output;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        // 월드 -> 뷰 -> 프로젝션
        float4 posV = mul(v[i], gmtxParticleView);
        output.positionH = mul(posV, gmtxParticleProjection);
        output.uv = uv[i];
        output.primID = (uint) i;
        
        outStream.Append(output);
    }
}

// 5. Pixel Shader (PS_Particle)
float4 PS_Particle(GS_PARTICLE_OUTPUT input) : SV_TARGET
{
    // 여기서 직접 정의한 텍스처와 샘플러 사용
    float4 color = gParticleTexture.Sample(gParticleSampler, input.uv);
    
    // (선택사항) 투명도 알파 테스트가 필요하면 주석 해제
    // clip(color.a - 0.1f);

    return color;
}