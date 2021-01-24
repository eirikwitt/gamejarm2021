#include <Pokitto.h>
#include <miloslav.h>
#include <Tilemap.hpp>
//#include <SDFileSystem.h>
#include "sprites.h"
#include "Smile.h"
#include "maps.h"
#include "util.h"

#define START_X 5640
#define START_Y 5640

typedef struct {
    Sprite sprite;
    Vec size; //subpixels
    Vec offset; //subpixels
    Vec speed;
    Vec pos; //subpixels
    bool onGround;
} Player;

static const Vec corners[] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};

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



static void draw_map(Draw *self, Player *player){
    self->camPos = vec_max(
        vec_min(
            vec_add(draw_downscale(self, player->pos), self->camOffset), 
            self->camMax), 
        self->camMin);
    self->tilemap.draw(-self->camPos.x, -self->camPos.y);
}

static void draw_player(Draw *self, Player *player){
    volatile Vec screenPos = vec_add(
            draw_downscale(self,
                vec_add(player->pos, player->offset)
            ), 
            vec_neg(self->camPos));
    player->sprite.draw(screenPos.x, screenPos.y, false, false, 0);
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
            self->onGround = true;
            self->pos.y -=tileRelative.y-(1<<(draw->subheight));
            self->speed.y = 0;
            break;
    }
}


static void correct_collision(Player *self, Draw *draw){
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




int main(){
    using PC=Pokitto::Core;
    using PD=Pokitto::Display;
    using PB=Pokitto::Buttons;
    
    PC::begin();
    PD::loadRGBPalette(miloslav);
    
    //Loads tilemap
        
    Draw draw = {
        .subwidth = 6,
        .subheight = 6,
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
        .pos = {START_X, START_Y}
    };
    
    player.sprite.play(dude, Dude::yay);
    player.size = draw_upscale(&draw, {player.sprite.getFrameWidth(), player.sprite.getFrameHeight()});
    player.offset = vec_scale(player.size, -1, 2);
    
    

    
    //int cameraX = 64, cameraY = 64, speed = 3, recolor = 0;
    int pX = 64, pY = 64, gravity = 10, xImpulse = 150, yImpulse = -300;
    
    
    
    
    while( PC::isRunning() ){
        if( !PC::update() ) 
            continue;
        
        player.speed.x = 0;

        if(PB::rightBtn()){
            player.speed.x += xImpulse;
        }
        if(PB::leftBtn()){
            player.speed.x -= xImpulse;
        }
        if(PB::aBtn()&&player.onGround){
            player.speed.y = yImpulse;
        }
        
        player.speed.y += gravity;
        player.onGround = false;
        
        
        player.pos = vec_add(player.pos, player.speed);
        correct_collision(&player, &draw);
        draw_map(&draw, &player);
        draw_player(&draw, &player);
    }
    
    return 0;
}
