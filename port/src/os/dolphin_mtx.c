#include <dolphin/mtx.h>

#include <math.h>
#include <string.h>

/*
 * Portable counterparts for the paired-single matrix/vector entry points used
 * by the original game.  The GameCube SDK implements these symbols in
 * PowerPC assembly; keeping the same ABI lets unmodified game code call them
 * on desktop targets.
 */

void PSMTXIdentity(Mtx m)
{
    static const Mtx identity = {
        { 1.0F, 0.0F, 0.0F, 0.0F },
        { 0.0F, 1.0F, 0.0F, 0.0F },
        { 0.0F, 0.0F, 1.0F, 0.0F },
    };
    memcpy(m, identity, sizeof(identity));
}

void PSMTXCopy(Mtx src, Mtx dst)
{
    memmove(dst, src, sizeof(Mtx));
}

void PSMTXConcat(Mtx a, Mtx b, Mtx result)
{
    Mtx temporary;
    MtxPtr output = result;
    int row;
    int column;

    if (result == a || result == b) {
        output = temporary;
    }

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            output[row][column] = a[row][0] * b[0][column] +
                                  a[row][1] * b[1][column] +
                                  a[row][2] * b[2][column];
        }
        output[row][3] = a[row][0] * b[0][3] +
                         a[row][1] * b[1][3] +
                         a[row][2] * b[2][3] + a[row][3];
    }

    if (output == temporary) {
        PSMTXCopy(temporary, result);
    }
}

void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z)
{
    PSMTXIdentity(m);
    m[0][3] = x;
    m[1][3] = y;
    m[2][3] = z;
}

void PSMTXScale(Mtx m, f32 x, f32 y, f32 z)
{
    memset(m, 0, sizeof(Mtx));
    m[0][0] = x;
    m[1][1] = y;
    m[2][2] = z;
}

void PSMTXQuat(Mtx m, QuaternionPtr quaternion)
{
    const f32 norm = quaternion->x * quaternion->x +
                     quaternion->y * quaternion->y +
                     quaternion->z * quaternion->z +
                     quaternion->w * quaternion->w;
    const f32 scale = 2.0F / norm;
    const f32 xs = quaternion->x * scale;
    const f32 ys = quaternion->y * scale;
    const f32 zs = quaternion->z * scale;
    const f32 wx = quaternion->w * xs;
    const f32 wy = quaternion->w * ys;
    const f32 wz = quaternion->w * zs;
    const f32 xx = quaternion->x * xs;
    const f32 xy = quaternion->x * ys;
    const f32 xz = quaternion->x * zs;
    const f32 yy = quaternion->y * ys;
    const f32 yz = quaternion->y * zs;
    const f32 zz = quaternion->z * zs;

    m[0][0] = 1.0F - (yy + zz);
    m[0][1] = xy - wz;
    m[0][2] = xz + wy;
    m[0][3] = 0.0F;
    m[1][0] = xy + wz;
    m[1][1] = 1.0F - (xx + zz);
    m[1][2] = yz - wx;
    m[1][3] = 0.0F;
    m[2][0] = xz - wy;
    m[2][1] = yz + wx;
    m[2][2] = 1.0F - (xx + yy);
    m[2][3] = 0.0F;
}

void PSVECSubtract(Vec* a, Vec* b, Vec* result)
{
    const f32 x = a->x - b->x;
    const f32 y = a->y - b->y;
    const f32 z = a->z - b->z;
    result->x = x;
    result->y = y;
    result->z = z;
}

void PSVECScale(Vec* source, Vec* result, f32 scale)
{
    const f32 x = source->x * scale;
    const f32 y = source->y * scale;
    const f32 z = source->z * scale;
    result->x = x;
    result->y = y;
    result->z = z;
}

void PSVECNormalize(Vec* source, Vec* result)
{
    const f32 magnitude = sqrtf(source->x * source->x +
                                source->y * source->y +
                                source->z * source->z);
    const f32 inverse = 1.0F / magnitude;
    PSVECScale(source, result, inverse);
}

f32 PSVECMag(Vec* vector)
{
    return sqrtf(vector->x * vector->x + vector->y * vector->y +
                 vector->z * vector->z);
}

f32 PSVECDotProduct(Vec* a, Vec* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void PSVECCrossProduct(Vec* a, Vec* b, Vec* result)
{
    const f32 x = a->y * b->z - a->z * b->y;
    const f32 y = a->z * b->x - a->x * b->z;
    const f32 z = a->x * b->y - a->y * b->x;
    result->x = x;
    result->y = y;
    result->z = z;
}
