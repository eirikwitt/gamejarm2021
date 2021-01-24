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
	SUBPIXELS = 1<<16
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
	bool grounded;
	int health;
	Ghost *ghosts;
	Vec chechpoints[10];
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

static void allign(Player *self, Draw *draw, Vec tileRelative, int x, int y){
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
            self->speed.y = 0;
            break;
        case -1:
            self->grounded = true;
            self->pos.y -=tileRelative.y-(1<<(draw->subheight));
            self->speed.y = 0;
            break;
    }
}

static void correct_collision(Player *self, Draw *draw){
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
            if(self->speed.x >= 0){
                allign(self, draw, tileRelative, 0, 1);
            }else if(self->speed.y >= 0) {
                allign(self, draw, tileRelative, 1, 0);
            }else if(vec_grad(self->speed) > vec_grad(cornerVec)) {
                allign(self, draw, tileRelative, 0, 1);
            }else {
                allign(self, draw, tileRelative, 1, 0);
            }
            break;
        case 0b0010:
            cornerVec = vec_add(tileRelative, {0, -draw->tileSize.y});
            if(self->speed.x <= 0){
                allign(self, draw, tileRelative, 0, 1);
            }else if(self->speed.y >= 0) {
                allign(self, draw, tileRelative, -1, 0);
            }else if(vec_grad(self->speed) > vec_grad(cornerVec)) {
                allign(self, draw, tileRelative, -1, 0);
            }else {
                allign(self, draw, tileRelative, 0, 1);
            }
            break;
        case 0b0100:
            cornerVec = tileRelative;
            if(self->speed.x <= 0){
                allign(self, draw, tileRelative, 0, -1);
            }else if(self->speed.y <= 0) {
                allign(self, draw, tileRelative, -1, 0);
            }else if(vec_grad(self->speed) < vec_grad(cornerVec)) {
                allign(self, draw, tileRelative, -1, 0);
            }else {
                allign(self, draw, tileRelative, 0, -1);
            }
            break;
        case 0b1000:
            cornerVec = vec_add(tileRelative, {0, -draw->tileSize.x});
            if(self->speed.x >= 0){
                allign(self, draw, tileRelative, 0, -1);
            }else if(self->speed.y <= 0) {
                allign(self, draw, tileRelative, 1, 0);
            }else if(vec_grad(self->speed) > vec_grad(cornerVec)) {
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
			 vec_add(pos, self->player_size),
			 vec_add(pos, vec_neg(self->player_size))
		)) self->health -= ghost_getdamage(ghost);

		/* draw */
		//TODO

		ghost = ghost->next;
	}
}

int main(){
    using PC=Pokitto::Core;
    using PD=Pokitto::Display;
    using PB=Pokitto::Buttons;

    PC::begin();
    PD::loadRGBPalette(miloslav);

    //Loads tilemap
    Draw draw = {
        .subwidth = 16,
        .subheight = 16,
        .tileSize = draw_upscale(&draw, (Vec){PROJ_TILE_W, PROJ_TILE_H}),
        .mapSize = (Vec){gardenPath[0], gardenPath[1]},
        .lcdSize = (Vec){LCDWIDTH, LCDHEIGHT},
        .camOffset = vec_scale(draw.lcdSize, -1, 2),
        .camMax = vec_add(vec_mul(draw.mapSize, (Vec){PROJ_TILE_W, PROJ_TILE_H}), vec_neg(draw.lcdSize)),
        .camMin = vec_zero
    };

    draw.tilemap.set(gardenPath[0], gardenPath[1], gardenPath+2);
    for(int i=0; i<sizeof(tiles)/(POK_TILE_W*POK_TILE_H); i++)
        draw.tilemap.setTile(i, POK_TILE_W, POK_TILE_H, tiles+i*POK_TILE_W*POK_TILE_H);

    Player player = {
        .state.pos = {START_X, START_Y}
    };

    game.player.sprite.play(dude, Dude::yay);
    game.player_size = (Vec){
		game.player.sprite.getFrameWidth() * (SUBPIXELS / 2),
		game.player.sprite.getFrameHeight() * (SUBPIXELS / 2)
	};
    game.player.offset = vec_scale(player.size, -1, 2);

    //int cameraX = 64, cameraY = 64, speed = 3, recolor = 0;
    int gravity = 2560, xImpulse = 38400, yImpulse = -76800;

    while( PC::isRunning() ){
        if( !PC::update() )
            continue;

        player.state.vel.x = 0;

        if(PB::rightBtn()){
            player.state.vel.x += xImpulse;
        }
        if(PB::leftBtn()){
            player.state.vel.x -= xImpulse;
        }
        if(PB::aBtn()&&player.grounded){
            player.state.vel.y = yImpulse;
        }

        player.state.vel.y += gravity;
        player.grounded = false;

        player.state.pos = vec_add(player.state.pos, player.state.vel);
        correct_collision(&player, &draw);
        draw_map(&draw, &player);
        draw_player(&draw, &player);
    }

    return 0;
}
