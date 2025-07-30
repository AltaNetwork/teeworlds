#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "koh.h"

CKoh::CKoh(CGameWorld *pGameWorld, vec2 Pos, int Flags)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Flags = Flags;
	m_WTeam = 0;
	m_Radius = 100;

  	GameWorld()->InsertEntity(this);
}

void CKoh::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CKoh::Tick()
{
}

void CKoh::Snap(int SnappingClient)
{
	// if(NetworkClipped(SnappingClient))
	// 	return;

	float X = m_Pos.x+(sin(Server()->Tick()*0.05f))*m_Radius;
	float Y = m_Pos.y+(cos(Server()->Tick()*0.05f))*m_Radius;

	CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pObj)
	{
    	pObj->m_X = (int)X;
    	pObj->m_Y = (int)Y;
    	pObj->m_VelX = 0;
    	pObj->m_VelY = 0;
    	pObj->m_StartTick = Server()->Tick();
    	pObj->m_Type = 0;
	}
}
