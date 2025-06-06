/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "lmb.h"

CGameControllerLMB::CGameControllerLMB(class CGameContext *pGameServer) : IGameController(pGameServer)  {
    m_pGameType = "LMB";
    m_pTakeDamage = false;
}
void CGameControllerLMB::Tick() {
    IGameController::Tick();
}
