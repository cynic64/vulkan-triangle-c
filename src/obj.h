#ifndef OBJ_H_
#define OBJ_H_

#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>

#include "vk_vertex.h"

/* Type OBJ files are read into */
struct ObjVertex {
	float pos[3];
	float normal[3];
};

/* Reads the OBJ file at fp. Panics on any kind of failure.

   If [vbuf] or [ibuf] are NULL, will only output to [vertex_ct] and [index_ct].

   Reads positions and normals.

   Otherwise, outputs to both with no safety checks as to whether they are large
   enough. */
void obj_load(FILE *fp,
              size_t *vertex_ct, size_t *index_ct,
	      struct ObjVertex *vertices, uint32_t *indices);

/* Converts arrays of ObjVertex into Vertex3PosNormal */

#endif // OBJ_H_
