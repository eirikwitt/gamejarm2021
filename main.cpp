#include <Pokitto.h>
#include <miloslav.h>
#include <Tilemap.hpp>
//#include <SDFileSystem.h>
#include "sprites.h"
#include "Smile.h"
#include "maps.h"

extern "C" {
#include "util.h"
#include "trail.h"
}

#define START_X 5640
#define START_Y 5640

enum {
	SUBPIXELS = 1<<16,
};

typedef struct Ghost Ghost;
struct Ghost {
	Trail trail;
	signed t;
	Ghost *next;
};



typedef struct {
	EntityState player;
	Trail trail;
	Sprite psprite;
	Vec playersize;
	Vec acc;
	Vec impulse;
	Vec gravity;
	bool grounded;
	int health;
	Ghost *ghosts;
	Sprite cpsprite;
	Vec chechpoints[10];
	int currentcp;
	Draw draw;
} Game;

typedef struct {
	EntityState state;
	Trail trail;
    Sprite sprite;
    Vec size; //subpixels
    Vec offset; //subpixels
    bool grounded;
} Player;

typedef struct {
	unsigned char subwidth, subheight; /* log2(subpixels per pixel) */
	Vec tileSize; //subpixels
	Vec mapSize;  //number of tiles
	Vec lcdSize; //pixels
	Vec camOffset; //pixels
	Vec camMax; //pixels
	Vec camMin; //pixels
	Vec camPos; //pixels
	Tilemap tilemap;
} Draw;

/* converts subpixel vector to pixel */
static Vec draw_downscale(Draw *self, Vec pt) {
	return (Vec){pt.x >> self->subwidth, pt.y >> self->subheight};
}

/* converts pixel vector to subpixel */
static Vec draw_upscale(Draw *self, Vec px) {
	return (Vec){px.x << self->subwidth, px.y << self->subheight};
}

Vec tile_from_pos(Draw *self, Vec pt){
    return vec_div(pt, self->tileSize);
}

Vec pos_from_tile(Draw *self, Vec tile){
    return vec_mul(tile, self->tileSize);
}

static void draw_map(Draw *self, Player *player){
    self->camPos = vec_max(
        vec_min(
            vec_add(draw_downscale(self, player->pos), self->camOffset),
            self->camMax),
        self->camMin);
    self->tilemap.draw(-self->camPos.x, -self->camPos.y);
}

static void draw_player(Draw *self, Player *player){
    Vec screenPos = vec_add(
        draw_downscale(self,
            vec_add(player->pos, player->offset)
        ),
        vec_neg(self->camPos)
    );
	Vec tl = vec_add(game.player.pos, vec_neg(game.playersize));
    player->sprite.draw(tl.x / SUBPIXELS, tl.y / SUBPIXELS, false, false, 0);
}

static void draw_sprite(Sprite sprite, Vec pos, Vec size){
    Vec screenPos = vec_add(
        draw_downscale(self,
            vec_add(pos, vec_neg(size))
        ),
        vec_neg(self->camPos)
    );
    sprite.draw(screenPos.x, screenPos.y, false, false, 0);
}

static void allign(PlayerState *self, Draw *draw, Vec tileRelative, int x, int y){
    switch (x) {
        case 1:
            self->pos.x += draw->tileSize.x - tileRelative.x;
            break;
        case -1:
            self->pos.x -=tileRelative.x;
            break;
    }
    switch (y) {
        case 1:
            self->pos.y += draw->tileSize.y - tileRelative.y;
            self->vel.y = 0;
            break;
        case -1:
            self->grounded = true;
            self->pos.y -= tileRelative.y-(1<<(draw->subheight));
            self->vel.y = 0;
            break;
    }
}

static void correct_collision(PlayerState *self, Draw *draw){
    static const Vec corners[] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
	int collisions = 0;
    Vec tileRelative = vec_mod(vec_add(self->pos, self->offset), draw->tileSize);
    for(int i = 0; i < lenof(corners); i++){
        Vec pt = vec_add(vec_mul(corners[i], vec_scale(self->size, 1, 2)), self->pos);
        Vec tile = tile_from_pos(draw, pt);
        volatile auto tileEnum = gardenPathEnum(tile.x, tile.y);
        if(tileEnum&Collide){
            collisions += 1<<i;
        }
    }
    Vec cornerVec;
    switch (collisions) {
        case 0b0001:
            cornerVec = vec_add(tileRelative, vec_neg(draw->tileSize));
            if(self->vel.x >= 0){
                allign(self, draw, tileRelative, 0, 1);
            }else if(self->vel.y >= 0) {
                allign(self, draw, tileRelative, 1, 0);
            }else if(vec_grad(self->vel) > vec_grad(cornerVec)) {
                allign(self, draw, tileRelative, 0, 1);
            }else {
                allign(self, draw, tileRelative, 1, 0);
            }
            break;
        case 0b0010:
            cornerVec = vec_add(tileRelative, {0, -draw->tileSize.y});
            if(self->vel.x <= 0){
                allign(self, draw, tileRelative, 0, 1);
            }else if(self->vel.y >= 0) {
                allign(self, draw, tileRelative, -1, 0);
            }else if(vec_grad(self->vel) > vec_grad(cornerVec)) {
                allign(self, draw, tileRelative, -1, 0);
            }else {
                allign(self, draw, tileRelative, 0, 1);
            }
            break;
        case 0b0100:
            cornerVec = tileRelative;
            if(self->vel.x <= 0){
                allign(self, draw, tileRelative, 0, -1);
            }else if(self->vel.y <= 0) {
                allign(self, draw, tileRelative, -1, 0);
            }else if(vec_grad(self->vel) < vec_grad(cornerVec)) {
                allign(self, draw, tileRelative, -1, 0);
            }else {
                allign(self, draw, tileRelative, 0, -1);
            }
            break;
        case 0b1000:
            cornerVec = vec_add(tileRelative, {0, -draw->tileSize.x});
            if(self->vel.x >= 0){
                allign(self, draw, tileRelative, 0, -1);
            }else if(self->vel.y <= 0) {
                allign(self, draw, tileRelative, 1, 0);
            }else if(vec_grad(self->vel) > vec_grad(cornerVec)) {
                allign(self, draw, tileRelative, 0, -1);
            }else {
                allign(self, draw, tileRelative, 1, 0);
            }
            break;
        case 0b0011:
            allign(self, draw, tileRelative, 0, 1);
            break;
        case 0b0110:
        allign(self, draw, tileRelative, -1, 0);
            break;
        case 0b1100:
        allign(self, draw, tileRelative, 0, -1);
            break;
        case 0b1001:
            allign(self, draw, tileRelative, 1, 0);
            break;
        case 0b0111:
            allign(self, draw, tileRelative, -1, 1);
            break;
        case 0b1110:
            allign(self, draw, tileRelative, -1, -1);
            break;
        case 0b1101:
            allign(self, draw, tileRelative, 1, -1);
            break;
        case 0b1011:
            allign(self, draw, tileRelative, 1, 1);
            break;
    }
}

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
		pos = kf->state.state.pos;
		pos = vec_add(pos, vec_scale(kf->state.vel, t, 1));
		pos = vec_add(pos, vec_scale(kf->acc, t * t, 2));

		/* detect collision with player */
		if (game_does_player_overlap(self,
			 vec_add(pos, vec_neg(self->player_size)),
			 vec_add(pos, self->player_size)
		)) self->health -= ghost_getdamage(ghost);

		/* draw */
		draw_sprite(self->psprite, pos, self->playersize);

		ghost = ghost->next;
	}
}

void game_tick_checkpoint(Game *self, int delta){
    if (game_does_player_overlap(self,
			 vec_add(self->checkpoints[self->currentcp], vec_neg(self->player_size)),
			 vec_add(self->checkpoints[self->currentcp], self->player_size)
		)){
        self->currentcp = self->currentcp+1 % 10;
        draw_sprite(self->cpsprite, game->checkpoints[game->currentcp], {7*SUBPIXELS, 14*SUBPIXELS})
	}
}

void game_captureframe(Game *self) {
	*trail_add(self->trail) = (Keyframe){
		.state = self->player,
		.t = self->t,
		.acc = self->acc
	}
}

void game_tick(Game *self, int delta) {
	bool dirty = false;
	int32_t velx = 0;
	Vel acc = {0, gravity};

	self->t += delta;
	if (PB::rightBtn()) velx += self->impulse.x;
	if (PB::leftBtn()) velx -= self->impulse.x;
	if (PB::aBtn()) {
		if (self->grounded) {
			self->player.vel.y = -self->impulse.y;
			dirty = true;
		}
		acc.y += -gravity / 2;
	}
	if (velx != self->player.vel.x) dirty = true;
	if (acc != self->acc) dirty = true;
	self->acc = acc;
	if (dirty) game_captureframe(self);
	self->player.vel = vec_add(self->player.vel, vec_scale(self->acc, delta, 1));
	self->player.pos = vec_add(self->player.pos, vec_scale(self->player.vel, delta, 1));
    correct_collision(&self->player, &self->draw);
    draw_map(&self->draw, &self->player);
    draw_player(&self->draw, &self->player);
	game_tickghosts(self, delta);
}

int main(){
    using PC=Pokitto::Core;
    using PD=Pokitto::Display;
    using PB=Pokitto::Buttons;

    PC::begin();
    PD::loadRGBPalette(miloslav);

	Game game = {
		.player = {
			.pos = {START_X, START_Y}
		},
		.playersize = {
			game.player.sprite.getFrameWidth() * (SUBPIXELS / 2),
			game.player.sprite.getFrameHeight() * (SUBPIXELS / 2)
		},
		.impulse = {38400, 76800},
		.gravity = {0, 2560},
		.health = 1,
		.checkpoints = {
		    pos_from_tile(draw, {24, 28}),
		    pos_from_tile(draw, {18, 21}),
		    pos_from_tile(draw, {11, 21}),
		    pos_from_tile(draw, {2, 21}),
		    pos_from_tile(draw, {8, 17}),
		    pos_from_tile(draw, {7, 24}),
		    pos_from_tile(draw, {27, 21}),
		    pos_from_tile(draw, {2, 28}),
		    pos_from_tile(draw, {1, 14}),
		    pos_from_tile(draw, {24, 17})
		},
		.draw = {
	        .subwidth = 16,
	        .subheight = 16,
	        .tileSize = draw_upscale(&draw, (Vec){PROJ_TILE_W, PROJ_TILE_H}),
	        .mapSize = (Vec){gardenPath[0], gardenPath[1]},
	        .lcdSize = (Vec){LCDWIDTH, LCDHEIGHT},
	        .camOffset = vec_scale(draw.lcdSize, -1, 2),
	        .camMax = vec_add(vec_mul(draw.mapSize, (Vec){PROJ_TILE_W, PROJ_TILE_H}), vec_neg(draw.lcdSize)),
	        .camMin = vec_zero
    	}
	};

	*trail_add(&game.trail) = {};
    draw.tilemap.set(gardenPath[0], gardenPath[1], gardenPath+2);
    for(int i=0; i<sizeof(tiles)/(POK_TILE_W*POK_TILE_H); i++)
        draw.tilemap.setTile(i, POK_TILE_W, POK_TILE_H, tiles+i*POK_TILE_W*POK_TILE_H);

    game.checkpoint.sprite.play(checkpoint, Checkpoint::base);
    game.player.sprite.play(dude, Dude::yay);

	time_t time = PC::getTime();

    while(PC::isRunning()){
        if(!PC::update())
            continue;
		time_t new_time = PC::getTime();
		game_tick(&game, new_time - time);
		time = new_time;
    }

    return 0;
}
