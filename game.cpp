#include <stdbool.h>

extern "C" {
#include "util.h"
#include "trail.h"
}
#include "game.h"

int ghost_getdamage(Ghost *self) {
	/* despawned ghosts and the most recently spawned ghost deal no damage */
	return self->t >= 0 && self->next;
}

bool game_does_player_overlap(Game *self, Vec tl, Vec br) {
	Vec ptl = vec_add(self->player.state.pos, self->player_size);
	Vec pbr = vec_add(self->player.state.pos, vec_neg(self->player_size));
	return (
		vec_isinside(ptl, tl, br) || vec_isinside(pbr, tl, br) ||
		vec_isinside(tl, ptl, pbr) || vec_isinside(br, ptl, pbr)
	);
}

void game_tickghosts(Game *self, int delta) {
	Ghost *ghost = self->ghosts;
	while (ghost) {
		Keyframe *kf;
		int t;
		Vec pos;

		/* calculate position */
		delta *= -1;
		ghost->t += delta;
		kf = trail_find(&ghost->trail, ghost->t);
		t = ghost->t - kf->t;
		//TODO: vec_scale overflow
		pos = kf->state.pos;
		pos = vec_add(pos, vec_scale(kf->state.vel, t, 1));
		pos = vec_add(pos, vec_scale(kf->acc, t * t, 2));

		/* detect collision with player */
		if (game_does_player_overlap(self,
			 vec_add(pos, self->player_size),
			 vec_add(pos, vec_neg(self->player_size))
		)) self->health -= ghost_getdamage(ghost);

		/* draw */
		//TODO

		ghost = ghost->next;
	}
}
