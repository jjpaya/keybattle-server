#include "color.hpp"

std::uint8_t clamp(float v) { //define a function to bound and round the input float value to 0-255
    if (v < 0)
        return 0;
    if (v > 255)
        return 255;
    return (std::uint8_t)v;
}

RGB_u rgb_hue_rotate(RGB_u in, const float fHue) {
    RGB_u out;
    const float cosA = cos(fHue*3.14159265f/180); //convert degrees to radians
    const float sinA = sin(fHue*3.14159265f/180); //convert degrees to radians
    //calculate the rotation matrix, only depends on Hue
    float matrix[3][3] = {{cosA + (1.0f - cosA) / 3.0f, 1.0f/3.0f * (1.0f - cosA) - sqrtf(1.0f/3.0f) * sinA, 1.0f/3.0f * (1.0f - cosA) + sqrtf(1.0f/3.0f) * sinA},
        {1.0f/3.0f * (1.0f - cosA) + sqrtf(1.0f/3.0f) * sinA, cosA + 1.0f/3.0f*(1.0f - cosA), 1.0f/3.0f * (1.0f - cosA) - sqrtf(1.0f/3.0f) * sinA},
        {1.0f/3.0f * (1.0f - cosA) - sqrtf(1.0f/3.0f) * sinA, 1.0f/3.0f * (1.0f - cosA) + sqrtf(1.0f/3.0f) * sinA, cosA + 1.0f/3.0f * (1.0f - cosA)}};
    //Use the rotation matrix to convert the RGB directly
    out.v.a = in.v.a;
    out.v.r = clamp(in.v.r*matrix[0][0] + in.v.g*matrix[0][1] + in.v.b*matrix[0][2]);
    out.v.g = clamp(in.v.r*matrix[1][0] + in.v.g*matrix[1][1] + in.v.b*matrix[1][2]);
    out.v.b = clamp(in.v.r*matrix[2][0] + in.v.g*matrix[2][1] + in.v.b*matrix[2][2]);
    return out;
}
