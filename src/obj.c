#include "obj.h"
#include "ll_obj.h"
#include "vk_tools.h"
#include "vk_vertex.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void obj_load(FILE *fp,
	      size_t *vertex_ct, size_t *index_ct,
	      struct ObjVertex *vertices, uint32_t *indices)
{
	assert(fp != NULL);
	rewind(fp);

	// Do a first pass, counting positions, normals and indices
	*index_ct = 0;
	*vertex_ct = 0;
	// The user doesn't care how many normals there are, but it's necessary to
	// allocate a separate buffer for them and therefore to know how many there
	// are
	size_t normal_ct = 0;

	char line[256];

	while (fgets(line, sizeof(line), fp) != NULL) {
		// Strip newline
		line[strlen(line) - 1] = '\0';

		if (strncmp(line, "v ", 2) == 0) (*vertex_ct)++;
		else if (strncmp(line, "vn ", 3) == 0) normal_ct++;
		else if (strncmp(line, "f ", 2) == 0) *index_ct += 3;
	}

	// If [vertices] or [indices] are NULL, counting is all we had to do
	if (vertices == NULL || indices == NULL) return;

	// Otherwise, do the real loading

	// Positions and indices can be written directly to [vertices] and [indices],
	// but we need to give each vertex a normal and we don't yet know who gets
	// what, so we have to store normals separately until later use
	float (*normals)[3] = malloc(sizeof(normals[0]) * normal_ct);

	// Now read positions, normal, and face data
	// Faces must come after positions and normal data, because if we find a
	// face to parse we assume the positions/normals it references are already
	// stored in [positions] or [normals]
	rewind(fp);
	size_t pos_idx = 0;
	size_t normal_idx = 0;
	size_t index_idx = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		// Strip newline
		line[strlen(line) - 1] = '\0';

		if (strncmp(line, "v ", 2) == 0) {
			// Write directly to the output, since the order positions arrive in
			// is the order they will be stored in
			parse_triplet(line, vertices[pos_idx++].pos);
		} else if (strncmp(line, "vn ", 3) == 0) {
			// Save normals in an intermediate buffer because we don't yet know
			// which vertex they belong to
			parse_triplet(line, normals[normal_idx++]);
		} else if (strncmp(line, "f ", 2) == 0) {
			size_t f_pos_idxs[3];
			size_t f_norm_idxs[3];
			parse_face(line, f_pos_idxs, f_norm_idxs);

			// Assign normals to positions
			copy_float3(vertices[f_pos_idxs[0] - 1].normal, normals[f_norm_idxs[0] - 1]);
			copy_float3(vertices[f_pos_idxs[1] - 1].normal, normals[f_norm_idxs[1] - 1]);
			copy_float3(vertices[f_pos_idxs[2] - 1].normal, normals[f_norm_idxs[2] - 1]);

			// Write indices
			indices[index_idx++] = f_pos_idxs[0] - 1;
			indices[index_idx++] = f_pos_idxs[1] - 1;
			indices[index_idx++] = f_pos_idxs[2] - 1;
		}
	}

	free(normals);
}

void parse_triplet(char *str, float out[3])
{
	char *token;
	char *rest = strdup(str);

	// Discard the first token (because we don't care about the v/vn/vt/whatever
	// part)
	assert(strtok_r(rest, " ", &rest) != NULL);

	size_t idx = 0;
	while ((token = strtok_r(rest, " ", &rest)) != NULL) {
		out[idx++] = strtof(token, NULL);
		if (idx >= 3) break;
	}
}

void copy_float3(float dest[3], float src[3])
{
	dest[0] = src[0];
	dest[1] = src[1];
	dest[2] = src[2];
}

void parse_face(char *str, size_t pos_idxs[3], size_t normal_idxs[3])
{
	char *token;
	char *rest = strdup(str);

	// Discard the first token ("f ")
	assert(strtok_r(rest, " ", &rest) != NULL);

	size_t idx = 0;
	while ((token = strtok_r(rest, " ", &rest)) != NULL) {
		char *subrest = strdup(token);

		// position of vertex
		char *subtoken = strtok_r(subrest, "/", &subrest);
		pos_idxs[idx] = atoi(subtoken);
		// skip texture coord
		strtok_r(subrest, "/", &subrest);
		// normal of first vertex
		subtoken = strtok_r(subrest, "/", &subrest);
		normal_idxs[idx] = atoi(subtoken);

		idx++;

		if (idx >= 3) break;
	}
}

void obj_vertex_to_vertex_3_pos_normal_list(struct Vertex3PosNormal *dest,
					    struct ObjVertex *src,
					    size_t ct)
{
	for (int i = 0; i < ct; i++) {
		obj_vertex_to_vertex_3_pos_normal(&dest[i], &src[i]);
	}
}

void obj_vertex_to_vertex_3_pos_normal(struct Vertex3PosNormal *dest,
				       struct ObjVertex *src)
{
	memcpy(dest->pos, src->pos, sizeof(src->pos));
	memcpy(dest->normal, src->normal, sizeof(src->normal));
}
