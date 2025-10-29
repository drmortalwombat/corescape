#include "intro.h"
#include "display.h"
#include "status.h"
#include <c64/vic.h>
#include <c64/rasterirq.h>
#include <oscar.h>
#include "gamemusic.h"

const char IntroFont[] = {
	#embed ctm_chars lzo "introfont.ctm"
};

// Tiles for expanding the title scrolling big font

// 3 1     1 0    1
// 2 0            0

#define LTILE_NONE				0

#define	LTILE_ETL	LevelTiles[ 7 + NUM_TILES * 10]
#define LTILE_ETR	LevelTiles[ 9 + NUM_TILES *  9]
#define LTILE_EBL	LevelTiles[12 + NUM_TILES *  6]
#define LTILE_EBR	LevelTiles[14 + NUM_TILES *  5]

#define LTILE_WL	LevelTiles[10 + NUM_TILES *  2]
#define LTILE_WR	LevelTiles[11 + NUM_TILES *  1]
#define LTILE_WT	LevelTiles[ 8 + NUM_TILES *  8]
#define LTILE_WB	LevelTiles[13 + NUM_TILES *  4]

#define	LTILE_CTL	LevelTiles[18 + NUM_TILES * 10]
#define LTILE_CTR	LevelTiles[17 + NUM_TILES *  9]
#define LTILE_CBL	LevelTiles[16 + NUM_TILES *  6]
#define LTILE_CBR	LevelTiles[15 + NUM_TILES *  5]

#define LTILE_FULL	LevelTiles[6]


// Expand a single font pixel into left most of three columns
// lmask is the bitmask to the left, mask the bitmask for this column
// the actual pixel is 2 with 1 and 4 for pixel above and below
void intro_dot_left(char * dp, char lmask, char mask)
{
	if (mask & 2)
	{
		if (lmask & 2)
		{
			if (mask & 1)
			{
				if (lmask & 1)
					dp[0] = LTILE_FULL;
				else
					dp[0] = LTILE_CTL;
			}
			else
				dp[0] = LTILE_WT;
			dp[40] = LTILE_FULL;
			if (mask & 4)
			{
				if (lmask & 4)
					dp[80] = LTILE_FULL;
				else
					dp[80] = LTILE_CBL;
			}
			else
				dp[80] = LTILE_WB;
		}
		else
		{
			if (mask & 1)
				dp[0] = LTILE_WL;
			else
				dp[0] = LTILE_ETL;
			dp[40] = LTILE_WL;
			if (mask & 4)
				dp[80] = LTILE_WL;
			else
				dp[80] = LTILE_EBL;
		}
	}
	else
		dp[0] = dp[40] = dp[80] = 0;		
}

// Expand a single font pixel into center of three columns
void intro_dot_center(char * dp, char mask)
{
	if (mask & 2)
	{
		if (mask & 1)
			dp[0] = LTILE_FULL;
		else
			dp[0] = LTILE_WT;
		dp[40] = LTILE_FULL;
		if (mask & 4)
			dp[80] = LTILE_FULL;
		else
			dp[80] = LTILE_WB;
	}
	else
		dp[0] = dp[40] = dp[80] = 0;		
}

// Expand a single font pixel into right most of three columns
void intro_dot_right(char * dp, char rmask, char mask)
{
	if (mask & 2)
	{
		if (rmask & 2)
		{
			if (mask & 1)
			{
				if (rmask & 1)
					dp[0] = LTILE_FULL;
				else
					dp[0] = LTILE_CTR;
			}
			else
				dp[0] = LTILE_WT;
			dp[40] = LTILE_FULL;
			if (mask & 4)
			{
				if (rmask & 4)
					dp[80] = LTILE_FULL;
				else
					dp[80] = LTILE_CBR;
			}
			else
				dp[80] = LTILE_WB;
		}
		else
		{
			if (mask & 1)
				dp[0] = LTILE_WR;
			else
				dp[0] = LTILE_ETR;
			dp[40] = LTILE_WR;
			if (mask & 4)
				dp[80] = LTILE_WR;
			else
				dp[80] = LTILE_EBR;
		}
	}
	else
		dp[0] = dp[40] = dp[80] = 0;		
}

// Expand leftmost column of three column font expansion
void intro_col_left(char lmask, char mask)
{
	char * dp = Screen0 + 39;

	unsigned	xmask  = mask << 1;
	unsigned	xlmask = lmask << 1;

	for(char i=0; i<8; i++)
	{
		intro_dot_left(dp, xlmask, xmask);
		dp += 120;
		xmask >>= 1;
		xlmask >>= 1;
	}

}

// Expand center column of three column font expansion
void intro_col_center(char mask)
{
	char * dp = Screen0 + 39;

	unsigned	xmask = mask << 1;

	for(char i=0; i<8; i++)
	{
		intro_dot_center(dp, xmask);
		dp += 120;
		xmask >>= 1;
	}
}

// Expand rightmost column of three column font expansion
void intro_col_right(char rmask, char mask)
{
	unsigned	xmask  = mask << 1;
	unsigned	xrmask = rmask << 1;

	char * dp = Screen0 + 39;

	for(char i=0; i<8; i++)
	{
		intro_dot_right(dp, xrmask, xmask);
		dp += 120;
		xmask >>= 1;
		xrmask >>= 1;
	}
}

// Scroll upper region of screen
void intro_scroll_0(void)
{
	for(char x=0; x<39; x++)
	{
		#pragma unroll(full)
		for(char y=0; y<7; y++)
			Screen0[40 * y + x] = Screen0[40 * y + x + 1];
	}
}

// Scroll lower regions of screen, split into three sections
// to race the beam
void intro_scroll_1(void)
{
	for(char x=0; x<39; x++)
	{
		#pragma unroll(full)
		for(char y=9; y<16; y++)
			Screen0[40 * y + x] = Screen0[40 * y + x + 1];
	}
	for(char x=0; x<39; x++)
	{
		#pragma unroll(full)
		for(char y=16; y<20; y++)
			Screen0[40 * y + x] = Screen0[40 * y + x + 1];
	}
	for(char x=0; x<39; x++)
	{
		#pragma unroll(full)
		for(char y=20; y<25; y++)
			Screen0[40 * y + x] = Screen0[40 * y + x + 1];
	}
}

static const char IntroText[] = S"\y80CORESCAPE BY CRISPS AND DR.MORTAL WOMBAT   ";
static const char ScrollText[] = 
	S"\y81*** CORESCAPE ***\y80   "
	S"--- MUSIC BY \y83CRISPS\y80 "
	S"--- CODING AND GRAPHICS BY \y83DR.MORTAL WOMBAT\y80 "
	S"--- JOYSTICK IN PORT \y812\y80 "
	S"--- BUTTON TO START "
	S"--- SPACE OR 2ND FIRE TO PAUSE "
	S"--- CHEATS: \y81S\y82KIP, \y81T\y82RAINER, \y81L\y82IVE, \y81R\y82ETRY, \y81H\y82ALFSPEED \y80"
	S"--- NO HIGHSCORE WHEN CHEATING :) ---       ";

// Fill intro column with background
void intro_col_back(char cb)
{
	Screen0[40 * 24 + 39] = 0;
	for(char i=0; i<25; i++)
	{
		if (!Screen0[40 * i + 39])
		{
			if (i > 0 && i != 9 && Screen0[40 * (i - 1) + 38] >= 8)
				Screen0[40 * i + 39] = cb | (i & 1) | 4;
			else
				Screen0[40 * i + 39] = cb | (i & 1);
		}
	}
}

void intro_back(char px)
{
	// Offset of parallax scrolled regions, half in x due
	// to multicolor mode
	char	ty = 0;
	char	tx = px & 7;

	ty += 32 * (tx & 3);

	if (tx & 4)
	{
		// Copy the char data from the cached shifted chars
		for(char i=0; i<16; i++)
		{
			Charset[i +  0] = LevelScrollFont[0][0][ty];
			Charset[i + 16] = LevelScrollFont[0][1][ty];

			Charset[i + 32] = LevelScrollFont[1][0][ty];
			Charset[i + 48] = LevelScrollFont[1][1][ty];

			// Next line
			ty++;
		}
	}
	else
	{
		for(char i=0; i<16; i++)
		{
			Charset[i +  0] = LevelScrollFont[0][1][ty];
			Charset[i + 16] = LevelScrollFont[0][0][ty];

			Charset[i + 32] = LevelScrollFont[1][1][ty];
			Charset[i + 48] = LevelScrollFont[1][0][ty];

			// Next line
			ty++;
		}		
	}
}

static RIRQCode	ScrollerTop, ScrollerBottom, ScrollerMusic;

char HighscoreText[] = "H$000000";

static const char HighscoreColor[] = {
	VCOL_BLACK, VCOL_RED, VCOL_ORANGE, VCOL_YELLOW,
	VCOL_WHITE, VCOL_YELLOW, VCOL_ORANGE, VCOL_RED,
};

__interrupt void intro_music(void)
{
	music_play();
}

void intro_play(void)
{
	// Setup screen
	vic.ctrl1 = VIC_CTRL1_DEN | VIC_CTRL1_RSEL | 3;
	vic.memptr = 0xce;
	vspr_screen(Screen0);

	background_init(0);

	for(char y=0; y<25; y++)
	{
		for(char x=0; x<40; x+=2)
		{
			Screen0[40 * y + x] = (y & 1);
			Screen0[40 * y + x + 1] = (y & 1) | 2;
		}
	}

	// Expand intro font
	oscar_expand_lzo(Screen1, IntroFont);

	for(signed char i=63; i>=0; i--)
	{
		char * sp = Screen1 + 8 * i;
		char * dp = Screen1 + 8 * i + 512;

		dp[0] = sp[4]; dp[1] = sp[5]; dp[2] = sp[6]; dp[3] = sp[7];
		dp[4] = 0; dp[5] = 0; dp[6] = 0; dp[7] = 0;
		sp[4] = sp[0]; sp[5] = sp[1]; sp[6] = sp[2]; sp[7] = sp[3];
		sp[0] = 0; sp[1] = 0; sp[2] = 0; sp[3] = 0;
	}
	display_fade_in();

	// Prepare line with smaller scroll text
	for(char i=0; i<40; i++)
	{
		Screen0[7 * 40 + i] = 0xa0;
		Screen0[8 * 40 + i] = 0xe0;
		Color[7 * 40 + i] = VCOL_YELLOW;
		Color[8 * 40 + i] = VCOL_ORANGE;
	}

	// Build interrupts for screen splits
	rirq_build(&ScrollerTop, 4);
	rirq_build(&ScrollerBottom, 5);
	rirq_build(&ScrollerMusic, 1);

	rirq_delay(&ScrollerTop, 10);
	rirq_write(&ScrollerTop, 1, &vic.color_back, VCOL_BLACK);
	rirq_write(&ScrollerTop, 2, &vic.ctrl2, 0);
	rirq_write(&ScrollerTop, 3, &vic.memptr, 0xcc);

	rirq_delay(&ScrollerBottom, 10);
	rirq_write(&ScrollerBottom, 1, &vic.color_back, VCOL_LT_BLUE);
	rirq_write(&ScrollerBottom, 2, &vic.ctrl2, VIC_CTRL2_MCM);
	rirq_write(&ScrollerBottom, 3, &vic.memptr, 0xce);
	rirq_call(&ScrollerBottom, 4, intro_music);

	rirq_call(&ScrollerMusic, 0, intro_music);


	rirq_set(17, 50 + 7 * 8 - 1, &ScrollerTop);
	rirq_set(18, 50 + 9 * 8 - 1, &ScrollerBottom);
	rirq_set(19, 232, &ScrollerMusic);

	// Set sprites for sinewave
	for(char i=0; i<16; i++)
		vspr_set(i, 24 + 20 * i, 180 + (sintab[i * 13 & 127] >> 1), SPIMAGE_CORVETTE, VCOL_PURPLE);

	// Prepare sprites for highscore
	for(char i=0; i<6; i++)
		HighscoreText[i + 2] = highscore[i] + '0';

	for(char i=0; i<8; i++)
	{
		text_sprimage(SPIMAGE_TEXT_1 + i, HighscoreText[i]);
		vspr_set(i + 16, 100 + 20 * i, 65, SPIMAGE_TEXT_1 + i, VCOL_YELLOW);
	}

	vspr_sort();
	vspr_update();


	rirq_sort();

	char cj = 0;
	unsigned ci = 0x100;
	char cc = 0;
	char cb = 2;
	char px = 0;
	char mask = 0, lmask = 0, rmask = 0;

	char sj = 0xff, si = 0;
	char spi = 0, spw = 0;

	for(;;)
	{
		lmask = mask;
		mask = rmask;

		const char * cp = StatusFont + 8 * (IntroText[cj] & 0x3f);

		// Build new column mask for big scroller
		for(char i=0; i<8; i++)
		{
			rmask >>= 1;
			if (cp[i] & ci)
				rmask |= 0x80;
		}

		// Three steps to add one more font column
		for(char cc=0; cc<3; cc++)
		{
			// Even number frame
			char j = spi++;
			spw++;

			// Update sprites
			char co = HighscoreColor[spw & 7];
			for(char i=0; i<8; i++)
				vspr_color(i + 16, co);

			for(char i=0; i<16; i++)
			{
				vspr_move(i, 24 + 20 * i, 180 + (sintab[j & 127] >> 1));
				vspr_image(i, SPIMAGE_CORVETTE | spw & 3);
				j += 13;
			}			
			vspr_sort();
			vic_waitBottom();
			vspr_update();
			rirq_sort();

			// Odd number frame

			intro_back(px); px--;
			vic.ctrl2 = VIC_CTRL2_MCM;
			rirq_data(&ScrollerBottom, 2, VIC_CTRL2_MCM);

			// Scroll center text
			if (si == 0)
			{
				for(char i=0; i<39; i++)
				{
					Screen0[7 * 40 + i] = Screen0[7 * 40 + i + 1];
					Screen0[8 * 40 + i] = Screen0[8 * 40 + i + 1];
					Color[7 * 40 + i] = Color[7 * 40 + i + 1];
					Color[8 * 40 + i] = Color[8 * 40 + i + 1];
				}
				sj++;
				if (!ScrollText[sj])
					sj = 0;

				si = 6;
			}
			else
				si -= 2;
			rirq_data(&ScrollerTop, 2, si & 7);

			// Add new column for big text
			switch(cc)
			{
			case 0:
				intro_col_left(lmask, mask);
				break;
			case 1:
				intro_col_center(mask);
				break;
			case 2:
				intro_col_right(rmask, mask);
				break;
			}
			intro_col_back(cb);

			// Next column for center scroll text
			if (ScrollText[sj] & 0x80)
			{
				switch (ScrollText[sj])
				{
				case 0x80:
					Color[7 * 40 + 39] = VCOL_YELLOW;
					Color[8 * 40 + 39] = VCOL_ORANGE;
					break;
				case 0x81:
					Color[7 * 40 + 39] = VCOL_WHITE;
					Color[8 * 40 + 39] = VCOL_LT_GREY;
					break;
				case 0x82:
					Color[7 * 40 + 39] = VCOL_LT_BLUE;
					Color[8 * 40 + 39] = VCOL_BLUE;
					break;
				case 0x83:
					Color[7 * 40 + 39] = VCOL_LT_GREEN;
					Color[8 * 40 + 39] = VCOL_GREEN;
					break;
				}
				sj++;
			}

			Screen0[7 * 40 + 39] = ScrollText[sj] | 0x80;
			Screen0[8 * 40 + 39] = ScrollText[sj] | 0xc0;

			// Update sprites
			j = spi++;
			for(char i=0; i<16; i++)
			{
				vspr_move(i, 24 + 20 * i, 180 + (sintab[j & 127] >> 1));
				j += 13;
			}
			vspr_sort();
			vic_waitBelow(50 + 7 * 8);
			intro_scroll_0();
			vic_waitBottom();
			vspr_update();

			intro_back(px); px--;
			vic.ctrl2 = VIC_CTRL2_MCM | 4;
			rirq_data(&ScrollerBottom, 2, VIC_CTRL2_MCM | 4);
			si -= 2;
			rirq_data(&ScrollerTop, 2, si & 7);

			intro_scroll_1();
			cb ^= 2;
		}
		ci >>= 1;
		if (!ci)
		{
			cj++;
			if (!IntroText[cj])
				cj = 0;
			ci = 0x100;
		}

		joy_poll(0);
		if (joyb[0])
			break;
	}

	music_volume(8);	

	rirq_clear(17);
	rirq_clear(18);
	rirq_clear(19);
	for(char i=0; i<24; i++)
		vspr_hide(i);
	vspr_sort();
	vic_waitBottom();
	vspr_update();
	rirq_sort();

	memset(Screen0, 254, 1000);
	memset(Color, VCOL_WHITE + 8, 1000);

	music_volume(4);	

	display_fade_out();

	music_volume(0);	
}
