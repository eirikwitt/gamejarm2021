typedef struct {
	EntityState state;
	Trail trail;
} Player;

typedef struct Ghost Ghost;
struct Ghost {
	Trail trail;
	signed t;
	Ghost *next;
};

typedef struct {
	Player player;
	Ghost *ghosts;
	Vec player_size;
	int health;
} Game;

int ghost_getdamage(Ghost *self);
void game_tickghosts(Game *self, int delta);
