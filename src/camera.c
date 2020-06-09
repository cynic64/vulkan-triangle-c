#include "camera.h"

#include <math.h>
#include <assert.h>

/* Helper function to avoid yaw angles of <0 or >2pi */
void wrap(float min, float max, float *val);

/* Helper function to constrain pitch angles to -0.5pi..0.5pi */
void clamp(float min, float max, float *val);

void cam_looker(vec3 eye, vec3 center, mat4 dest)
{
	glm_lookat(eye, center, (vec3){0.0f, 1.0f, 0.0f}, dest);
}

void cam_projector(uint32_t swidth, uint32_t sheight, mat4 dest)
{
	float aspect = (float) swidth / (float) sheight;
	float fov = M_PI * 0.25;
	glm_perspective(fov, aspect, 1.0f, 100.0f, dest);

	// Flip vertically (GLM and Vulkan don't match up in this regard)
	glm_scale(dest, (vec3){1.0, -1.0f, 1.0f});
}

void cam_mouse_diff(struct MouseTracker *t,
		    double new_x, double new_y,
		    double *out_x, double *out_y)
{
	float scale_factor = 0.001;

	double x_diff = new_x - t->prev_x;
	double y_diff = new_y - t->prev_y;

	*out_x = x_diff * scale_factor;
	*out_y = y_diff * scale_factor;

	t->prev_x = new_x;
	t->prev_y = new_y;
}

struct OrbitCamera cam_orbit_new(double x, double y)
{
	struct OrbitCamera self = {0};
	self.tracker.prev_x = x;
	self.tracker.prev_y = y;

	return self;
}

void cam_orbit_mat(struct OrbitCamera *c,
		   uint32_t swidth, uint32_t sheight,
		   double x, double y,
		   mat4 dest)
{
	float distance = 16.0f;

	// Update pitch and yaw
	double x_diff, y_diff;
	cam_mouse_diff(&c->tracker, x, y, &x_diff, &y_diff);
	c->yaw += x_diff;
	c->pitch += y_diff;
	wrap(-M_PI, M_PI, &c->yaw);
	clamp(-M_PI * 0.5f, M_PI * 0.5f, &c->pitch);

	// View matrix
	vec3 dir;
	cam_get_dir_vec(c->yaw, c->pitch, dir);
	dir[0] *= distance;
	dir[1] *= distance;
	dir[2] *= distance;

	vec3 center = {0.0f, 0.0f, 0.0f};
	mat4 view;
	cam_looker(dir, center, view);

	// Projection matrix
	mat4 proj;
	cam_projector(swidth, sheight, proj);

	// Multiply
	glm_mat4_mul(proj, view, dest);
}

void cam_get_dir_vec(float yaw, float pitch, vec3 dest)
{
	dest[0] = cos(yaw) * cos(pitch);
	dest[1] = sin(pitch);
	dest[2] = sin(yaw) * cos(pitch);
}

void wrap(float min, float max, float *val)
{
	assert(max > min);

	*val -= min;
	*val = fmodf(*val, (max - min));
	*val += min;
}

void clamp(float min, float max, float *val)
{
	assert(max > min);

	if (*val > max) *val = max;
	else if (*val < min) *val = min;
}
