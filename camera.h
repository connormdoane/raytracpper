#ifndef CAMERA_H_
#define CAMERA_H_

#include "hittable.h"
#include "material.h"

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

class camera {
 public:
  double aspect_ratio = 16.0 / 9.0;
  int image_width = 400;
  int samples_per_pixel = 10;
  int max_depth = 10;

  double vfov = 90;
  point3 lookfrom = point3(0, 0, 0);
  point3 lookat = point3(0,0,-1);
  vec3 vup = vec3(0,1,0);

  double defocus_angle = 0;
  double focus_dist = 10;

  void render(const hittable& world) {
    initialize();

    std::vector<color> framebuffer(image_width * image_height);

    std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

    int thread_count = std::thread::hardware_concurrency();
    int rows_per_thread = image_height / thread_count;
    sections_remaining = thread_count;

    std::vector<std::thread> threads;

    for (int t = 0; t < thread_count; t++) {
      int start_row = t * rows_per_thread;
      int end_row = (t==thread_count-1) ? image_height : start_row + rows_per_thread;

      threads.emplace_back([this, &world, &framebuffer, start_row, end_row]() {
        this->render_section(world, framebuffer, start_row, end_row);
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    for (int j = 0; j < image_height; ++j) {
      for (int i = 0; i < image_width; ++i) {
        int index = j * image_width + i;
        write_color(std::cout, pixel_samples_scale * framebuffer[index]);
      }
    }

    std::clog << "\rDone.                             \n";
  }

 private:
  int image_height;
  double pixel_samples_scale;
  point3 center;
  point3 pixel00_loc;
  vec3 pixel_delta_u;
  vec3 pixel_delta_v;
  vec3 u, v, w;
  vec3 defocus_disk_u;
  vec3 defocus_disk_v;
  std::mutex output_mutex;
  std::atomic<int> sections_remaining;
  
  void initialize() {
    image_height = int(image_width / aspect_ratio);
    image_height = (image_height < 1) ? 1 : image_height;

    pixel_samples_scale = 1.0 / samples_per_pixel;

    center = lookfrom;

    // auto focal_length = (lookfrom-lookat).length();
    auto theta = degrees_to_radians(vfov);
    auto h = std::tan(theta/2);
    auto viewport_height = 2 * h * focus_dist;
    auto viewport_width = viewport_height * (double(image_width)/image_height);

    w = unit_vector(lookfrom - lookat);
    u = unit_vector(cross(vup, w));
    v = cross(w, u);

    vec3 viewport_u = viewport_width * u;
    vec3 viewport_v = viewport_height * -v;

    pixel_delta_u = viewport_u / image_width;
    pixel_delta_v = viewport_v / image_height;

    auto viewport_upper_left = center - (focus_dist * w) - viewport_u/2 - viewport_v/2;
    pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

    auto defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2));
    defocus_disk_u = u*defocus_radius;
    defocus_disk_v = v*defocus_radius;
  }

  void render_section(const hittable& world, std::vector<vec3>& framebuffer, int start_row, int end_row) {
    for (int j = start_row; j < end_row; j++) {
      for (int i = 0; i < image_width; i++) {
        color pixel_color(0,0,0);

        for (int sample = 0; sample < samples_per_pixel; sample++) {
          ray r = get_ray(i,j);
          pixel_color += ray_color(r, max_depth, world);
        }

        int index = j * image_width + i;
        framebuffer[index] = pixel_color;
      }
    }

    int remaining = --sections_remaining;
    std::clog << "\rSections Remaining: " << remaining << ' ' << std::flush;
  }

  ray get_ray(int i, int j) const {
    auto offset = sample_square();
    auto pixel_sample = pixel00_loc + ((i + offset.x()) * pixel_delta_u) + ((j + offset.y()) * pixel_delta_v);
    auto ray_origin = (defocus_angle <= 0) ? center : defocus_disk_sample();
    auto ray_direction = pixel_sample - ray_origin;
    return ray(ray_origin, ray_direction);
  }

  vec3 sample_square() const {
    return vec3(random_double() - 0.5, random_double() - 0.5, 0);
  }

  point3 defocus_disk_sample() const {
    auto p = random_in_unit_disk();
    return center + (p[0] * defocus_disk_u) + (p[1] * defocus_disk_v);
  }

  color ray_color(const ray& r, int depth, const hittable& world) const {
    if (depth <= 0) return color(0, 0, 0);

    hit_record rec;

    if (world.hit(r, interval(0.001, infinity), rec)) {
      ray scattered;
      color attenuation;
      if (rec.mat->scatter(r, rec, attenuation, scattered)) return attenuation * ray_color(scattered, depth-1, world);
      return color(0, 0, 0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto a = 0.5*(unit_direction.y() + 1.0);
    return (1.0-a)*color(1.0,1.0,1.0) + a*color(0.5, 0.7, 1.0);
  }
};

#endif
