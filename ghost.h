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

typedef struct {
	KeyframeChunk *chunk;
	unsigned index;
} KeyframeIter;

typedef struct {
	EntityState state;
	KeyframeIter trail;
} Player;

typedef struct {
	EntityState state;
	KeyframeIter trail;
	signed t;
	Ghost *next;
} Ghost;


Keyframe *keyframe_find(KeyframeIter *iter, int t);

void ghost_tick(Ghost *self, int delta);
