#ifndef CAMERA_H_
#define CAMERA_H_

#include <cglm/cglm.h>
#include <GLFW/glfw3.h>

/*
 * Returns a view matrix looking from <eye> to <center> in <dest>, with up as
 * (0, 1, 0).
 */
void cam_looker(vec3 eye, vec3 center, mat4 dest);

/*									\
 * Returns a projection matrix in dest correcting for the aspect ratio of the
 * given dimensions, with a hard-coded FOV and depth range.
 */
void cam_projector(uint32_t swidth, uint32_t sheight, mat4 dest);

/*
 * Outputs a direction vector into <dest> with magnitude one with the given
 * pitch and yaw.
 */
void cam_get_dir_vec(float yaw, float pitch, vec3 dest);

struct MouseTracker {
	double prev_x;
	double prev_y;
};

/*
 * Uses a MouseTracker to calculate, given a mouse position, the mouse movement.
 * Converts the movement to the range -1..1 for each axis, where 1 or -1 is the
 * mouse moving from one edge of the screen to the other.
 */
void cam_mouse_diff(struct MouseTracker *t,
		    double new_x, double new_y,
		    double *out_x, double *out_y);

struct OrbitCamera {
	struct MouseTracker tracker;
	float yaw;
	float pitch;
	float distance;
};

/*
 * Returns a new OrbitCamera.
 * x and y are the mouse's current position.
 */
struct OrbitCamera cam_orbit_new(double x, double y);

/*
 * Outputs a combined view and projection matrix into dest, given screen
 * dimensions and the current mouse coordinates.
 */
void cam_orbit_mat(struct OrbitCamera *c,
		   uint32_t swidth, uint32_t sheight,
		   double x, double y,
		   mat4 dest);

struct FlyCamera {
	struct MouseTracker tracker;
	float yaw;
	float pitch;
	vec3 pos;
	float speed;
};

/*
 * Returns a new FlyCamera.
 *
 * cx, cy, cz: Camera position
 * yaw, pitch: Direction camera initially points towards
 * mx, my: Mouse coordinates
 */
struct FlyCamera cam_fly_new(double cx, double cy, double cz,
			     float yaw, float pitch,
			     double mx, double my);

/*
 * Outputs a combined view and projection matrix into dest, given screen
 * dimensions and the current mouse coordinates.
 *
 * Unlike cam_orbit_mat, does not also update the camera angle based on mouse
 * movement.
 */
void cam_fly_mat(struct FlyCamera *c,
		 uint32_t swidth, uint32_t sheight,
		 mat4 dest);

/*
 * Updates a FlyCamera using mouse and keyboard input.
 *
 * gwin: GLFW window to query for keyboard input
 * mx, my: Mouse coordinates
 * delta: Frame delta, used to scale movement
 */
void cam_fly_update(struct FlyCamera *c,
		    GLFWwindow *gwin,
		    double mx, double my,
		    double delta);

#endif // CAMERA_H_
