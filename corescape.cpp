#include <c64/vic.h>
#include <c64/memmap.h>
#include <c64/joystick.h>
#include <c64/sprites.h>
#include <c64/rasterirq.h>
#include <c64/keyboard.h>
#include <oscar.h>
#include <string.h>
#include <c64/cia.h>
#include "gamemusic.h"
#include "display.h"
#include "assets.h"
#include "player.h"
#include "enemies.h"
#include "levelseq0.h"
#include "levelseq1.h"
#include "levelseq2.h"
#include "levelseq3.h"
#include "levelseq4.h"
#include "levelseq11.h"
#include "levelseq12.h"
#include "levelseq13.h"
#include "levelseq14.h"
#include "levelseq15.h"
#include "levelseq16.h"
#include "levelseq17.h"
#include "levelseq18.h"
#include "status.h"
#include "intro.h"

#define NOP __asm{nop}
#define SEI __asm{sei}
#define CLI __asm{cli}

static char LevelText[] = S"STAGE 1";
static char GameOverText[] = S"GAMEOVER";

bool	trainer_mode, level_skip, level_retry, restart;
bool	cheating;
bool    second_fire, previous_second_fire;

struct Level
{
	const char 	*	seq;
	const char 	*	wave;
	char 			lsize, back;
	Tune			tune;
	bool			hardcore;
	const char *	name;
}	Levels[] = {

	{LevelSequence1, LevelWaves1, sizeof(LevelSequence1), 0, TUNE_MAIN_1, false, S"STAGE 1"},
	{LevelSequence11, LevelWaves11, sizeof(LevelSequence11), 4, TUNE_INTERLUDE, false, S"BONUS"},
	{LevelSequence2, LevelWaves2, sizeof(LevelSequence2), 1, TUNE_MAIN_2, false, S"STAGE 2"},
	{LevelSequence12, LevelWaves12, sizeof(LevelSequence12), 4, TUNE_INTERLUDE, false, S"ENJOY"},
	{LevelSequence3, LevelWaves3, sizeof(LevelSequence3), 2, TUNE_MAIN_3, false, S"STAGE 3"},
	{LevelSequence13, LevelWaves13, sizeof(LevelSequence13), 4, TUNE_INTERLUDE, false, S"CLOSE"},
	{LevelSequence4, LevelWaves4, sizeof(LevelSequence4), 3, TUNE_MAIN_4, false, S"STAGE 4"},
	{LevelSequence14, LevelWaves14, sizeof(LevelSequence14), 4, TUNE_INTERLUDE, false, S"BOOST"},

	{LevelSequence1, LevelWaves1, sizeof(LevelSequence1), 0, TUNE_MAIN_1, true, S"STAGE 5"},
	{LevelSequence15, LevelWaves15, sizeof(LevelSequence15), 4, TUNE_INTERLUDE, false, S"STAMINA"},
	{LevelSequence2, LevelWaves2, sizeof(LevelSequence2), 1, TUNE_MAIN_2, true, S"STAGE 6"},
	{LevelSequence16, LevelWaves16, sizeof(LevelSequence16), 4, TUNE_INTERLUDE, false, S"SOLACE"},
	{LevelSequence3, LevelWaves3, sizeof(LevelSequence3), 2, TUNE_MAIN_3, true, S"STAGE 7"},
	{LevelSequence17, LevelWaves17, sizeof(LevelSequence17), 4, TUNE_INTERLUDE, false, S"BURDEN"},
	{LevelSequence4, LevelWaves4, sizeof(LevelSequence4), 3, TUNE_MAIN_4, true, S"STAGE 8"},
	{LevelSequence18, LevelWaves18, sizeof(LevelSequence18), 4, TUNE_INTERLUDE, false, S"VICTORY"}
};

static const char PausedText[] = "PAUSED";

const char PausedColors[] = {
	VCOL_PURPLE, VCOL_RED, VCOL_ORANGE, VCOL_YELLOW,
	VCOL_ORANGE, VCOL_RED, VCOL_PURPLE, VCOL_BLUE
};

void check_second_fire(void)
{
	byte* JoyReg = (byte*)0xDC00;
	byte* CtrlPort = (byte*)0xD419;
	byte b;

	previous_second_fire = second_fire;

	SEI
	JoyReg[2] = 0xC0;
	NOP
	b = JoyReg[0];
	b = b & 0b00111111;
	b = b | 0b10000000;
	JoyReg[0] = b;
	for (byte c = 0; c < 20; c++)
		NOP
	
	b = CtrlPort[0];
	CLI
	if (b < 16) second_fire = true;
	else second_fire = false;
	JoyReg[0] = 0x7F;
	JoyReg[2] = 0xFF;
}


void game_pause(void)
{
	// Silence during pause
	music_volume(0);

	// Show PAUSE text using sprites
	for(char i=0; i<6; i++)
	{
		text_sprimage(SPIMAGE_TEXT_1 + i, PausedText[i]);
		vspr_set(i + 16, 100, 10, SPIMAGE_TEXT_1 + i, VCOL_PURPLE);
	}

	// Animate paused text
	char ci = 0;
	for(;;)
	{
		int cx = 120 + sintab[ci & 0x7f];
		int cy = 140 + (sintab[(ci + 0x20) & 0x7f] >> 1);

		for(char i=0; i<6; i++)
		{
			vspr_move(i + 16, cx + 20 * i, cy + (sintab[(4 * ci + 8 * i) & 0x7f] >> 4));
			vspr_color(i + 16, PausedColors[ci & 7]);
		}
		ci++;

		score_update();
		vspr_sort();

		rirq_wait();
		vspr_update();
		rirq_sort();

		// Wait for spacebar to continue
		keyb_poll();
		if (keyb_key == (KSCAN_SPACE | KSCAN_QUAL_DOWN))
			break;

		// ... or second fire to continue
		check_second_fire();
		if (second_fire && !previous_second_fire)
			break;

	}

	// Hide pause text sprited
	for(char i=0; i<6; i++)
		vspr_hide(i + 16);

	music_volume(15);	
}

// Mark score grey to signal cheat mode
void game_cheat(void)
{
	cheating = true;
	vspr_color(6, VCOL_DARK_GREY);
	vspr_color(7, VCOL_DARK_GREY);	
}

void game_keyboard(void)
{
	keyb_poll();
	if (keyb_key & KSCAN_QUAL_DOWN)
	{
		switch (keyb_key & 0x7f)
		{
		case KSCAN_R:
			level_skip = true;
			level_retry = true;
			game_cheat();
			break;
		case KSCAN_S:
			level_skip = true;
			game_cheat();
			break;
		case KSCAN_T:
			trainer_mode = true;
			game_cheat();
			break;
		case KSCAN_H:
			halfspeed = !halfspeed;
			game_cheat();
			break;
		case KSCAN_L:
			ships_inc();
			break;
		case KSCAN_STOP:
			restart = true;
			level_skip = true;
			break;
		case KSCAN_SPACE:
			game_pause();
			break;

		}
	}
	//check Second Fire
	check_second_fire();
	if (second_fire && !previous_second_fire)
		game_pause();
}


void game_play(void)
{
	// Undo all cheat modes at a new start
	trainer_mode = false;	
	cheating = false;
	hardcore = false;
	intermission = false;
	restart = false;
	halfspeed = false;

	// Clear score
	score_init();

	// Loop through all 16 levels
	char level = 0;
	while (level < 16)
	{
		level_skip = false;
		level_retry = false;

		// Check for special level stats
		hardcore = Levels[level].hardcore;
		intermission = level & 1;

		// Init level data and music
		level_init(Levels[level].seq, Levels[level].wave, Levels[level].lsize);
		music_init(Levels[level].tune);
		background_init(Levels[level].back);

		music_volume(15);

		// Init of player data and enemies list
		player_init();
		enemies_init();

		display_fade_in();

		// Show level string
		const char * str = Levels[level].name;
		char i = 0;
		while (str[i])
		{
			if (str[i] != ' ')
			{
				text_sprimage(SPIMAGE_TEXT_1 + i, str[i]);
				enemies_add(350 + 20 * i, 100, ET_LEVEL, i, 0);
			}
			i++;	
		}

		// Main game loop
		for(;;)
		{
			game_keyboard();

			player_move();

			enemies_move();
			enemies_check();

			display_loop();

			if (playerState == PLST_DESTROYED)
			{					
				if (!trainer_mode)
				{
					if (num_ships == 0)
						break;
					ships_dec();
				}

				player_init();
			}
			else if (levely < 4 || level_skip)
			{
				break;
			}
		}

		halfspeed = false;

		if (restart) return;

		// Level end animation
		for(int i=0; i<15; i++)
		{
			music_volume(15 - i);
			for(int j=0; j<10; j++)
			{
				if (playerState != PLST_DESTROYED && shipy > 20)
				{
					shipy -= i;
					vspr_move(0, shipx - vscreenx, shipy);
				}

				score_update();
				shots_move();
				vspr_sort();

				rirq_wait();
				vspr_update();
				rirq_sort();

				music_play();
				vic_waitBelow(100);					
				music_play();

				game_keyboard();
				if (restart) return;
			}
		}

		music_volume(0);

		// Check for game end
		if (playerState == PLST_DESTROYED && num_ships == 0)
		{
			enemies_init();

			// Display game over text
			unsigned x = 338;
			for(char i=0; i<8; i++)
			{
				text_sprimage(SPIMAGE_TEXT_1 + i, GameOverText[i]);
				enemies_add(x, 100, ET_LEVEL, i, 0);
				x += 20;
				if (i == 3)
					x += 20;
			}

			music_init(TUNE_GAMEOVER);
			music_volume(15);

			for(int i=0; i<15; i++)
			{
				for(int j=0; j<20; j++)
				{
					score_update();
					enemies_move();
					shots_move();
					vspr_sort();

					rirq_wait();
					vspr_update();
					rirq_sort();

					music_play();
					vic_waitBelow(100);					
					music_play();

					game_keyboard();
					if (restart) return;
				}
			}
			display_fade_out();

			break;			
		}

		display_fade_out();

		if (!level_retry)
			level++;
	}

	// Check for highscore if not cheating
	if (!cheating)
		score_check();
}

int main(void)
{
	vic.ctrl1 = 0;
	vic_waitFrame();

	assets_init();

	display_init();

	for(char i=0; i<6; i++)
		highscore[i] = 0;

	for(;;)
	{
		hardcore = false;
		intermission = false;

		music_init(TUNE_HIGHSCORE);
		music_volume(15);
		intro_play();			

		game_play();
	}

	return 0;
}