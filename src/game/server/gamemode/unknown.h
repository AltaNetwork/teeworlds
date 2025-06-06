/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_UNKNOWN_H
#define GAME_SERVER_GAMEMODES_UNKNOWN_H
#include <game/server/gamecontroller.h>

class CGameControllerUNKNOWN : public IGameController
{
public:
	CGameControllerUNKNOWN(class CGameContext *pGameServer);
	virtual void Tick();
};
#endif
