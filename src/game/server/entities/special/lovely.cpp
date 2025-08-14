#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "lovely.h"

CLovely::CLovely(CGameWorld *pGameWorld, vec2 Pos, int VTeam)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Pos = Pos;
	m_WTeam = VTeam;
	m_Lifetime = 1*SERVER_TICK_SPEED;

  	GameWorld()->InsertEntity(this);
}

void CLovely::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CLovely::Tick()
{
    m_Pos.y-=5;
    m_Lifetime--;
    if(m_Lifetime < 0)
        Reset();
}

void CLovely::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Pickup *pP = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
	if(!pP)
		return;

	pP->m_X = (int)m_Pos.x;
	pP->m_Y = (int)m_Pos.y;
	pP->m_Type = POWERUP_HEALTH;
	pP->m_Subtype = 0;
}
