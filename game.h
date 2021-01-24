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

int ghost_getdamage(Ghost *self);
void game_tickghosts(Game *self, int delta);
