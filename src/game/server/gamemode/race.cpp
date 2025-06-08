/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "race.h"
#include <game/server/gamecontext.h>

CGameControllerRACE::CGameControllerRACE(class CGameContext *pGameServer) : IGameController(pGameServer)  {
    m_pGameType = "Race";
    m_pTimeScore = true;
    m_pTakeDamage = false;
    m_pNoAmmo = true;
    m_pPausable = true;
    GameServer()->Tuning()->m_PlayerHit = false;
    GameServer()->Tuning()->m_PlayerHooking = false;
    GameServer()->Tuning()->m_PlayerCollision = false;
}
void CGameControllerRACE::Tick() {	IGameController::Tick();    }
