/*
 * I don't want to publicly expose the helper functions in obj.c, but I still
 * want to be able to unit test them, so this is my solution: having a separate
 * header file for the helper functions that be included separately.
 *
 * Whether this is a good idea or not, I'm not sure.
 */



/*
 * Helper function to parse positions/normals/whatever groups of 3 in OBJ files,
 * like these:
 *
 * v -1.000000 0.000000 1.000000
 *
 * Discards the first word and converts the remaining 3 to floats, outputting to
 * out.
 *
 * Aborts or segfaults on any kind of failure.
 */
void parse_triplet(char *str, float out[3]);

/*
 * Helper function to parse faces in OBJ files, which look like this:
 * 
 * f 2/1/1 3/2/1 1/3/1
 *
 * The first number in each group of 3 is the position index, then texture
 * coordinate (which we discard), then normal.
 *
 * Aborts or segfaults on any kind of failure.
 */
void parse_face(char *str, size_t pos_idxs[3], size_t normal_idxs[3]);

/*
 * Copies src to dest.
 */
void copy_float3(float dest[3], float src[3]);

