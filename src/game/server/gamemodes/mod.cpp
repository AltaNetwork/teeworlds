#include "mod.h"
#include <game/generated/protocol.h>
#include <engine/shared/config.h>

CGameControllerMOD::CGameControllerMOD(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "Block   BW"; // 16 characters max

	//m_GameFlags = GAMEFLAG_TEAMS; // GAMEFLAG_TEAMS makes it a two-team gamemode
}
