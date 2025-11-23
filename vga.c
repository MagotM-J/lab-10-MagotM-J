#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// if using CPUlator, you should copy+paste contents of the file below instead of using #include
#include "address_map_niosv.h"

#define CTRL_OFFSET 6
#define ROBOTSET_OFFSET 4
#define EDGE_WIDTH 1
#define WINNING_SCORE 9
#define GAME_SPEED 800000
#define GAME_SPEEDR 30000

#define CEL_SPEED 1000
#define NUM_SQ 15

typedef uint16_t pixel_t;

typedef enum {
	NONE = 4,
	LEFT = 0,
	UP = 1,
	RIGHT = 2,
	DOWN = 3
} dir_t;

//for testing
int score1 = 0;
int score2 = 0;

volatile pixel_t *pVGA = (pixel_t *)FPGA_PIXEL_BUF_BASE;
volatile uint32_t *pHEX = (uint32_t *)HEX3_HEX0_BASE;
volatile uint8_t *pBUT = (uint8_t *)KEY_BASE;
volatile uint16_t *pSW = (uint16_t *)SW_BASE;

const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;

const pixel_t pl1_color = red;
const pixel_t pl2_color = blu;

typedef struct {
	int dead;
	int ctrl;
	int turned_left;
    int is_robot;
	int score;
	int x;
	int y;
	pixel_t color;
	dir_t dir;
} player_t; 

player_t pl1;
player_t pl2;


void delay( int N )
{
	for( int i=0; i<N; i++ ) 
		*pVGA; // read volatile memory location to waste time
}





/* STARTER CODE BELOW. FEEL FREE TO DELETE IT AND START OVER */
//game functions
void game_init();
void game();
void end_game(player_t *player);


//helper functions
void update( player_t *cycle);
void set_dir(player_t *cycle);
void autoset_dir(player_t *cycle);
void set_robot(int *game_speed);
uint32_t get_hex(int num);
void display_score(int s1, int s2);


//graphics functions
pixel_t get_pixel(int y, int x);
pixel_t make_pixel( uint8_t r8, uint8_t g8, uint8_t b8 );
pixel_t next_pixel(int y, int x, dir_t dir);
void draw_pixel( int y, int x, pixel_t colour );
void rect( int y1, int y2, int x1, int x2, pixel_t c );



int main()
{
	srand(time(NULL));
	game_init(0,0);
	delay(100000);
	game();
    //will not reach this point
    return 0;
}

void game_init(int s1, int s2){
	for(int x=0; x<MAX_X; x++){
		for(int y=0; y<MAX_Y; y++){
			if(x < EDGE_WIDTH  || x >= MAX_X-EDGE_WIDTH || y < EDGE_WIDTH || y >= MAX_Y-(EDGE_WIDTH+2))
				draw_pixel( y, x, wht );
			else
				draw_pixel( y, x, blk );
		}
	}

	for(int i = 0; i < NUM_SQ; i++){
		int sq_x = (rand() % (MAX_X - 2*EDGE_WIDTH - 4)) + EDGE_WIDTH + 2;
		int sq_y = (rand() % (MAX_Y - 2*EDGE_WIDTH - 4)) + EDGE_WIDTH + 2;
		rect(sq_y - 2, sq_y + 2, sq_x - 2, sq_x + 2, wht);
	}
	
	dir_t rand_dir2 = (rand() % 4);
	dir_t rand_dir1 = (rand() % 4);
	
	pl1 = (player_t){
		.dead = 0,
		.turned_left = 0,
		.ctrl = 0,
        .is_robot = 0,
		.score = s1,
		.x = 2*MAX_X/3,
		.y = MAX_Y/2,
		.dir = rand_dir1,
		.color = pl1_color
	};

	pl2 = (player_t){
		.dead = 0,
		.turned_left = 0,
		.ctrl = 1,
        .is_robot = 0,
		.score = s2,
		.x = MAX_X/3 - 1,
		.y = MAX_Y/2,
		.dir = rand_dir2,
		.color = pl2_color
	};
	
	draw_pixel(pl1.y, pl1.x, pl1.color);
	draw_pixel(pl2.y, pl2.x, pl2.color);
    display_score( pl1.score, pl2.score);
}

void game(){
	int game_over = 0;
	int game_speed = GAME_SPEED;
	
	while(!game_over){
		set_robot(&game_speed);

		update(&pl1);
		update(&pl2);
		if (pl1.dead || pl2.dead) {
			int s1 =  pl1.score;
			int s2 = pl2.score;
			if (pl1.dead && !pl2.dead) {
				s2 = s2 + 1;
			} else if (pl2.dead && !pl1.dead) {
				s1 = s1 + 1;
			}
			game_init( s1, s2);
			if(s1 >= WINNING_SCORE){
				end_game(&pl1);
				game_over = 1;
			}
			if(s2 >= WINNING_SCORE){
				end_game(&pl2);
				game_over = 1;
			}

		}
		delay(game_speed);
	}
}

void end_game(player_t *player){
	pixel_t color = player->color;
	for( int y=0; y<MAX_Y; y++ ) {
		for( int x=0; x<MAX_X; x++ ) {
			// red, green, blue and white colour bars
			//const uint32_t scale = (x+y) / (MAX_X+MAX_Y); // scale = 0..255
			//uint8_t r8 = scale*((color & 0xf800)>>11);
			//uint8_t g8 = scale*((color & 0x07e0)>>5);
			//uint8_t b8 = scale*(color & 0x001f);
			//pixel_t colour = make_pixel( r8, g8, b8 ); 
			draw_pixel( y, x, color );
			delay(CEL_SPEED);
		}
	}
	if(player == &pl1)
		score1++;
	else if(player == &pl2)
		score2++;
		
	printf("\n\n\n\nGame Over!\nTotal Wins \nPlayer 1: %d \nPlayer 2: %d\n", score1, score2);
    // Wait for player to press button to start new game
	while(1)
		if((*pBUT&0x1)){ 
			game_init(0,0);
			game();
		}
}

void update( player_t *cycle){
	if(cycle->dead) return;
	//rect( (cycle->y)-(PL_SIZE/2), (cycle->y)+(PL_SIZE+1)/2, (cycle->x)-(PL_SIZE/2), (cycle->x)+(PL_SIZE+1)/2, blk);
    if(cycle->is_robot){
        autoset_dir(cycle);
    }else{
        set_dir(cycle);
	}

	switch(cycle->dir){
		case UP: cycle->y--; break;
		case DOWN: cycle->y++; break;
		case LEFT: cycle->x--; break;
		case RIGHT: cycle->x++; break;
		default: break;
	}
	pixel_t p = get_pixel(cycle->y, cycle->x) ;
	if(p != blk && cycle->dir != NONE ){
		cycle->dead = 1;
	} else
		draw_pixel(cycle->y, cycle->x, cycle->color);
}

void set_dir(player_t *cycle){
	int off_set = cycle->ctrl * CTRL_OFFSET;
    if(((*pSW>>off_set)&0x1) && cycle->dir != LEFT) cycle->dir = RIGHT;
    else if(((*pSW>>off_set)&0x2) && cycle->dir != DOWN) cycle->dir = UP;
    else if(((*pSW>>off_set)&0x4) && cycle->dir != UP) cycle->dir = DOWN;
    else if(((*pSW>>off_set)&0x8) && cycle->dir != RIGHT) cycle->dir = LEFT;
}

//make it alternate between trying left and right first
//by conditionally returning from left if statemnt 
void autoset_dir(player_t *cycle){
	dir_t dir = cycle->dir;
    if(next_pixel(cycle->y, cycle->x, cycle->dir) == blk) return;

	dir_t left = (cycle->dir + 3)%4; 
	if(next_pixel(cycle->y, cycle->x, left) == blk){
		dir = left;
		if(!cycle->turned_left/*rand()%2 == 0*/){
			cycle->dir = dir;
			cycle->turned_left = 1;
			return;
		}
	} 
	dir_t right = (cycle->dir + 1)%4;
	if(next_pixel(cycle->y, cycle->x, right) == blk){
		cycle->turned_left = 0;
		dir = right;
	}
	cycle->dir = dir;
}

void set_robot(int *game_speed){
	pl1.is_robot = !((*pSW >> ROBOTSET_OFFSET)&0x1);
	pl2.is_robot = !((*pSW >> ROBOTSET_OFFSET)&0x2);

	*game_speed = (*pSW & 0x0030)? GAME_SPEED : GAME_SPEEDR;

}

uint32_t get_hex(int num){
	uint32_t hex_output;

    switch (num) {
		case 0: hex_output = 0x40; break;
        case 1: hex_output = 0x79; break;
        case 2: hex_output = 0x24; break;
        case 3: hex_output = 0x30; break;
        case 4: hex_output = 0x19; break;
        case 5: hex_output = 0x12; break;
        case 6: hex_output = 0x02; break;
        case 7: hex_output = 0x78; break;
        case 8: hex_output = 0x00; break;
        case 9: hex_output = 0x10; break;
        default: hex_output = 0xFF; break;
    }

    return ~(hex_output | 0x80)&0x000000ff;
}

void display_score(int s1, int s2){
	uint32_t hex1 = get_hex(s1);
	uint32_t hex2 = get_hex(s2);
	
	uint32_t p = (hex2<<(3*8))|(hex1);
	
	*pHEX = p; 
}

pixel_t make_pixel( uint8_t r8, uint8_t g8, uint8_t b8 )
{
	// inputs: 8b of each: red, green, blue
	const uint16_t r5 = (r8 & 0xf8)>>3; // keep 5b red
	const uint16_t g6 = (g8 & 0xfc)>>2; // keep 6b green
	const uint16_t b5 = (b8 & 0xf8)>>3; // keep 5b blue
	return (pixel_t)( (r5<<11) | (g6<<5) | b5 );
}

pixel_t get_pixel(int y, int x){
	pixel_t p = *(pVGA + (y<<YSHIFT) + x );
	return p;
}

//make it return if the next n pixels in the given dir are blk or not, with input n.
//maybe call it dir_clear(int y, int x, dir_t dir, int n) or something
pixel_t next_pixel(int y, int x, dir_t dir){
	pixel_t p;
	switch(dir){ 
		case UP: p = get_pixel(y - 1, x); break;
		case DOWN: p = get_pixel(y + 1, x); break;
		case LEFT: p = get_pixel(y, x - 1); break;
		case RIGHT: p = get_pixel(y, x + 1); break;
		default: p = blk; break;
	}
	return p;
}

void draw_pixel( int y, int x, pixel_t colour ){
	*(pVGA + (y<<YSHIFT) + x ) = colour;
}

void rect( int y1, int y2, int x1, int x2, pixel_t c ){
	for( int y=y1; y<y2; y++ )
		for( int x=x1; x<x2; x++ )
			draw_pixel( y, x, c );
}