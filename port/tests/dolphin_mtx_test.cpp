#include "test.hpp"

extern "C" {
#include <dolphin/mtx.h>
void PSMTXTrans(Mtx matrix, f32 x, f32 y, f32 z);
}

#include <cmath>

namespace {

bool near(float actual, float expected)
{
    return std::fabs(actual - expected) < 0.0001F;
}

} // namespace

TEST_CASE("portable paired-single vector operations preserve aliases")
{
    Vec vector{ 3.0F, 4.0F, 0.0F };
    REQUIRE(near(PSVECMag(&vector), 5.0F));
    PSVECNormalize(&vector, &vector);
    REQUIRE(near(vector.x, 0.6F));
    REQUIRE(near(vector.y, 0.8F));
    REQUIRE(near(vector.z, 0.0F));

    Vec other{ 0.0F, 0.0F, 2.0F };
    PSVECCrossProduct(&vector, &other, &vector);
    REQUIRE(near(vector.x, 1.6F));
    REQUIRE(near(vector.y, -1.2F));
    REQUIRE(near(vector.z, 0.0F));
    REQUIRE(near(PSVECDotProduct(&vector, &other), 0.0F));
}

TEST_CASE("portable paired-single matrices compose in place")
{
    Mtx translation;
    Mtx scale;
    PSMTXTrans(translation, 10.0F, 20.0F, 30.0F);
    PSMTXScale(scale, 2.0F, 3.0F, 4.0F);
    PSMTXConcat(translation, scale, translation);

    REQUIRE(near(translation[0][0], 2.0F));
    REQUIRE(near(translation[1][1], 3.0F));
    REQUIRE(near(translation[2][2], 4.0F));
    REQUIRE(near(translation[0][3], 10.0F));
    REQUIRE(near(translation[1][3], 20.0F));
    REQUIRE(near(translation[2][3], 30.0F));

    Quaternion identity{ 0.0F, 0.0F, 0.0F, 1.0F };
    PSMTXQuat(scale, &identity);
    REQUIRE(near(scale[0][0], 1.0F));
    REQUIRE(near(scale[1][1], 1.0F));
    REQUIRE(near(scale[2][2], 1.0F));
}
