#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include <memory>
#include "lmb.h"
#include "../character.h"

CLmb::CLmb(CGameWorld *pGameWorld)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LMB)
{
    m_WTeam = 0;
    m_Winner = -1;
    m_TimeLeft = g_Config.m_SvLMBTime * SERVER_TICK_SPEED;
    m_RegTimeLeft = g_Config.m_SvLMBRegTime * SERVER_TICK_SPEED;
    m_IsTeamfight = false;
    m_Finished = false;

    GameWorld()->InsertEntity(this);

    if(GameServer()->m_LMBState != CGameContext::LMB_NAN)
    {
        GameServer()->m_LMBState = CGameContext::LMB_CANCEL;
    } else {
        GameServer()->m_LMBState = CGameContext::LMB_REG;
        GameServer()->m_LastLMB = Server()->Tick();
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
        	if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_REG)
        	{
                GameServer()->m_apPlayers[i]->m_LMBState = CPlayer::LMB_STANDBY;
        	}
        }
    }
}

void CLmb::Reset()
{
	GameWorld()->DestroyEntity(this);
    GameServer()->m_LMBState = CGameContext::LMB_NAN;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_PLAYING)
        {
            GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_SELF);
        }
    }
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
		    GameServer()->m_apPlayers[i]->m_SpawnTeam = m_IsTeamfight ? 1 : 255;
		    GameServer()->m_apPlayers[i]->KillCharacter(WEAPON_SELF);
			GameServer()->m_apPlayers[i]->m_DieTick = 0;
			GameServer()->m_apPlayers[i]->m_SpawnVTeam = GameServer()->m_pController->VTeamDuel(MAX_CLIENTS+1, MAX_CLIENTS);;
		    GameServer()->m_apPlayers[i]->m_LMBState = CPlayer::LMB_PLAYING;
			GameServer()->m_apPlayers[i]->m_Effects = CPlayer::EFFECT_BLIND;
		}
	}
    return true;
}

void CLmb::Tick()
{
    char aBuf[512];
    if(!m_Finished)
    {
        switch(GameServer()->m_LMBState)
        {
            case CGameContext::LMB_REG:
                m_RegTimeLeft--;
                if (Server()->Tick() % SERVER_TICK_SPEED == 0)
                {
                    char Buf[256];
                    int Registers = 0;
                    for(int i = 0; i < MAX_CLIENTS; i++)
                    {
                        if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_REG)
                            Registers++;
                    }
                    str_format(aBuf, sizeof(Buf), "%d", m_RegTimeLeft/SERVER_TICK_SPEED);
                    str_format(Buf, sizeof(Buf), "%d", Registers);
                    GameServer()->SendBroadcast_VL(_("Tournament is starting in {str:Time}s\n{str:Amnt} subscribed{str:BL}"), -1,
                    "Time", aBuf, "Amnt", Buf, "BL", "                                                          ");
                }
                if(m_RegTimeLeft < 1)
                {
                    if(StartLMB())
                    {
                        str_format(aBuf, sizeof(aBuf), "                                                     ");
                        GameServer()->SendBroadcast_VL(_("Tournament has begun!{str:BL}"), -1, "BL", aBuf);
                    } else
                    {
                        str_format(aBuf, sizeof(aBuf), "                                                     ");
                        GameServer()->SendBroadcast_VL(_("Tournament failed to begin!{str:BL}"), -1, "BL", aBuf);
                        Reset();
                    }
                }
                break;
            case CGameContext::LMB_IN:
                LMBTick();
                break;
            case CGameContext::LMB_CANCEL:
                GameServer()->SendBroadcast_VL(_("Tournament was canceled"), -1);
                GameWorld()->PurgeEntityType(CGameWorld::ENTTYPE_LMB);
                break;
        }
        return;
    }
    if(!GameServer()->m_apPlayers[m_Winner] || GameServer()->m_apPlayers[m_Winner]->m_LMBState != CPlayer::LMB_STANDBY)
    {
        Reset();
        return;
    }
    CCharacter* pWinnerChar = GameServer()->GetPlayerChar(m_Winner);

    if(!pWinnerChar || !pWinnerChar->IsAlive())
        return;

    m_WTeam = pWinnerChar->GetVTeam();
    m_Pos = vec2(pWinnerChar->m_Pos.x, pWinnerChar->m_Pos.y);
}

void CLmb::LMBTick()
{
    m_TimeLeft--;
    int Opponents = 0;
    for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_PLAYING)
		{
            Opponents++;
		}
	}

    char aBuf[256];
    char sBuf[128];
    char Buf[128];

    if (Server()->Tick() % SERVER_TICK_SPEED == 0)
    {
        for(int i = 0; i < MAX_CLIENTS; i++)
       	{
      		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_PLAYING)
      		{
                str_format(aBuf, sizeof(aBuf), "                                                       ");
                str_format(Buf, sizeof(Buf), "%d", m_TimeLeft/SERVER_TICK_SPEED);
                str_format(sBuf, sizeof(Buf), "%d", Opponents-1);
               	GameServer()->SendBroadcast_VL(_("Opponents left: {str:Amnt}\nTime Left: {str:Time}s{str:BL}"), i,
                "Amnt", sBuf, "Time", Buf, "BL", aBuf);
      		}
        }
    }
    if(Opponents < 2)
    {
        for(int i = 0; i < MAX_CLIENTS; i++)
    	{
    		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_LMBState == CPlayer::LMB_PLAYING)
                m_Winner = i;
    	}
        if(m_Winner == -1)
        {
            Reset();
            return;
        }
        GameServer()->m_LMBState = CGameContext::LMB_NAN;
        GameServer()->SendBroadcast_VL(_("{str:Player} has won the Tournament"), -1, "Player", Server()->ClientName(m_Winner));
        GameServer()->m_apPlayers[m_Winner]->KillCharacter(WEAPON_SELF, FLAG_NOKILLMSG);
        m_Finished = true;
        return;
    }
    if(m_TimeLeft < 1 || Opponents < 1)
    {
       	GameServer()->SendBroadcast_VL(_("Tournament has ended with no winners"), -1);
        Reset();
    }

}

void CLmb::Snap(int SnappingClient)
{
    if(NetworkClipped(SnappingClient) || m_Winner == -1)
    	return;

    CCharacter* pWinnerChar = GameServer()->GetPlayerChar(m_Winner);

    int Intensifier = 1;
    vec2 Velocity = vec2(0,0);

    if(pWinnerChar && pWinnerChar->IsAlive())
        Velocity = pWinnerChar->GetCore().m_Vel;
    else
        return;

    if(SnappingClient == m_Winner)
        Intensifier = 2;

    CNetObj_Flag *pFlag = (CNetObj_Flag *)Server()->SnapNewItem(NETOBJTYPE_FLAG, m_ID, sizeof(CNetObj_Flag));
    if(!pFlag)
    	return;

    pFlag->m_X = (int)m_Pos.x+Velocity.x*Intensifier;
    pFlag->m_Y = (int)m_Pos.y+Velocity.y*Intensifier;
    pFlag->m_Team = 0;
}
