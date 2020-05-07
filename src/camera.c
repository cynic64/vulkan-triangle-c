#include "camera.h"

#include <math.h>

void cam_looker(vec3 eye, vec3 center, mat4 dest) {
    glm_lookat(eye, center, (vec3){0.0f, 1.0f, 0.0f}, dest);
}

void cam_projector(uint32_t swidth, uint32_t sheight, mat4 dest) {
    float aspect = (float) swidth / (float) sheight;
    float fov = M_PI * 0.25;
    glm_perspective(fov, aspect, -1.0f, 1.0f, dest);
}

void cam_mouse_diff(
    struct MouseTracker *t,
    uint32_t swidth, uint32_t sheight,
    double new_x, double new_y,
    double *out_x, double *out_y
) {
    double fw = swidth;
    double fh = sheight;

    double x_diff = new_x - t->prev_x;
    double y_diff = new_y - t->prev_y;

    *out_x = x_diff / fw;
    *out_y = y_diff / fh;

    t->prev_x = new_x;
    t->prev_y = new_y;
}

struct OrbitCamera cam_orbit_new(double x, double y) {
    struct OrbitCamera self = {0};
    self.tracker.prev_x = x;
    self.tracker.prev_y = y;

    return self;
}

void cam_orbit_mat(
    struct OrbitCamera *c,
    uint32_t swidth, uint32_t sheight,
    double x, double y,
    mat4 dest
) {
    // update pitch and yaw
    double x_diff, y_diff;
    cam_mouse_diff(&c->tracker, swidth, sheight, x, y, &x_diff, &y_diff);
    c->yaw += x_diff;
    c->pitch += y_diff;

    // view matrix
    vec3 dir;
    cam_get_dir_vec(c->yaw, c->pitch, dir);

    vec3 center = {0.0f, 0.0f, 0.0f};
    mat4 view;
    cam_looker(dir, center, view);

    // projection matrix
    mat4 proj;
    cam_projector(swidth, sheight, proj);

    // multiply
    glm_mat4_mul(proj, view, dest);
}

void cam_get_dir_vec(float yaw, float pitch, vec3 dest) {
    float radius = 4.0;

    dest[0] = cos(yaw) * cos(pitch) * radius;
    dest[1] = sin(pitch) * radius;
    dest[2] = sin(yaw) * cos(pitch) * radius;
}
