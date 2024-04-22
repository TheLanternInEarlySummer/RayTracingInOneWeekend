#ifndef COLOR_HPP
#define COLOR_HPP

#include "interval.hpp"
#include "vec3.hpp"

using color = vec3;

inline double linear_to_gamma(double linear_component) {
    if (linear_component > 0)
        return sqrt(linear_component);
    return 0;
}

void write_color(std::ostream& out, const color& pixel_color) {
    auto r = pixel_color.x();
    auto g = pixel_color.y();
    auto b = pixel_color.z();

    // the inverse of gamma correction
    r = linear_to_gamma(r);
    g = linear_to_gamma(g);
    b = linear_to_gamma(b);

    static const interval intensity(0.000, 0.999);
    int rbyte = int(255.999 * intensity.clamp(r));
    int gbyte = int(255.999 * intensity.clamp(g));
    int bbyte = int(255.999 * intensity.clamp(b));

    out << rbyte << " " << gbyte << " " << bbyte << "\n";
}

#endif