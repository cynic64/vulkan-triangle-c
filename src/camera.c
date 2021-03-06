#include "camera.h"

#include <math.h>
#include <assert.h>

#define DEFAULT_ORBIT_DISTANCE 8.0
#define WORLD_UP (vec3){0.0f, 1.0f, 0.0f}

/* Helper function to avoid yaw angles of <0 or >2pi */
void wrap(float min, float max, float *val);

/* Helper function to constrain pitch angles to -0.5pi..0.5pi */
void clamp(float min, float max, float *val);

void cam_looker(vec3 eye, vec3 center, mat4 dest)
{
	glm_lookat(eye, center, WORLD_UP, dest);
}

void cam_projector(uint32_t swidth, uint32_t sheight, mat4 dest)
{
	float aspect = (float) swidth / (float) sheight;
	float fov = M_PI * 0.25;
	glm_perspective(fov, aspect, 1.0f, 1024.0f, dest);

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

struct OrbitCamera cam_orbit_new(double x, double y)
{
	struct OrbitCamera self = {0};
	self.tracker.prev_x = x;
	self.tracker.prev_y = y;
	self.distance = DEFAULT_ORBIT_DISTANCE;

	return self;
}

void cam_orbit_mat(struct OrbitCamera *c,
		   uint32_t swidth, uint32_t sheight,
		   double x, double y,
		   mat4 dest)
{
	// Update pitch and yaw
	double x_diff, y_diff;
	cam_mouse_diff(&c->tracker, x, y, &x_diff, &y_diff);
	c->yaw += x_diff;
	c->pitch += y_diff;
	wrap(-M_PI, M_PI, &c->yaw);
	clamp(-M_PI * 0.49f, M_PI * 0.49f, &c->pitch);

	// View matrix
	vec3 dir;
	cam_get_dir_vec(c->yaw, c->pitch, dir);
	dir[0] *= c->distance;
	dir[1] *= c->distance;
	dir[2] *= c->distance;

	vec3 center = {0.0f, 0.0f, 0.0f};
	mat4 view;
	cam_looker(dir, center, view);

	// Projection matrix
	mat4 proj;
	cam_projector(swidth, sheight, proj);

	// Multiply
	glm_mat4_mul(proj, view, dest);
}

struct FlyCamera cam_fly_new(double cx, double cy, double cz,
			     float yaw, float pitch,
			     double mx, double my)
{
	struct FlyCamera c = {{.prev_x = mx, .prev_y = my},
			      .yaw = yaw, .pitch = pitch,
			      {cx, cy, cz},
			      .speed = 16.0f};

	return c;
}

void cam_fly_mat(struct FlyCamera *c,
		 uint32_t swidth, uint32_t sheight,
		 mat4 dest)
{
	// View matrix
	vec3 dir;
	cam_get_dir_vec(c->yaw, c->pitch, dir);

	vec3 to;
	glm_vec3_add(c->pos, dir, to);

	mat4 view;
	cam_looker(c->pos, to, view);

	// Projection matrix
	mat4 proj;
	cam_projector(swidth, sheight, proj);

	// Multiply
	glm_mat4_mul(proj, view, dest);
}

void cam_fly_update(struct FlyCamera *c,
		    GLFWwindow *gwin,
		    double mx, double my,
		    double delta)
{
	// Update pitch and yaw
	double x_diff, y_diff;
	cam_mouse_diff(&c->tracker, mx, my, &x_diff, &y_diff);
	c->yaw += x_diff;
	c->pitch -= y_diff;
	wrap(-M_PI, M_PI, &c->yaw);
	clamp(-M_PI * 0.49f, M_PI * 0.49f, &c->pitch);

	vec3 forward;
	cam_get_dir_vec(c->yaw, c->pitch, forward);

	vec3 right;
	glm_vec3_cross(WORLD_UP, forward, right);
	glm_vec3_normalize(right);

	// Update position
	if (glfwGetKey(gwin, GLFW_KEY_W) == GLFW_PRESS) {
		c->pos[0] += forward[0] * delta * c->speed;
		c->pos[1] += forward[1] * delta * c->speed;
		c->pos[2] += forward[2] * delta * c->speed;
	}

	if (glfwGetKey(gwin, GLFW_KEY_S) == GLFW_PRESS) {
		c->pos[0] -= forward[0] * delta * c->speed;
		c->pos[1] -= forward[1] * delta * c->speed;
		c->pos[2] -= forward[2] * delta * c->speed;
	}

	if (glfwGetKey(gwin, GLFW_KEY_A) == GLFW_PRESS) {
		c->pos[0] += right[0] * delta * c->speed;
		c->pos[1] += right[1] * delta * c->speed;
		c->pos[2] += right[2] * delta * c->speed;
	}

	if (glfwGetKey(gwin, GLFW_KEY_D) == GLFW_PRESS) {
		c->pos[0] -= right[0] * delta * c->speed;
		c->pos[1] -= right[1] * delta * c->speed;
		c->pos[2] -= right[2] * delta * c->speed;
	}
}
