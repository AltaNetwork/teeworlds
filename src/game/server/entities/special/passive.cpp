/*
 *	by Rei
*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "passive.h"

CPassiveIndicator::CPassiveIndicator(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Owner = Owner;
	m_Pos = Pos;

  	GameWorld()->InsertEntity(this);
}

void CPassiveIndicator::Reset()
{
    CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
    if(pOwnerChar)
    {
        pOwnerChar->m_PassiveInd = false;
    }
    GameServer()->m_World.DestroyEntity(this);
}

void CPassiveIndicator::Tick()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(!pOwnerChar || !pOwnerChar->IsAlive() || pOwnerChar->GetCore().m_VTeam > -1)
	{
		Reset();
		return;
	}

	m_Pos = vec2(pOwnerChar->m_Pos.x, pOwnerChar->m_Pos.y - 48);
}

void CPassiveIndicator::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

    // CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	CNetObj_Pickup *pObj = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(pObj)
	{
		pObj->m_X = m_Pos.x;
		pObj->m_Y = m_Pos.y;
		pObj->m_Type = POWERUP_ARMOR;
		pObj->m_Subtype = 0;
	}
 //// THIS IS SOME SHITTY HATS AND STUFF MAKE IT IN SAME CODE FOR EASe OF USE
	// if(m_Core.m_VTeam < 0 ) {
 //        CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, IDStarter, sizeof(CNetObj_Pickup)));
 //        if(!pP)
 //     		return;
 //        pP->m_X = (int)m_Pos.x;
 //        pP->m_Y = (int)m_Pos.y-48;
 //        pP->m_Type = POWERUP_ARMOR;
	// };
	// CNetObj_Pickup *pP1 = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, IDStarter+1, sizeof(CNetObj_Pickup)));
 //    if(!pP1)
 //    	return;
 //    pP1->m_X = (int)m_Pos.x-cos(Server()->Tick()/5.0f)*30;
 //    pP1->m_Y = (int)m_Pos.y-36;
 //    pP1->m_Type = POWERUP_HEALTH;

 //    CNetObj_Pickup *pP2 = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, IDStarter+2, sizeof(CNetObj_Pickup)));
 //    if(!pP1)
 //    	return;
 //    pP2->m_X = (int)m_Pos.x+cos(Server()->Tick()/5.0f)*30;
 //    pP2->m_Y = (int)m_Pos.y-36;
 //    pP2->m_Type = POWERUP_HEALTH;
}
