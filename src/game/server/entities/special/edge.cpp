#include <game/server/gamecontext.h>
#include "edge.h"

CEdge::CEdge(CGameWorld *pGameWorld, vec2 Pos, bool Tourna)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Pos = Pos;
	m_WTeam = -1;
	m_Tournament = Tourna;

  	GameWorld()->InsertEntity(this);
}

void CEdge::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CEdge::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
	    return;

	if(GameServer()->m_apPlayers[SnappingClient]->m_DuelFlags&CPlayer::DUEL_INDUEL || (GameServer()->m_apPlayers[SnappingClient]->m_LMBState == CPlayer::LMB_PLAYING && m_Tournament))
	{
    	CNetObj_Pickup *pObj = static_cast<CNetObj_Pickup *>(Server()->SnapNewItem(NETOBJTYPE_PICKUP, m_ID, sizeof(CNetObj_Pickup)));
    	if(pObj)
    	{
    		pObj->m_X = m_Pos.x;
    		pObj->m_Y = m_Pos.y;
    		pObj->m_Type = POWERUP_ARMOR;
    		pObj->m_Subtype = 0;
    	}
	}
}
