#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include "loremipsum.h"
#include "loltext.h"

CLoremIpsum::CLoremIpsum(CGameWorld *pGameWorld, vec2 Pos)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_PICKUP)
{
	m_Pos = Pos;
	m_WTeam = 0;

  	GameWorld()->InsertEntity(this);
}

void CLoremIpsum::Reset()
{
    GameServer()->m_World.DestroyEntity(this);
}

void CLoremIpsum::Tick()
{
    if(Server()->Tick()%(SERVER_TICK_SPEED*5) == 0)
    {
        CLoltext::Create(GameWorld(), m_Pos, SERVER_TICK_SPEED*3, "/DUEL", true, false, 0);
    }
    // Reset();
}

void CLoremIpsum::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;
}
