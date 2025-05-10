#ifndef SPHERE_H_
#define SPHERE_H_

#include "hittable.h"

class sphere : public hittable {
 public:
  sphere(const point3& center, double radius) : center(center), radius(std::fmax(0, radius)) {}

  bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
      
    vec3 oc = center - r.origin();
    auto a = r.direction().length_squared();
    auto h = dot(r.direction(), oc);
    auto c = oc.length_squared() - radius*radius;
    auto discriminant = h*h - a*c;

    if (discriminant < 0) return false;

    auto sqrtd = std::sqrt(discriminant);

    // Find the nearest root that lies in the acceptable range
    auto root = (h-sqrtd) / a;
    if (!ray_t.surrounds(root)) {
      root = (h + sqrtd) / a;
      if (!ray_t.surrounds(root)) return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    rec.normal = (rec.p - center) / radius;

    return true;
  }

 private:
  point3 center;
  double radius;
};

#endif
