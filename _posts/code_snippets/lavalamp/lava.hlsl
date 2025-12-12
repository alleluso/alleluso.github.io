#ifndef LAVA_NODE_INCLUDED
#define LAVA_NODE_INCLUDED

#define MAX_STEPS 128
#define MAX_DIST 15
#define SURF_DIST 0.0001

#include "sdf.hlsl"
#include "easing.hlsl"

float SDF(float3 p, float t)
{
    float time = t * 0.15;
    float time2 = t * 0.1;

    float smoothUnionVal = clamp(saturate(abs((p.y - 0.1) * 0.08)), 0.01, 1.0);

    // Bottle Shape (cone with rounded edges)
    float height = 0.55;
    float topSize = 0.17;
    float bottomSize = 0.21;
    float round = 0.05;
    float bottle = rounding(sdCappedCone(p, height - round, bottomSize - round, topSize - round), round);

    // Top and Bottom wax blobs
    float tp = sdSphere(opTx(p, float3(0, 0.6, 0)), topSize * 1.25);
    float btm = sdSphere(opTx(p, float3(0, -0.6, 0)), bottomSize * 1.25);

    // Combine Top and Bottom wax blobs
    float wax = opSmoothUnion(btm, tp, smoothUnionVal);

    // Add animated noise to wax surface
    wax = opDisplace(wax, sin(15*p.x + t)*sin(25*p.y - t * 2)*sin(35*p.z + t) * 0.015);

    // Add blobs that move up and down at different speeds
    float sp1 = sdSphere(opTx(p, float3(0, easeInSine(frac(time)) * 2 - 0.75, 0)), 0.11);
    wax = opSmoothUnion(wax, sp1, smoothUnionVal);

    float sp2 = sdSphere(opTx(p, float3(0.07, easeInCubic(frac(time2 + 0.25)) * -1.5 + 0.8, 0.07)), 0.075);
    wax = opSmoothUnion(wax, sp2, smoothUnionVal);

    float sp3 = sdSphere(opTx(p, float3(-0.07, easeInCubic(frac(time2 * 0.9 + 0.75)) * -1.5 + 0.8, 0.07)), 0.04);
    wax = opSmoothUnion(wax, sp3, smoothUnionVal);

    float sp4 = sdSphere(opTx(p, float3(0.07, sin((t)) * 0.25 - 0.5, -0.07)), 0.076);
    wax = opSmoothUnion(wax, sp4, smoothUnionVal);

    float sp5 = sdSphere(opTx(p, float3(-0.07, sin((t * 0.45 + 0.78)) * 0.5, -0.07)), 0.05);
    wax = opSmoothUnion(wax, sp5, smoothUnionVal);

    // Add wavy distortion to wax surface
    wax = opDisplace(wax, sin(15*p.y + t) * sin(35*p.y) * 0.3 * smoothUnionVal);

    // Final shape intersection of bottle and wax
    return opIntersection(bottle, wax);
}

// Computes normal at point p on the surface defined by SDF
float3 GetNormal(float dS, float3 p, float time){
    float2 e = float2(0.001, 0.0);
    float3 n = dS - float3(
        SDF(p - e.xyy, time),
        SDF(p - e.yxy, time),
        SDF(p - e.yyx, time)
    );
    return normalize(n);
}

// Marches the ray and returns hit info (x < 0 means no hit)
// Returns float4(distance, normal.x, normal.y, normal.z)
float4 Raymarch(float3 ro, float3 rd, float time)
{
    float dO = 0.0;
    float dS = 0.0;

    for (int i = 0; i < MAX_STEPS; i++) {
        float3 p = ro + rd * dO;
        dS = SDF(p, time);

        if (dS < SURF_DIST) {
            float3 n = GetNormal(dS, p, time);
            return float4(dO, n);
        }

        dO += max(abs(dS), 1e-4);
        if (dO > MAX_DIST) 
            break;
    }

    return float4(-1.0, 0.0, 0.0, 0.0); // no hit
}

// Main function for Shader Graph node
void LavaNode_float(float3 ro, float3 rd, float time, out bool hit, out float3 hitPos, out float3 hitNormal)
{
    float4 raymarch = Raymarch(ro, rd, time);
    hit = raymarch.x >= 0.0;

    if(hit)
    {
        hitPos = ro + rd * raymarch.x;
        hitNormal = raymarch.yzw;
    }
    else
    {
        hitPos = float3(0,0,0);
        hitNormal = float3(0,0,0);
    }
}

#endif // LAVA_NODE_INCLUDED