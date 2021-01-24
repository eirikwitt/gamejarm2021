#include <stdint.h>

typedef struct {
	Vec pos;
	Vec vel;
	uint16_t sprite_frame;
	uint16_t sprite_animation;
} EntityState;

typedef struct {
	EntityState state;
	signed t;
	Vec acc;
} Keyframe;

typedef struct KeyframeChunk KeyframeChunk;

/* a bidirectionally navigable reference to a keyframe */
typedef struct {
	KeyframeChunk *chunk;
	unsigned index;
} Trail;

Keyframe *trail_find(Trail *self, int t);
Keyframe *trail_add(Trail *iter);
