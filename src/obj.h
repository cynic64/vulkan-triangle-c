#ifndef OBJ_H_
#define OBJ_H_

#include <stddef.h>
#include <stdio.h>
#include <inttypes.h>

struct Vertex3PosNormal;

/* Reads the OBJ file at fp. Panics on any kind of failure.

   If [vbuf] or [ibuf] are NULL, will only output to [vertex_ct] and [index_ct].

   Reads positions and normals.

   Otherwise, outputs to both with no safety checks as to whether they are large
   enough. */
void load_obj(FILE *fp,
              size_t *vertex_ct, size_t *index_ct,
	      struct Vertex3PosNormal *vertices, uint32_t *indices);

#endif // OBJ_H_
