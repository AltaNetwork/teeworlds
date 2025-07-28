/*
 *	by Rei
*/

#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "passive.h"

CPassiveIndicator::CPassiveIndicator(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Flags)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Owner = Owner;
	m_Pos = Pos;
	m_Flags = Flags;
	m_WTeam = 0;

  	GameWorld()->InsertEntity(this);
}

void CPassiveIndicator::Reset()
{
    CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
    if(pOwnerChar)
    {
        if(m_Flags&INDFLAG_PASSIVE)
            pOwnerChar->m_Indicator = false;
        if(m_Flags&INDFLAG_HAT)
            pOwnerChar->m_Hat = false;
    }
    GameServer()->m_World.DestroyEntity(this);
}

void CPassiveIndicator::Tick()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(!pOwnerChar || !pOwnerChar->IsAlive())
	{
		Reset();
		return;
	}
	if(m_Flags&INDFLAG_PASSIVE && pOwnerChar->GetCore().m_VTeam > -1)
	{
		Reset();
		return;
	}
	if(m_Flags&INDFLAG_HAT && )
	m_Pos = vec2(pOwnerChar->m_Pos.x, pOwnerChar->m_Pos.y - 48);
}

void CPassiveIndicator::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

    CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

    if(m_Flags&INDFLAG_FREEZEHIDE && pOwnerChar->IsFrozen())
        return;

    int Intensifier = 1;
    if(SnappingClient == m_Owner)
        Intensifier = 3;
    vec2 Velocity = vec2(0,0);
    if(pOwnerChar)
        Velocity = pOwnerChar->GetCore().m_Vel;

	CNetObj_Pickup *pObj = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(pObj)
	{
		pObj->m_X = m_Pos.x+Velocity.x*Intensifier;
		pObj->m_Y = m_Pos.y+Velocity.y*Intensifier;
		if(m_Flags&INDFLAG_PASSIVE)
		{
    		pObj->m_Type = POWERUP_ARMOR;
    		pObj->m_Subtype = 0;
		}
		if(m_Flags&INDFLAG_HAT)
		{
		    pObj->m_Type = POWERUP_WEAPON;
    		pObj->m_Subtype = WEAPON_GUN;
		}
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
