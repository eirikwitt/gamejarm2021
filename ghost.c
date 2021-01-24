#include "ghost.h"

enum {
	KEYFRAMECHUNK_LEN = 32;
};

struct {
	KeyframeChunk *next, *prev;
	unsigned len;
	Keyframe frames[];
} KeyframeChunk;

static void keyframe_next(KeyframeIter *iter) {
	if (++iter.index >= iter.chunk->len) {
		iter.chunk = iter.chunk->next;
		iter.index = 0;
	}
}

static void keyframe_prev(KeyframeIter *iter) {
	if (iter.index <= 0) {
		iter.chunk = iter.chunk->prev;
		iter.index = iter.chunk->len;
	}
	iter.index--;
}

static Keyframe *keyframe_get(const KeyframeIter *iter) {
	return &iter->chunk->frames[iter->index];
}

Keyframe *keyframe_search(KeyframeIter *iter, int t) {
	while (t >= keyframe_get(iter)->t) keyframe_next(iter);
	while (t < keyframe_get(iter)->t) keyframe_prev(iter);
	return keyframe_get(iter);
}

void keyframe_next(KeyframeIter *iter) {
	if (++iter->index >= iter->chunk->len) {
		if (!iter->chunk->next) {
			iter->chunk->next = malloc(
				sizeof(KeyframeChunk) + sizeof(Keyframe) * KEYFRAMECHUNK_LEN
			);
			assert(iter->chunk->next);
			*iter->chunk->next = {
				.prev = iter->chunk;
				.len = KEYFRAMECHUNK_LEN;
			}
		}
		iter->chunk = iter->chunk->next;
		iter->index = 0;
	}
}

void ghost_tick(Ghost *self, int delta) {
	while (self) {
		Keyframe *kf;
		int t;
		Vec pos;

		self->t += delta;
		kf = keyframe_find(self->trail, self->t);
		t = self->t - kf->t;

		//TODO: vec_scale overflow
		pos = kf->state.pos;
		pos = vec_add(pos, vec_scale(kf->state.vel, t, 1));
		pos = vec_add(pos, vec_scale(kf->acc, t * t, 2));

		rect_intersect

		delta *= -1;
		self = self->next;
	}
}
