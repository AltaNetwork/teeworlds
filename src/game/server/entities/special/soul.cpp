#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "soul.h"

CSoul::CSoul(CGameWorld *pGameWorld, vec2 Pos, int Owner)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PROJECTILE)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_WTeam = 0;

	CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(!pOwnerChar || !pOwnerChar->IsAlive())
    {
        // m_WTeam = pOwnerChar->GetVTeam();
    }

  	GameWorld()->InsertEntity(this);
}

void CSoul::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CSoul::Tick()
{
    CCharacter* pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
   	if(!pOwnerChar || !pOwnerChar->IsAlive())
	{
		Reset();
		return;
	}

	int X = abs(m_Pos.x-pOwnerChar->m_Pos.x);
	int Y = abs(m_Pos.y-pOwnerChar->m_Pos.y);

	CCharacter* pIntersect = GameWorld()->ClosestCharacter(m_Pos, 16, this);
	if(pIntersect && pIntersect == pOwnerChar)
	{
		pIntersect->m_EmoteType = EMOTE_HAPPY;
		pIntersect->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();

    	Reset();
    	return;
	}

	if(m_Pos.x < pOwnerChar->m_Pos.x)
	    m_Pos.x+=X/g_Config.m_SoulSpeed;
	else
	    m_Pos.x-=X/g_Config.m_SoulSpeed;
	if(m_Pos.y < pOwnerChar->m_Pos.y)
	    m_Pos.y+=Y/g_Config.m_SoulSpeed;
	else
	    m_Pos.y-=Y/g_Config.m_SoulSpeed;

	m_WTeam = pOwnerChar->GetVTeam();
}

void CSoul::Snap(int SnappingClient)
{

	CNetObj_Projectile *pObj = static_cast<CNetObj_Projectile *>(Server()->SnapNewItem(NETOBJTYPE_PROJECTILE, m_ID, sizeof(CNetObj_Projectile)));
	if(pObj)
	{
    	pObj->m_X = (int)m_Pos.x;
    	pObj->m_Y = (int)m_Pos.y;
    	pObj->m_VelX = 0;
    	pObj->m_VelY = 0;
    	pObj->m_StartTick = Server()->Tick();
    	pObj->m_Type = 0;
	}
}
