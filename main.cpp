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
    Vec speed;
    Vec pos;
} Player;

typedef struct {
	unsigned char subwidth, subheight; /* log2(subpixels per pixel) */
} Draw;

/* converts subpixel vector to pixel */
static Vec draw_downscale(Draw *self, Vec pt) {
	return (Vec){pt.x >> self->subwidth, pt.y >> self->subheight};
}

/* converts pixel vector to subpixel */
static Vec draw_upscale(Draw *self, Vec px) {
	return (Vec){px.x << self->subwidth, px.y << self->subheight};
}

int main(){
    using PC=Pokitto::Core;
    using PD=Pokitto::Display;
    using PB=Pokitto::Buttons;
    
    PC::begin();
    PD::loadRGBPalette(miloslav);
    
    //Loads tilemap
    Tilemap tilemap;
    tilemap.set(gardenPath[0], gardenPath[1], gardenPath+2);
    for(int i=0; i<sizeof(tiles)/(POK_TILE_W*POK_TILE_H); i++)
        tilemap.setTile(i, POK_TILE_W, POK_TILE_H, tiles+i*POK_TILE_W*POK_TILE_H);
        
    Player player = {
        .pos = {START_X, START_Y}
    };
    player.sprite.play(dude, Dude::walkS);
    player.size = {player.sprite.getFrameWidth(), player.sprite.getFrameHeight()};
    
    Vec camOffset = {LCDWIDTH/2 - player.width/2, LCDHEIGHT/2 - player.height/2};
    Vec lcdSize = {LCDWIDTH, LCDHEIGHT}
    

    
    //int cameraX = 64, cameraY = 64, speed = 3, recolor = 0;
    int pX = 64, pY = 64, gravity = 1, xImpulse = 3, yImpulse = -10;
    
    
    
    
    while( PC::isRunning() ){
        if( !PC::update() ) 
            continue;
            
        int oldX = pX;
        int oldY = pY;
        

        if(PB::rightBtn()){
            pX += xImpulse;
        }else if(PB::leftBtn()){
            pX -= xImpulse;
        }
        if(PB::aBtn()){
            ySpeed += yImpulse;
        }
        ySpeed += gravity;
        //pY += ySpeed;
        
        int tileX = (pX + playerWidth) / PROJ_TILE_W;
        int tileY = (pY + playerHeight) / PROJ_TILE_H;
        auto tile = gardenPathEnum(tileX, tileY);

        if( tile&Collide ){
            pX = oldX;
            pY = oldY;
            ySpeed = 0;
        }
        

        tilemap.draw(0, 0);
        player.sprite.draw(pX, pY, false, false, 0);
    }
    
    return 0;
}
