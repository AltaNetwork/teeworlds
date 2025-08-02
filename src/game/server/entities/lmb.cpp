#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <memory>
#include "lmb.h"
#include "character.h"

CLmb::CLmb(CGameWorld *pGameWorld)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_FLAG)
{
    m_WTeam = 0;
    m_Winner = -1;
    m_TimeLeft = g_Config.m_SvLMBTime * SERVER_TICK_SPEED;
    m_RegTimeLeft = g_Config.m_SvLMBRegTime * SERVER_TICK_SPEED;
    m_IsTeamfight = false;

    GameServer()->m_LMBState = CGameContext::LMB_REG;
    GameServer()->m_LastLMB = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CLmb::Reset()
{
    GameServer()->m_LMBState = CGameContext::LMB_NAN;
    for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_PLAYING)
		{
			GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_SELF);
		}
	}
	GameWorld()->DestroyEntity(this);
}

bool CLmb::StartLMB()
{
    int Subscribers = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_REG)
		    Subscribers++;
	}
    if(Subscribers < 2)
        return false;

    GameServer()->m_LMBState = CGameContext::LMB_IN;
    for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_REG)
		{
		    GameServer()->m_apPlayers[i]->m_SpawnTeam = 2;
		    GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_SELF);
			GameServer()->m_apPlayers[i]->m_DieTick = 0;
		    GameServer()->m_apPlayers[i]->m_LMBState = CPlayer::LMB_PLAYING;
		}
	}
    return true;
}

void CLmb::Tick()
{
    char aBuf[512];
    switch(GameServer()->m_LMBState)
    {
        case CGameContext::LMB_REG:
            m_RegTimeLeft--;
            if (Server()->Tick() % SERVER_TICK_SPEED == 0)
            {
                int Registers = 0;
                for(int i = 0; i < MAX_CLIENTS; i++)
               	{
              		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_REG)
                        Registers++;
               	}
                bool Registered = false;
                for(int i = 0; i < MAX_CLIENTS; i++)
               	{
              		if(GameServer()->m_apPlayers[i])
              		{
                        Registered = GameServer()->m_apPlayers[i]->m_LMBState == CGameContext::LMB_REG ? true : false;
                        str_format(aBuf, sizeof(aBuf), "Tournament is starting in %ds\n%d subscribed %s                                                       ",
                        m_RegTimeLeft/SERVER_TICK_SPEED , Registers, Registered ? "" : "(use /sub to subscribe)");
                       	GameServer()->SendBroadcast(aBuf, i);
              		}
                }
            }
            if(m_RegTimeLeft < 1)
            {
                if(StartLMB())
                {
                    str_format(aBuf, sizeof(aBuf), "Tournament has begun!                                                     ");
               	    GameServer()->SendBroadcast(aBuf, -1);
                } else
                {
                    str_format(aBuf, sizeof(aBuf), "Tournament failed to begin!                                                     ");
               	    GameServer()->SendBroadcast(aBuf, -1);
                    Reset();
                }
            }
            break;
        case CGameContext::LMB_IN:
            LMBTick();
            break;
        case CGameContext::LMB_POST:
            CCharacter* pWinnerChar = GameServer()->GetPlayerChar(m_Winner);

           	if(!pWinnerChar || !pWinnerChar->IsAlive())
          		return;

           	m_Pos = vec2(pWinnerChar->m_Pos.x, pWinnerChar->m_Pos.y);
            break;
    }
}

void CLmb::LMBTick()
{
    char aBuf[512];
    m_TimeLeft--;
    int Opponents = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_PLAYING)
		{
            Opponents++;
		}
	}

    if (Server()->Tick() % SERVER_TICK_SPEED == 0)
    {
        for(int i = 0; i < MAX_CLIENTS; i++)
       	{
      		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_PLAYING)
      		{
                str_format(aBuf, sizeof(aBuf), "Opponents left: %d\nTime Left: %ds                                                     ",
                Opponents-1, m_TimeLeft/SERVER_TICK_SPEED);
                GameServer()->SendBroadcast(aBuf, i);
      		}
        }
    }
    if(m_TimeLeft < 1)
    {
        str_format(aBuf, sizeof(aBuf), "Tournament has ended with no winners");
        GameServer()->SendBroadcast(aBuf, -1);
        Reset();
    }

}

void CLmb::Snap(int SnappingClient)
{
    if(NetworkClipped(SnappingClient) || m_Winner == -1)
    	return;

    CCharacter* pWinnerChar = GameServer()->GetPlayerChar(m_Winner);

    int Intensifier = 1;
    if(SnappingClient == m_Winner)
        Intensifier = 2;
    vec2 Velocity = vec2(0,0);

    if(pWinnerChar && pWinnerChar->IsAlive())
        Velocity = pWinnerChar->GetCore().m_Vel;

    CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_ID, sizeof(CNetObj_Flag));
    if(!pFlag)
    	return;

    pFlag->m_X = (int)m_Pos.x+Velocity.x*Intensifier;;
    pFlag->m_Y = (int)m_Pos.y+Velocity.y*Intensifier;;
    pFlag->m_Team = 0;
}
