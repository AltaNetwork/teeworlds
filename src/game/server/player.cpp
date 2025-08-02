#include <new>
#include <engine/shared/config.h>
#include "player.h"
#include "entities/character.h"
#include "tournament.h"

MACRO_ALLOC_POOL_ID_IMPL(CPlayer, MAX_CLIENTS)

IServer *CPlayer::Server() const { return m_pGameServer->Server(); }

CPlayer::CPlayer(CGameContext *pGameServer, int ClientID, int Team)
{
	m_pGameServer = pGameServer;
	m_DieTick = Server()->Tick();
	m_ScoreStartTick = Server()->Tick();
	m_pCharacter = 0;
	m_ClientID = ClientID;
	m_Team = GameServer()->m_pController->ClampTeam(Team);
	m_SpectatorID = SPEC_FREEVIEW;
	m_LastActionTick = Server()->Tick();
	m_TeamChangeTick = Server()->Tick();

	m_SpawnTeam = 0;
	m_DuelPlayer = -1;
	m_DuelScore = 0;
	m_InvitedBy = -1;

	m_Cosmetics = 0;
	m_Settings = SETTINGS_BEYONDZOOM;
	m_WTeam = 0;

	m_LMBState = LMB_STANDBY;

	SetLanguage(Server()->GetClientLanguage(ClientID));

	m_Authed = IServer::AUTHED_NO;

	m_PrevTuningParams = *pGameServer->Tuning();
	m_NextTuningParams = m_PrevTuningParams;

	int* idMap = Server()->GetIdMap(ClientID);
	for (int i = 1;i < VANILLA_MAX_CLIENTS;i++)
	{
	    idMap[i] = -1;
	}
	idMap[0] = ClientID;
}

CPlayer::~CPlayer()
{
	delete m_pCharacter;
	m_pCharacter = 0;
}

void CPlayer::HandleTuningParams()
{
	if(m_PrevTuningParams != m_NextTuningParams)
	{
		if(m_IsReady)
		{
			CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
			int *pParams = (int *)&m_NextTuningParams;
			for(unsigned i = 0; i < sizeof(m_NextTuningParams)/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
			Server()->SendMsg(&Msg, MSGFLAG_VITAL, GetCID());
		}

		m_PrevTuningParams = m_NextTuningParams;
	}

	m_NextTuningParams = *GameServer()->Tuning();
}

void CPlayer::Tick()
{
	if(!Server()->ClientIngame(m_ClientID))
		return;

	Server()->SetClientScore(m_ClientID, m_Score);
	Server()->SetClientLanguage(m_ClientID, m_aLanguage);

	// do latency stuff
	{
		IServer::CClientInfo Info;
		if(Server()->GetClientInfo(m_ClientID, &Info))
		{
			m_Latency.m_Accum += Info.m_Latency;
			m_Latency.m_AccumMax = max(m_Latency.m_AccumMax, Info.m_Latency);
			m_Latency.m_AccumMin = min(m_Latency.m_AccumMin, Info.m_Latency);
		}
		// each second
		if(Server()->Tick()%Server()->TickSpeed() == 0)
		{
			m_Latency.m_Avg = m_Latency.m_Accum/Server()->TickSpeed();
			m_Latency.m_Max = m_Latency.m_AccumMax;
			m_Latency.m_Min = m_Latency.m_AccumMin;
			m_Latency.m_Accum = 0;
			m_Latency.m_AccumMin = 1000;
			m_Latency.m_AccumMax = 0;
		}
	}

	if(!GameServer()->m_World.m_Paused)
	{
	    int Event = PlayerEvent();
	    if(Event == EVENT_DUEL)
			DuelTick();
		if(m_Team != TEAM_SPECTATORS)
		{
    		if(m_pCharacter)
    			m_WTeam = m_pCharacter->GetCore().m_VTeam;
		} else {
            if(m_SpectatorID != SPEC_FREEVIEW)
                m_WTeam = GameServer()->m_apPlayers[m_SpectatorID]->m_WTeam;
            else
                m_WTeam = -1;
		}

		if(!m_pCharacter && (m_DieTick+Server()->TickSpeed()*3 <= Server()->Tick() || PlayerEvent() != EVENT_NONE))
			m_Spawning = true;

		if(m_pCharacter)
		{
			if(m_pCharacter->IsAlive())
				m_ViewPos = m_pCharacter->m_Pos;
			else
			{
				delete m_pCharacter;
				m_pCharacter = 0;
			}
		}
		else if(m_Spawning)
			TryRespawn();
		if(m_Team == TEAM_SPECTATORS && m_SpectatorID == SPEC_FREEVIEW)
			m_ViewPos = vec2(m_LatestActivity.m_TargetX, m_LatestActivity.m_TargetY);
	}
	else
	{
		++m_DieTick;
		++m_ScoreStartTick;
		++m_LastActionTick;
		++m_TeamChangeTick;
 	}

	HandleTuningParams();
}

void CPlayer::PostTick()
{
	// update latency value
	if(m_PlayerFlags&PLAYERFLAG_SCOREBOARD)
	{
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				m_aActLatency[i] = GameServer()->m_apPlayers[i]->m_Latency.m_Min;
		}
	}

	// update view pos for spectators
	if(m_Team == TEAM_SPECTATORS && m_SpectatorID != SPEC_FREEVIEW && GameServer()->m_apPlayers[m_SpectatorID])
		m_ViewPos = GameServer()->m_apPlayers[m_SpectatorID]->m_ViewPos;
}

void CPlayer::Snap(int SnappingClient)
{
	if(!Server()->ClientIngame(m_ClientID))
		return;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, m_ClientID, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

    char aBufName[16];
    snprintf(aBufName, sizeof(aBufName), "%s%s", Server()->ClientName(m_ClientID), ProccessName());
    StrToInts(&pClientInfo->m_Name0, 4, aBufName);

    char aBufClan[12];
    snprintf(aBufClan, sizeof(aBufClan), "%s", ProccessClan());
    StrToInts(&pClientInfo->m_Clan0, 4, aBufClan);

    int Colour = 0xff32 + GameServer()->m_pController->m_RainbowColor * 0x010000; // rainbow colour
	StrToInts(&pClientInfo->m_Skin0, 6, ProccessSkin());
	pClientInfo->m_UseCustomColor = m_Cosmetics < 5 && m_Cosmetics > 0 ? true : m_TeeInfos.m_UseCustomColor; // has to be on if any cosmetics is on
	pClientInfo->m_ColorBody = m_Cosmetics&COSM_RAINBOW ? Colour : m_TeeInfos.m_ColorBody;
	pClientInfo->m_ColorFeet = m_Cosmetics&COSM_RAINBOWFEET ? Colour : m_TeeInfos.m_ColorFeet;

	CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, m_ClientID, sizeof(CNetObj_PlayerInfo)));
	if(!pPlayerInfo)
		return;

	pPlayerInfo->m_Latency = SnappingClient == -1 ? m_Latency.m_Min : GameServer()->m_apPlayers[SnappingClient]->m_aActLatency[m_ClientID];
	pPlayerInfo->m_Local = 0;
	pPlayerInfo->m_ClientID = m_ClientID;
	pPlayerInfo->m_Score = m_Score;
	pPlayerInfo->m_Team = SnappingClient == m_ClientID ? TEAM_RED : m_Team;
	pPlayerInfo->m_Local = m_ClientID == SnappingClient ? 1 : 0;

	if(m_ClientID == SnappingClient && m_Team == TEAM_SPECTATORS)
	{
		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, m_ClientID, sizeof(CNetObj_SpectatorInfo)));
		if(!pSpectatorInfo)
			return;

		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
		pSpectatorInfo->m_X = m_ViewPos.x;
		pSpectatorInfo->m_Y = m_ViewPos.y;
	}

	// WARNING, this is very hardcoded; for ddnet client support
	CNetObj_DDNetPlayer *pDDNetPlayer = (CNetObj_DDNetPlayer *)Server()->SnapNewItem(32765, GetCID(), 8);
	if(!pDDNetPlayer)
		return;

	if(m_Team == TEAM_SPECTATORS)
    	pDDNetPlayer->m_Flags |= EXPLAYERFLAG_PAUSED;
}

void CPlayer::FakeSnap(int SnappingClient)
{
	IServer::CClientInfo info;
	Server()->GetClientInfo(SnappingClient, &info);
	if (info.m_CustClt)
		return;

	int id = VANILLA_MAX_CLIENTS - 1;

	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, id, sizeof(CNetObj_ClientInfo)));

	if(!pClientInfo)
		return;

	StrToInts(&pClientInfo->m_Name0, 4, " ");
	StrToInts(&pClientInfo->m_Clan0, 3, Server()->ClientClan(m_ClientID));
	StrToInts(&pClientInfo->m_Skin0, 6, m_TeeInfos.m_SkinName);
}

void CPlayer::OnDisconnect(const char *pReason)
{
	KillCharacter(WEAPON_SELF, FLAG_ENDDUEL);

	if(Server()->ClientIngame(m_ClientID))
	{
		char aBuf[512];
		if(pReason && *pReason)
		{
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game (%s)", Server()->ClientName(m_ClientID), pReason);
			GameServer()->SendChatTarget(-1, _("'%s' has left the game ({str:Reason})"), "PlayerName", Server()->ClientName(m_ClientID), "Reason", pReason);
		}
		else
		{
			str_format(aBuf, sizeof(aBuf), "'%s' has left the game", Server()->ClientName(m_ClientID));
			GameServer()->SendChatTarget(-1, _("'{str:PlayerName}' has left the game"), "PlayerName", Server()->ClientName(m_ClientID));
		}

		str_format(aBuf, sizeof(aBuf), "leave player='%d:%s'", m_ClientID, Server()->ClientName(m_ClientID));
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "game", aBuf);
	}
}

void CPlayer::OnPredictedInput(CNetObj_PlayerInput *NewInput)
{
	// skip the input if chat is active
	if(m_Team == TEAM_SPECTATORS || ((m_PlayerFlags&PLAYERFLAG_CHATTING && (NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING))))
		return;

	if(m_pCharacter)
		m_pCharacter->OnPredictedInput(NewInput);
}

void CPlayer::OnDirectInput(CNetObj_PlayerInput *NewInput)
{
	if(NewInput->m_PlayerFlags&PLAYERFLAG_CHATTING)
	{
		// skip the input if chat is active
		if(m_PlayerFlags&PLAYERFLAG_CHATTING || m_Team == TEAM_SPECTATORS)
			return;

		// reset input
		// if(m_pCharacter)
		// 	m_pCharacter->ResetInput();

		m_PlayerFlags = NewInput->m_PlayerFlags;
 		return;
	}

	m_PlayerFlags = NewInput->m_PlayerFlags;

	if(m_pCharacter)
		m_pCharacter->OnDirectInput(NewInput);

	m_LatestActivity.m_TargetX = NewInput->m_TargetX;
	m_LatestActivity.m_TargetY = NewInput->m_TargetY;

	if(!m_pCharacter && m_Team != TEAM_SPECTATORS && (NewInput->m_Fire&1))
		m_Spawning = true;

	// check for activity
	if(NewInput->m_Direction || m_LatestActivity.m_TargetX != NewInput->m_TargetX ||
		m_LatestActivity.m_TargetY != NewInput->m_TargetY || NewInput->m_Jump ||
		NewInput->m_Fire&1 || NewInput->m_Hook)
	{
		// m_LatestActivity.m_TargetX = NewInput->m_TargetX;
		// m_LatestActivity.m_TargetY = NewInput->m_TargetY;
		m_LastActionTick = Server()->Tick();
	}
}

CCharacter *CPlayer::GetCharacter()
{
	if(m_pCharacter && m_pCharacter->IsAlive())
		return m_pCharacter;
	return 0;
}

void CPlayer::KillCharacter(int Weapon, int Flags)
{
	if(m_pCharacter)
	{
		m_pCharacter->Die(m_ClientID, Weapon, Flags);
		delete m_pCharacter;
		m_pCharacter = 0;
	}
}

void CPlayer::Respawn()
{
	if(m_Team != TEAM_SPECTATORS)
		m_Spawning = true;
}

void CPlayer::SetTeam(int Team, bool DoChatMsg, bool KillChr)
{
	// clamp the team
	Team = GameServer()->m_pController->ClampTeam(Team);
	if(m_Team == Team)
		return;

	// if(DoChatMsg)
	// {
	//     char aBuf[512];
	// 	if(Team == TEAM_SPECTATORS)
	// 	{
	// 		GameServer()->SendChatTarget(-1, _("'{str:Player}' joined the spectators"),"Player", Server()->ClientName(m_ClientID));
	// 	// }else if(Team == TEAM_RED && GameServer()->m_pController->IsTeamplay())
	// 	// {
	// 	// 	GameServer()->SendChatTarget(-1, _("'{str:Player}' joined the redteam"),"Player", Server()->ClientName(m_ClientID));
	// 	// }else if(Team == TEAM_BLUE && GameServer()->m_pController->IsTeamplay())
	// 	// {
	// 	// 	GameServer()->SendChatTarget(-1, _("'{str:Player}' joined the blueteam"),"Player", Server()->ClientName(m_ClientID));
	// 	}else
	// 	{
	// 		GameServer()->SendChatTarget(-1, _("'{str:Player}' joined the game"),"Player", Server()->ClientName(m_ClientID));
	// 	}
	// }

	// if(KillChr)
	//     KillCharacter();

	m_Team = Team;
	m_LastActionTick = Server()->Tick();
	// m_SpectatorID = SPEC_FREEVIEW;
	// str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' m_Team=%d", m_ClientID, Server()->ClientName(m_ClientID), m_Team);
	// GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// GameServer()->m_pController->OnPlayerInfoChange(GameServer()->m_apPlayers[m_ClientID]);

	// if(Team == TEAM_SPECTATORS)
	// {
	// 	// update spectator modes
	// 	for(int i = 0; i < MAX_CLIENTS; ++i)
	// 	{
	// 		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->m_SpectatorID == m_ClientID)
	// 			GameServer()->m_apPlayers[i]->m_SpectatorID = SPEC_FREEVIEW;
	// 	}
	// }
}

void CPlayer::TryRespawn()
{
	vec2 SpawnPos;

	if(!GameServer()->m_pController->CanSpawn(m_SpawnTeam, &SpawnPos))
		return;

	m_Spawning = false;
	m_pCharacter = new(m_ClientID) CCharacter(&GameServer()->m_World);
	m_pCharacter->Spawn(this, SpawnPos);
	// GameServer()->CreatePlayerSpawn(SpawnPos);
}

const char* CPlayer::GetLanguage()
{
	return m_aLanguage;
}

void CPlayer::SetLanguage(const char* pLanguage)
{
	str_copy(m_aLanguage, pLanguage, sizeof(m_aLanguage));
}

int CPlayer::PlayerEvent()
{
    if(m_DuelPlayer != -1 && GameServer()->m_apPlayers[m_DuelPlayer])
    {
        if(m_DuelPlayer != -1)
            return EVENT_DUEL;
    }
    return EVENT_NONE;
}

void CPlayer::DuelTick()
{
    if(m_ClientID == m_DuelPlayer ||
        GameServer()->m_apPlayers[m_DuelPlayer]->m_DuelPlayer != m_ClientID)
    {
        m_DuelPlayer = -1;
        m_SpawnTeam = 0;
      	char Buf[256];
		str_format(Buf, sizeof(Buf), "error occured! please report to admins.");
		GameServer()->SendBroadcast(Buf, m_ClientID);
        return;
    }

    m_SpawnTeam = 1;
    SetTeam(TEAM_RED);

    if(GameServer()->m_apPlayers[m_DuelPlayer]->GetCharacter() && m_pCharacter)
    {
        CCharacter *DuelPlayer = GameServer()->m_apPlayers[m_DuelPlayer]->GetCharacter();
        if(m_pCharacter->m_DeepFrozen && m_pCharacter->IsGrounded()
           && DuelPlayer->m_DeepFrozen && DuelPlayer->IsGrounded())
        {
            KillCharacter(WEAPON_GAME);
            GameServer()->m_apPlayers[m_DuelPlayer]->KillCharacter(WEAPON_GAME);
            GameServer()->SendChatTarget(m_DuelPlayer, _("Draw!"));
            GameServer()->SendChatTarget(GetCID(), _("Draw!"));
        }

    }

    if(Server()->Tick()%SERVER_TICK_SPEED == 1)
	{
		char Buf[256];
		str_format(Buf, sizeof(Buf), "%s: %d\n%s: %d                                                  ",
            Server()->ClientName(m_ClientID), m_DuelScore,
            Server()->ClientName(m_DuelPlayer), GameServer()->m_apPlayers[m_DuelPlayer]->m_DuelScore);
		GameServer()->SendBroadcast(Buf, m_ClientID);
	}
}

const char* CPlayer::ProccessName() const
{
    static char s_aBuf[24]; // Static to avoid returning pointer to local, but not thread-safe if called concurrently for different players

    str_format(s_aBuf, sizeof(s_aBuf), "");

    return s_aBuf;
}

const char* CPlayer::ProccessClan() const
{
    static char s_aBuf[24]; // Static to avoid returning pointer to local

    if (m_PlayerFlags & PLAYERFLAG_CHATTING) {
        switch ((Server()->Tick() / (SERVER_TICK_SPEED / 3) + m_ClientID) % 4) {
            case 3: str_format(s_aBuf, sizeof(s_aBuf), "...✍"); break;
            case 2: str_format(s_aBuf, sizeof(s_aBuf), "..✍"); break;
            case 1: str_format(s_aBuf, sizeof(s_aBuf), ".✍"); break;
            case 0: str_format(s_aBuf, sizeof(s_aBuf), "✍"); break;
            default: str_format(s_aBuf, sizeof(s_aBuf), ""); break; // Should not happen
        }
    } else if (m_PlayerFlags & PLAYERFLAG_IN_MENU) {
        switch ((Server()->Tick() / (SERVER_TICK_SPEED / 2) + m_ClientID) % 4) {
            case 3: str_format(s_aBuf, sizeof(s_aBuf), "zzz"); break;
            case 2: str_format(s_aBuf, sizeof(s_aBuf), "zz"); break;
            case 1: str_format(s_aBuf, sizeof(s_aBuf), "z"); break;
            default: str_format(s_aBuf, sizeof(s_aBuf), ""); break; // Should not happen
        }
    } else {
        str_format(s_aBuf, sizeof(s_aBuf), Server()->ClientClan(m_ClientID)); // No animation
    }
    return s_aBuf;
}

const char* CPlayer::ProccessSkin() const
{
    static char s_aBuf[24]; // Static to avoid returning pointer to local
    str_format(s_aBuf, sizeof(s_aBuf), "default");
    str_format(s_aBuf, sizeof(s_aBuf), m_TeeInfos.m_SkinName);
    // if(m_Cosmetics&COSM_RANDOMSKINSANTA)
    //     str_format(s_aBuf, sizeof(s_aBuf), aSkinsSanta[Server()->Tick()/50%15]);
    //
    //
    // if(m_Cosmetics&COSM_RANDOMSKIN)
    //     str_format(s_aBuf, sizeof(s_aBuf), aSkins[Server()->Tick()/50%15]);
        //
        //
        //
    // if(m_Cosmetics&COSM_RANDOMSKINCOALA)
    //     str_format(s_aBuf, sizeof(s_aBuf), aSkinsCoala[Server()->Tick()/50%15]);
    // if(m_Cosmetics&COSM_RANDOMSKINKITTY)
    //     str_format(s_aBuf, sizeof(s_aBuf), aSkinsKitty[Server()->Tick()/50%15]);

	// float fval = 256-abs(cos(Server()->Tick()/50.0f)) * 256.0f;
	// int m_PulseColor = static_cast<int>(fval);

	// BaseColor = 96000;
	// if(m_Cosmetics&COSM_PULSEREDFEET)
 //        pClientInfo->m_ColorFeet = BaseColor + m_PulseColor;
    return s_aBuf;
}
