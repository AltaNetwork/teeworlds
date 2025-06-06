/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_RACE_H
#define GAME_SERVER_GAMEMODES_RACE_H
#include <game/server/gamecontroller.h>
// RACE MODIFICATION

class CGameControllerRACE : public IGameController
{
public:
	CGameControllerRACE(class CGameContext *pGameServer);
	virtual void Tick();
};
#endif
