#include "hsd_include.hpp"
#include "test.hpp"

MELEE_HOST_TEST_HSD_BEGIN
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/spline.h>
MELEE_HOST_TEST_HSD_END

#include <array>
#include <cmath>

namespace {

bool near(f32 value, f32 expected, f32 tolerance = 1.0e-5F)
{
    return std::fabs(value - expected) <= tolerance;
}

bool vec_near(const Vec& value, f32 x, f32 y, f32 z, f32 tolerance = 1.0e-5F)
{
    return near(value.x, x, tolerance) && near(value.y, y, tolerance) &&
           near(value.z, z, tolerance);
}

} // namespace

TEST_CASE("affine inverse round-trips a scaled rotation with translation")
{
    Mtx source = { { 0.0F, -2.0F, 0.0F, 5.0F },
                   { 0.5F, 0.0F, 0.0F, -3.0F },
                   { 0.0F, 0.0F, 4.0F, 1.0F } };
    Mtx inverse;
    REQUIRE(PSMTXInverse(source, inverse) == 1);

    Mtx product;
    PSMTXConcat(source, inverse, product);
    REQUIRE(near(product[0][0], 1.0F));
    REQUIRE(near(product[1][1], 1.0F));
    REQUIRE(near(product[2][2], 1.0F));
    REQUIRE(near(product[0][1], 0.0F));
    REQUIRE(near(product[0][3], 0.0F));
    REQUIRE(near(product[1][3], 0.0F));
    REQUIRE(near(product[2][3], 0.0F));

    // A point pushed through the transform and back must land where it began.
    Vec point{ 2.0F, -1.0F, 7.0F };
    Vec moved;
    Vec restored;
    PSMTXMultVec(source, &point, &moved);
    PSMTXMultVec(inverse, &moved, &restored);
    REQUIRE(vec_near(restored, 2.0F, -1.0F, 7.0F));
}

TEST_CASE("affine inverse reports a singular rotation block")
{
    Mtx singular = { { 1.0F, 2.0F, 3.0F, 4.0F },
                     { 2.0F, 4.0F, 6.0F, 5.0F },
                     { 0.0F, 1.0F, 1.0F, 6.0F } };
    Mtx inverse = { { 9.0F, 9.0F, 9.0F, 9.0F },
                    { 9.0F, 9.0F, 9.0F, 9.0F },
                    { 9.0F, 9.0F, 9.0F, 9.0F } };
    REQUIRE(PSMTXInverse(singular, inverse) == 0);
    // The destination must be left untouched when the matrix cannot invert.
    REQUIRE(inverse[0][0] == 9.0F);
    REQUIRE(inverse[2][3] == 9.0F);
}

TEST_CASE("matrix inverse and transpose accept an aliased destination")
{
    Mtx matrix = { { 0.0F, -2.0F, 0.0F, 5.0F },
                   { 0.5F, 0.0F, 0.0F, -3.0F },
                   { 0.0F, 0.0F, 4.0F, 1.0F } };
    Mtx reference;
    REQUIRE(PSMTXInverse(matrix, reference) == 1);
    REQUIRE(PSMTXInverse(matrix, matrix) == 1);
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 4; ++column) {
            REQUIRE(near(matrix[row][column], reference[row][column]));
        }
    }

    Mtx transposed;
    PSMTXTranspose(reference, transposed);
    PSMTXTranspose(reference, reference);
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 4; ++column) {
            REQUIRE(near(reference[row][column], transposed[row][column]));
        }
    }
}

TEST_CASE("inverse transpose carries normals through a non-uniform scale")
{
    // Scaling x by 4 must squash the x component of a normal, not stretch it.
    Mtx scale;
    PSMTXScale(scale, 4.0F, 1.0F, 1.0F);
    Mtx normal_matrix;
    REQUIRE(PSMTXInvXpose(scale, normal_matrix) == 1);

    Vec normal{ 1.0F, 1.0F, 0.0F };
    Vec transformed;
    PSMTXMultVecSR(normal_matrix, &normal, &transformed);
    REQUIRE(vec_near(transformed, 0.25F, 1.0F, 0.0F));
    // Directions ignore translation, so the fourth column stays clear.
    REQUIRE(normal_matrix[0][3] == 0.0F);
    REQUIRE(normal_matrix[1][3] == 0.0F);
    REQUIRE(normal_matrix[2][3] == 0.0F);
}

TEST_CASE("vector multiply applies translation and the SR form skips it")
{
    Mtx transform = { { 1.0F, 0.0F, 0.0F, 10.0F },
                      { 0.0F, 1.0F, 0.0F, 20.0F },
                      { 0.0F, 0.0F, 1.0F, 30.0F } };
    Vec source{ 1.0F, 2.0F, 3.0F };
    Vec point;
    Vec direction;
    PSMTXMultVec(transform, &source, &point);
    PSMTXMultVecSR(transform, &source, &direction);
    REQUIRE(vec_near(point, 11.0F, 22.0F, 33.0F));
    REQUIRE(vec_near(direction, 1.0F, 2.0F, 3.0F));

    // The game multiplies vectors in place, so aliasing must be safe.
    Vec aliased{ 1.0F, 2.0F, 3.0F };
    PSMTXMultVec(transform, &aliased, &aliased);
    REQUIRE(vec_near(aliased, 11.0F, 22.0F, 33.0F));

    std::array<Vec, 2> sources{ Vec{ 1.0F, 0.0F, 0.0F },
                                Vec{ 0.0F, 1.0F, 0.0F } };
    std::array<Vec, 2> results{};
    PSMTXMultVecArray(transform, sources.data(), results.data(), 2);
    REQUIRE(vec_near(results[0], 11.0F, 20.0F, 30.0F));
    REQUIRE(vec_near(results[1], 10.0F, 21.0F, 30.0F));
}

TEST_CASE("axis rotation normalizes its axis and turns a quarter circle")
{
    // An unnormalized z axis must still produce a pure quarter turn.
    Vec axis{ 0.0F, 0.0F, 9.0F };
    Mtx rotation;
    PSMTXRotAxisRad(rotation, &axis, 1.5707963267948966F);

    Vec x_axis{ 1.0F, 0.0F, 0.0F };
    Vec rotated;
    PSMTXMultVecSR(rotation, &x_axis, &rotated);
    REQUIRE(vec_near(rotated, 0.0F, 1.0F, 0.0F, 1.0e-4F));
    REQUIRE(rotation[0][3] == 0.0F);
    REQUIRE(rotation[2][3] == 0.0F);

    // Rotating about an arbitrary axis leaves that axis fixed.
    Vec diagonal{ 1.0F, 1.0F, 1.0F };
    Mtx about_diagonal;
    PSMTXRotAxisRad(about_diagonal, &diagonal, 2.0F);
    Vec fixed;
    PSMTXMultVecSR(about_diagonal, &diagonal, &fixed);
    REQUIRE(vec_near(fixed, 1.0F, 1.0F, 1.0F, 1.0e-4F));
}

TEST_CASE("vector add keeps its result when the destination is an operand")
{
    Vec a{ 1.0F, 2.0F, 3.0F };
    Vec b{ 10.0F, 20.0F, 30.0F };
    Vec sum;
    PSVECAdd(&a, &b, &sum);
    REQUIRE(vec_near(sum, 11.0F, 22.0F, 33.0F));

    PSVECAdd(&a, &b, &a);
    REQUIRE(vec_near(a, 11.0F, 22.0F, 33.0F));
}

TEST_CASE("original spline evaluator interpolates and clamps its parameter")
{
    std::array<Vec3, 2> control{ Vec3{ 0.0F, 0.0F, 0.0F },
                                 Vec3{ 10.0F, -4.0F, 2.0F } };
    HSD_Spline spline{};
    spline.type = 0;
    spline.numcv = 2;
    spline.cv = control.data();

    Vec3 point{ 99.0F, 99.0F, 99.0F };
    splGetSplinePoint(&point, &spline, 0.0F);
    REQUIRE(vec_near(point, 0.0F, 0.0F, 0.0F));
    splGetSplinePoint(&point, &spline, 0.5F);
    REQUIRE(vec_near(point, 5.0F, -2.0F, 1.0F));
    splGetSplinePoint(&point, &spline, 1.0F);
    REQUIRE(vec_near(point, 10.0F, -4.0F, 2.0F));

    // Out-of-range parameters leave the destination alone.
    Vec3 untouched{ 7.0F, 7.0F, 7.0F };
    splGetSplinePoint(&untouched, &spline, -0.1F);
    REQUIRE(vec_near(untouched, 7.0F, 7.0F, 7.0F));
    splGetSplinePoint(&untouched, &spline, 1.1F);
    REQUIRE(vec_near(untouched, 7.0F, 7.0F, 7.0F));
}
