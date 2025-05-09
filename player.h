#pragma once

enum PlayerState
{
	PLST_IDLE,
	PLST_ENTERING,
	PLST_ACTIVE,
	PLST_EXPLODING,
	PLST_DESTROYED
};

extern PlayerState	playerState;
extern char			playerStateCount;

extern int shipx, shipy;
extern char shots, shotd;

extern __striped struct Shot
{
	int 		x;
	char 		y;
	signed char	dx;
}	shot[4];


// Init player structs
void player_init(void);

// Move player
void player_move(void);

// Move player shots
void shots_move(void);

#pragma compile("player.cpp")

