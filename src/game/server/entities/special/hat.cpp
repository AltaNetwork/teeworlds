#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "hat.h"

CHat::CHat(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Owner = Owner;
	m_Pos = Pos;
	m_WTeam = 0;

	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(pOwnerChar && pOwnerChar->IsAlive())
	    m_WTeam = pOwnerChar->GetCore().m_VTeam;

  	GameWorld()->InsertEntity(this);
}

void CHat::Reset()
{
    CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
    if(pOwnerChar)
    {
        pOwnerChar->m_Hat = -1;
    }
    GameServer()->m_World.DestroyEntity(this);
}

void CHat::Tick()
{
	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(!pOwnerChar || !pOwnerChar->IsAlive())
	{
		Reset();
		return;
	}

	m_Pos = vec2(pOwnerChar->m_Pos.x, pOwnerChar->m_Pos.y - 48);
	m_WTeam = pOwnerChar->GetCore().m_VTeam;

}

void CHat::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

    CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

    if(pOwnerChar && pOwnerChar->IsFrozen())
        return;

    int Intensifier = 1;
    if(SnappingClient == m_Owner)
        Intensifier = 2;
    vec2 Velocity = vec2(0,0);
    if(pOwnerChar)
    {
        Velocity = pOwnerChar->GetCore().m_Vel;
    }
	CNetObj_Pickup *pObj = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(pObj)
	{
		pObj->m_X = m_Pos.x+Velocity.x*Intensifier;
		pObj->m_Y = m_Pos.y+Velocity.y*Intensifier;
		pObj->m_Type = POWERUP_WEAPON;
		pObj->m_Subtype = WEAPON_NINJA;
	}
}
