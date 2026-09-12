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

void PSMTXTranspose(Mtx src, Mtx xPose)
{
    Mtx temporary;
    MtxPtr output = (xPose == src) ? temporary : xPose;
    int row;
    int column;

    for (row = 0; row < 3; ++row) {
        for (column = 0; column < 3; ++column) {
            output[row][column] = src[column][row];
        }
        output[row][3] = 0.0F;
    }

    if (output == temporary) {
        PSMTXCopy(temporary, xPose);
    }
}

/* Affine inverse of a 3x4 matrix: the 3x3 block is inverted through its
 * adjugate and the translation column is mapped through that inverse.  A
 * singular 3x3 block returns 0 and leaves the destination untouched, matching
 * the SDK contract the game relies on. */
u32 PSMTXInverse(Mtx src, Mtx inv)
{
    Mtx temporary;
    MtxPtr output = (inv == src) ? temporary : inv;
    f32 determinant;
    f32 inverse_determinant;

    determinant = src[0][0] * (src[1][1] * src[2][2] - src[2][1] * src[1][2]) -
                  src[0][1] * (src[1][0] * src[2][2] - src[2][0] * src[1][2]) +
                  src[0][2] * (src[1][0] * src[2][1] - src[2][0] * src[1][1]);
    if (determinant == 0.0F) {
        return 0;
    }
    inverse_determinant = 1.0F / determinant;

    output[0][0] = (src[1][1] * src[2][2] - src[2][1] * src[1][2]) *
                   inverse_determinant;
    output[0][1] = -(src[0][1] * src[2][2] - src[2][1] * src[0][2]) *
                   inverse_determinant;
    output[0][2] = (src[0][1] * src[1][2] - src[1][1] * src[0][2]) *
                   inverse_determinant;
    output[1][0] = -(src[1][0] * src[2][2] - src[2][0] * src[1][2]) *
                   inverse_determinant;
    output[1][1] = (src[0][0] * src[2][2] - src[2][0] * src[0][2]) *
                   inverse_determinant;
    output[1][2] = -(src[0][0] * src[1][2] - src[1][0] * src[0][2]) *
                   inverse_determinant;
    output[2][0] = (src[1][0] * src[2][1] - src[2][0] * src[1][1]) *
                   inverse_determinant;
    output[2][1] = -(src[0][0] * src[2][1] - src[2][0] * src[0][1]) *
                   inverse_determinant;
    output[2][2] = (src[0][0] * src[1][1] - src[1][0] * src[0][1]) *
                   inverse_determinant;

    output[0][3] = -(output[0][0] * src[0][3] + output[0][1] * src[1][3] +
                     output[0][2] * src[2][3]);
    output[1][3] = -(output[1][0] * src[0][3] + output[1][1] * src[1][3] +
                     output[1][2] * src[2][3]);
    output[2][3] = -(output[2][0] * src[0][3] + output[2][1] * src[1][3] +
                     output[2][2] * src[2][3]);

    if (output == temporary) {
        PSMTXCopy(temporary, inv);
    }
    return 1;
}

/* Inverse transpose of the rotation block, used to carry normals through a
 * non-uniform transform.  The translation column is cleared because a normal
 * is a direction. */
u32 PSMTXInvXpose(Mtx src, Mtx invX)
{
    Mtx inverse;

    if (PSMTXInverse(src, inverse) == 0) {
        return 0;
    }
    PSMTXTranspose(inverse, invX);
    return 1;
}

/* Both the 3x4 and 4x4 matrix types share a four-float row stride and these
 * entry points only read the first three rows, so the shared helper takes a
 * row pointer.  Forwarding an Mtx to an Mtx44 parameter instead would claim
 * 64 readable bytes where the caller only owns 48. */
static void multiply_vector(MtxPtr m, Vec* src, Vec* dst)
{
    const f32 x = m[0][0] * src->x + m[0][1] * src->y + m[0][2] * src->z +
                  m[0][3];
    const f32 y = m[1][0] * src->x + m[1][1] * src->y + m[1][2] * src->z +
                  m[1][3];
    const f32 z = m[2][0] * src->x + m[2][1] * src->y + m[2][2] * src->z +
                  m[2][3];
    dst->x = x;
    dst->y = y;
    dst->z = z;
}

void PSMTXMultVec(Mtx44 m, Vec* src, Vec* dst)
{
    multiply_vector(m, src, dst);
}

/* Scale and rotation only: the translation column is skipped, so a direction
 * stays a direction. */
void PSMTXMultVecSR(Mtx44 m, Vec* src, Vec* dst)
{
    const f32 x = m[0][0] * src->x + m[0][1] * src->y + m[0][2] * src->z;
    const f32 y = m[1][0] * src->x + m[1][1] * src->y + m[1][2] * src->z;
    const f32 z = m[2][0] * src->x + m[2][1] * src->y + m[2][2] * src->z;
    dst->x = x;
    dst->y = y;
    dst->z = z;
}

void PSMTXMultVecArray(Mtx m, Vec* srcBase, Vec* dstBase, u32 count)
{
    u32 i;

    for (i = 0; i < count; ++i) {
        multiply_vector(m, &srcBase[i], &dstBase[i]);
    }
}

/* Rodrigues rotation about an arbitrary axis.  The SDK normalizes the axis
 * before building the matrix, so callers may pass an unnormalized direction. */
void PSMTXRotAxisRad(Mtx m, Vec* axis, f32 rad)
{
    Vec unit;
    const f32 sine = sinf(rad);
    const f32 cosine = cosf(rad);
    f32 complement;

    PSVECNormalize(axis, &unit);
    complement = 1.0F - cosine;

    m[0][0] = (unit.x * unit.x) * complement + cosine;
    m[0][1] = (unit.x * unit.y) * complement - (unit.z * sine);
    m[0][2] = (unit.x * unit.z) * complement + (unit.y * sine);
    m[0][3] = 0.0F;
    m[1][0] = (unit.y * unit.x) * complement + (unit.z * sine);
    m[1][1] = (unit.y * unit.y) * complement + cosine;
    m[1][2] = (unit.y * unit.z) * complement - (unit.x * sine);
    m[1][3] = 0.0F;
    m[2][0] = (unit.z * unit.x) * complement - (unit.y * sine);
    m[2][1] = (unit.z * unit.y) * complement + (unit.x * sine);
    m[2][2] = (unit.z * unit.z) * complement + cosine;
    m[2][3] = 0.0F;
}

/* ---------------------------------------------------------------------------
 * Projection and view matrices.
 *
 * MTXFrustum, MTXPerspective and MTXOrtho build 4x4 projections.  The SDK
 * header this port compiles against declares their destination as Mtx, which
 * decays to the same row pointer as Mtx44, so the caller is responsible for
 * providing four rows of storage.  Every caller inside the ported HSD camera
 * code passes an Mtx44.
 * ------------------------------------------------------------------------- */

void MTXFrustum(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    const f32 width_scale = 1.0F / (r - l);
    const f32 height_scale = 1.0F / (t - b);
    const f32 depth_scale = 1.0F / (f - n);

    m[0][0] = 2.0F * n * width_scale;
    m[0][1] = 0.0F;
    m[0][2] = (r + l) * width_scale;
    m[0][3] = 0.0F;
    m[1][0] = 0.0F;
    m[1][1] = 2.0F * n * height_scale;
    m[1][2] = (t + b) * height_scale;
    m[1][3] = 0.0F;
    m[2][0] = 0.0F;
    m[2][1] = 0.0F;
    m[2][2] = -n * depth_scale;
    m[2][3] = -(f * n) * depth_scale;
    m[3][0] = 0.0F;
    m[3][1] = 0.0F;
    m[3][2] = -1.0F;
    m[3][3] = 0.0F;
}

void MTXPerspective(Mtx m, f32 fovY, f32 aspect, f32 n, f32 f)
{
    const f32 half_angle = MTXDegToRad(fovY) * 0.5F;
    const f32 cotangent = cosf(half_angle) / sinf(half_angle);
    const f32 depth_scale = 1.0F / (f - n);

    m[0][0] = cotangent / aspect;
    m[0][1] = 0.0F;
    m[0][2] = 0.0F;
    m[0][3] = 0.0F;
    m[1][0] = 0.0F;
    m[1][1] = cotangent;
    m[1][2] = 0.0F;
    m[1][3] = 0.0F;
    m[2][0] = 0.0F;
    m[2][1] = 0.0F;
    m[2][2] = -n * depth_scale;
    m[2][3] = -(f * n) * depth_scale;
    m[3][0] = 0.0F;
    m[3][1] = 0.0F;
    m[3][2] = -1.0F;
    m[3][3] = 0.0F;
}

void MTXOrtho(Mtx m, f32 t, f32 b, f32 l, f32 r, f32 n, f32 f)
{
    const f32 width_scale = 1.0F / (r - l);
    const f32 height_scale = 1.0F / (t - b);
    const f32 depth_scale = 1.0F / (f - n);

    m[0][0] = 2.0F * width_scale;
    m[0][1] = 0.0F;
    m[0][2] = 0.0F;
    m[0][3] = -(r + l) * width_scale;
    m[1][0] = 0.0F;
    m[1][1] = 2.0F * height_scale;
    m[1][2] = 0.0F;
    m[1][3] = -(t + b) * height_scale;
    m[2][0] = 0.0F;
    m[2][1] = 0.0F;
    m[2][2] = -depth_scale;
    m[2][3] = -f * depth_scale;
    m[3][0] = 0.0F;
    m[3][1] = 0.0F;
    m[3][2] = 0.0F;
    m[3][3] = 1.0F;
}

/* Builds a view matrix whose +z axis points from the target back to the
 * camera, matching the SDK's right-handed convention. */
void C_MTXLookAt(Mtx m, Point3dPtr camPos, VecPtr camUp, Point3dPtr target)
{
    Vec look;
    Vec right;
    Vec up;

    PSVECSubtract(camPos, target, &look);
    PSVECNormalize(&look, &look);
    PSVECCrossProduct(camUp, &look, &right);
    PSVECNormalize(&right, &right);
    PSVECCrossProduct(&look, &right, &up);

    m[0][0] = right.x;
    m[0][1] = right.y;
    m[0][2] = right.z;
    m[0][3] = -(camPos->x * right.x + camPos->y * right.y +
                camPos->z * right.z);
    m[1][0] = up.x;
    m[1][1] = up.y;
    m[1][2] = up.z;
    m[1][3] = -(camPos->x * up.x + camPos->y * up.y + camPos->z * up.z);
    m[2][0] = look.x;
    m[2][1] = look.y;
    m[2][2] = look.z;
    m[2][3] = -(camPos->x * look.x + camPos->y * look.y +
                camPos->z * look.z);
}

void MTXRotRad(Mtx m, char axis, f32 rad)
{
    const f32 sine = sinf(rad);
    const f32 cosine = cosf(rad);

    PSMTXIdentity(m);
    switch (axis | 0x20) {
    case 'x':
        m[1][1] = cosine;
        m[1][2] = -sine;
        m[2][1] = sine;
        m[2][2] = cosine;
        break;
    case 'y':
        m[0][0] = cosine;
        m[0][2] = sine;
        m[2][0] = -sine;
        m[2][2] = cosine;
        break;
    case 'z':
        m[0][0] = cosine;
        m[0][1] = -sine;
        m[1][0] = sine;
        m[1][1] = cosine;
        break;
    default:
        break;
    }
}

void PSVECAdd(Vec* a, Vec* b, Vec* result)
{
    const f32 x = a->x + b->x;
    const f32 y = a->y + b->y;
    const f32 z = a->z + b->z;
    result->x = x;
    result->y = y;
    result->z = z;
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
