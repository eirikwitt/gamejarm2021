#include <Pokitto.h>
#include <miloslav.h>
#include <Tilemap.hpp>
//#include <SDFileSystem.h>
#include "sprites.h"
#include "Smile.h"
#include "maps.h"
#include "util.h"

#define START_X 640
#define START_Y 640

typedef struct {
    Sprite sprite;
    Vec size;
    Vec offset;
    Vec speed;
    Vec pos;
} Player;

static const Vec corners[] = {{-1, -1}, {1, -1}, {-1, -1}, {-1, 1}};

typedef struct {
	unsigned char subwidth, subheight; /* log2(subpixels per pixel) */
	Vec tileSize;
	Vec mapSize;
	Vec lcdSize;
	Vec camOffset;
	Vec camMax;
	Vec camMin;
	Vec camPos;
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



static Vec tile_from_pos(Draw *self, Vec pt){
    Vec px = draw_downscale(self, pt);
    return (Vec){px.x / self->tileSize.x, px.y / self->tileSize.y};
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
    volatile Vec screenPos = vec_add(draw_downscale(self, player->pos), vec_neg(self->camPos));
    player->sprite.draw(screenPos.x, screenPos.y, false, false, 0);
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
        .tileSize = (Vec){PROJ_TILE_W, PROJ_TILE_H},
        .mapSize = (Vec){gardenPath[0], gardenPath[1]},
        .lcdSize = (Vec){LCDWIDTH, LCDHEIGHT},
        .camOffset = vec_scale(draw.lcdSize, -1, 2),
        .camMax = vec_add(vec_scale(draw.mapSize, draw.tileSize.x, 1), vec_neg(draw.lcdSize)),
        .camMin = vec_zero
    };
    
    draw.tilemap.set(gardenPath[0], gardenPath[1], gardenPath+2);
    for(int i=0; i<sizeof(tiles)/(POK_TILE_W*POK_TILE_H); i++)
        draw.tilemap.setTile(i, POK_TILE_W, POK_TILE_H, tiles+i*POK_TILE_W*POK_TILE_H);
        
    Player player = {
        .pos = {START_X, START_Y}
    };
    
    player.sprite.play(dude, Dude::walkS);
    player.size = {player.sprite.getFrameWidth(), player.sprite.getFrameHeight()};
    player.offset = vec_scale(player.size, -1, 2);
    
    

    
    //int cameraX = 64, cameraY = 64, speed = 3, recolor = 0;
    int pX = 64, pY = 64, gravity = 10, xImpulse = 300, yImpulse = -400;
    
    
    
    
    while( PC::isRunning() ){
        if( !PC::update() ) 
            continue;
            
        Vec oldPos = player.pos;
        player.speed.x = 0;

        if(PB::rightBtn()){
            player.speed.x += xImpulse;
        }
        if(PB::leftBtn()){
            player.speed.x -= xImpulse;
        }
        if(PB::aBtn()){
            player.speed.y = yImpulse;
        }
        //player.speed.y += gravity;
        
        
        Vec tileVec = tile_from_pos(&draw, player.pos);
        auto tile = gardenPathEnum(tileVec.x, tileVec.y);

        // if( tile&Collide ){
        //     pX = oldX;
        //     pY = oldY;
        // }
        

        //draw.tilemap.draw(20, 20);
        player.pos = vec_add(player.pos, player.speed);
        draw_map(&draw, &player);
        draw_player(&draw, &player);
    }
    
    return 0;
}
