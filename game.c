#include "trail.h"
#include "game.h"

int ghost_getdamage(Ghost *self) {
	/* despawned ghosts and the most recently spawned ghost deal no damage */
	return self->t >= 0 && self->next;
}

void game_tickghosts(Game *self, int delta)) {
	Ghost *ghost = self->ghosts;
	while (ghost) {
		Keyframe *kf;
		int t;
		Vec pos;

		/* calculate position */
		delta *= -1;
		ghost->t += delta;
		kf = keyframe_find(ghost->trail, ghost->t);
		t = ghost->t - kf->t;
		//TODO: vec_scale overflow
		pos = kf->state.pos;
		pos = vec_add(pos, vec_scale(kf->state.vel, t, 1));
		pos = vec_add(pos, vec_scale(kf->acc, t * t, 2));

		/* detect collision with player */
		if (game_is_player_inside(self,
			 vec_add(pos, game->player_size),
			 vec_add(pos, vec_neg(game->player_size))
		)) self->health -= ghost_getdamage(ghost);

		/* draw */
		//TODO

		ghost = ghost->next;
	}
}
