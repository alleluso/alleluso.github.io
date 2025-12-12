// Common SDF functions and operations
// Taken from Inigo Quilez's articles on SDFs
// https://iquilezles.org/articles/distfunctions/

// ------------ Vector operations ------------

float dot2(in float2 v)
{
    return dot(v, v);
}

float dot2(in float3 v)
{
    return dot(v, v);
}

float ndot(in float2 a, in float2 b)
{
    return a.x * b.x - a.y * b.y;
}

// ------------ Transformations ------------

float3 opTx(in float3 p, in float3 t)
{
    return p - t;
}

float3 opRx(float3 p, float3 a)
{
    // convert to radians
    float3 r = radians(a);

    // build rotation matrix from Euler XYZ
    float cx = cos(r.x), sx = sin(r.x);
    float cy = cos(r.y), sy = sin(r.y);
    float cz = cos(r.z), sz = sin(r.z);

    float3x3 m;
    m[0] = float3(cy * cz, - cy * sz, sy);
    m[1] = float3(sx * sy * cz + cx * sz, - sx * sy * sz + cx * cz, - sx * cy);
    m[2] = float3(- cx * sy * cz + sx * sz, cx * sy * sz + sx * cz, cx * cy);

    // use transpose(m) for inverse rotation
    return mul(p, transpose(m));
}

// ------------ Boolean operations ------------

float opUnion( float d1, float d2 )
{
    return min(d1,d2);
}

float opSubtraction( float d1, float d2 )
{
    return max(-d1,d2);
}

float opIntersection( float d1, float d2 )
{
    return max(d1,d2);
}

float opXor( float d1, float d2 )
{
    return max(min(d1,d2),-max(d1,d2));
}

float opSmoothUnion( float d1, float d2, float k )
{
    k *= 4.0;
    float h = max(k-abs(d1-d2),0.0);
    return min(d1, d2) - h*h*0.25/k;
}

float opSmoothSubtraction( float d1, float d2, float k )
{
    return -opSmoothUnion(d1,-d2,k);
}

float opSmoothIntersection( float d1, float d2, float k )
{
    return -opSmoothUnion(-d1,-d2,k);
}

// ------------ Helper functions ------------

float rounding( in float d, in float h )
{
    return d - h;
}

float opDisplace( in float d, in float disp )
{;
    return d + disp;
}

// ------------ Primitives ------------

float sdSphere(float3 p, float s)
{
    return length(p) - s;
}

float sdCappedCone(float3 p, float h, float r1, float r2)
{
    float2 q = float2(length(p.xz), p.y);
    float2 k1 = float2(r2, h);
    float2 k2 = float2(r2 - r1, 2.0 * h);
    float2 ca = float2(q.x - min(q.x, (q.y < 0.0)?r1 : r2), abs(q.y) - h);
    float2 cb = q - k1 + k2 * clamp(dot(k1 - q, k2) / dot2(k2), 0.0, 1.0);
    float s = (cb.x < 0.0 && ca.y < 0.0) ? - 1.0 : 1.0;
    return s * sqrt(min(dot2(ca), dot2(cb)));
}