/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_LMB_H
#define GAME_SERVER_GAMEMODES_LMB_H
#include <game/server/gamecontroller.h>
// LMB MODIFICATION

class CGameControllerLMB : public IGameController
{
public:
	CGameControllerLMB(class CGameContext *pGameServer);
	// int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual void Tick();
};
#endif
