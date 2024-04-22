#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "hittable.hpp"
#include "rtweekend.hpp"

class sphere : public hittable {
   public:
    sphere(const point3& center, double radius, shared_ptr<material> mat) : center(center), radius(fmax(0, radius)), mat(mat) {}

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // std::clog << ray_tmin << " " << ray_tmax << "**\n";
        vec3 oc = center - r.origin();
        double a = r.direction().length_squared();
        auto h = dot(r.direction(), oc);
        auto c = oc.length_squared() - radius * radius;
        double discriminant = h * h - a * c;
        if (discriminant < 0.0)
            return false;

        double sqrtd = sqrt(discriminant);
        double root = (h - sqrtd) / a;
        if (!ray_t.surrounds(root)) {
            root = (h + sqrtd) / a;
            if (!ray_t.surrounds(root)) {
                return false;
            }
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.mat = mat;

        return true;
    }

   private:
    point3 center;
    double radius;
    shared_ptr<material> mat;
};

#endif