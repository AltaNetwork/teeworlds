/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "gores.h"

CGameControllerGORES::CGameControllerGORES(class CGameContext *pGameServer) : IGameController(pGameServer)  {
    m_pGameType = "Gores";
    m_pTimeScore = true;
    m_pTakeDamage = false;
    m_pNoAmmo = true;
    m_pPausable = true;
}
void CGameControllerGORES::Tick() {	IGameController::Tick();    }
