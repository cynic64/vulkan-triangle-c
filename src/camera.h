#ifndef CAMERA_H_
#define CAMERA_H_

#include <cglm/cglm.h>

// Returns a view matrix looking from <eye> to <center> in <dest>, with up as
// (0, 1, 0)
void cam_looker(vec3 eye, vec3 center, mat4 dest);

// Returns a projection matrix in dest correcting for the aspect ratio of the
// given dimensions, with a hard-coded FOV and depth range
void cam_projector(uint32_t swidth, uint32_t sheight, mat4 dest);

struct MouseTracker {
    double prev_x;
    double prev_y;
};

// Uses a MouseTracker to calculate, given a mouse position, the mouse movement.
// Converts the movement to the range -1..1 for each axis, where 1 or -1 is the
// mouse moving from one edge of the screen to the other.
void cam_mouse_diff(
    struct MouseTracker *t,
    double new_x, double new_y,
    double *out_x, double *out_y
);

struct OrbitCamera {
    struct MouseTracker tracker;
    float yaw;
    float pitch;
};

// Returns a new OrbitCamera.
// x and y are the mouse's current position
struct OrbitCamera cam_orbit_new(double x, double y);

// Stores a combined view and projection matrix into dest, given screen
// dimensions and the current mouse coordinates
void cam_orbit_mat(
    struct OrbitCamera *c,
    uint32_t swidth, uint32_t sheight,
    double x, double y,
    mat4 dest
);

// Outputs a direction vector into <dest> with magnitude one with the given
// pitch and yaw.
void cam_get_dir_vec(float yaw, float pitch, vec3 dest);

#endif // CAMERA_H_
