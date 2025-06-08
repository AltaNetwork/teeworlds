/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/entities/character.h>
#include <game/server/player.h>

#include "lmb.h"

CGameControllerLMB::CGameControllerLMB(class CGameContext *pGameServer) : IGameController(pGameServer)  {
    m_pGameType = "LMB      DDrace K"; // this ddarce K is for colour
    m_pTakeDamage = false;
    m_pNoAmmo = true;
    // m_GameFlags |= GAMEFLAG_TEAMS;
}
void CGameControllerLMB::Tick() {
    IGameController::Tick();
}

// int CGameControllerLMB::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
// {
// 	IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);


// 	// do team scoring
// 	if(pKiller == pVictim->GetPlayer() || pKiller->GetTeam() == pVictim->GetPlayer()->GetTeam())

// 	// pVictim->GetPlayer()->m_RespawnTick = max(pVictim->GetPlayer()->m_RespawnTick, Server()->Tick()+Server()->TickSpeed()*g_Config.m_SvRespawnDelayTDM);

// 	return 0;
// }
