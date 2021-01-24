#include <assert.h>

#include "util.h"
#include "trail.h"

enum {
	KEYFRAMECHUNK_LEN = 32,
};

struct KeyframeChunk {
	KeyframeChunk *next, *prev;
	unsigned len;
	Keyframe frames[];
};

static void trail_next(Trail *self) {
	if (++self->index >= self->chunk->len) {
		self->chunk = self->chunk->next;
		self->index = 0;
	}
}

static void trail_prev(Trail *self) {
	if (self->index <= 0) {
		self->chunk = self->chunk->prev;
		self->index = self->chunk->len;
	}
	self->index--;
}

static Keyframe *trail_get(const Trail *self) {
	return &self->chunk->frames[self->index];
}

Keyframe *trail_search(Trail *self, int t) {
	while (t >= trail_get(self)->t) trail_next(self);
	while (t < trail_get(self)->t) trail_prev(self);
	return trail_get(self);
}

Keyframe *trail_add(Trail *self) {
	KeyframeChunk *prev = self->chunk;
	trail_next(self);
	if (!self->chunk) {
		self->chunk = malloc(
			sizeof(KeyframeChunk) + sizeof(Keyframe) * KEYFRAMECHUNK_LEN
		);
		assert(self->chunk);
		*self->chunk = (KeyframeChunk){
			.prev = prev,
			.len = KEYFRAMECHUNK_LEN
		};
	}
	return trail_get(self);
}
