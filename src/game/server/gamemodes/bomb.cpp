/* (c) heinrich5991 */

#include "bomb.h"

#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/gamecontext.h>

CGameControllerBOMB::CGameControllerBOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "BOMB";
	m_Bomb.m_ClientID = -1;
	m_BombEndTick = -1;
	m_Running = false;
	// Will be executed once only btw;
}

void CGameControllerBOMB::MakeBomb(int ClientID)
{
	m_Bomb.m_ClientID = ClientID;
	GameServer()->SendBroadcast_VL("{str:Player} is the new bomb!", -1, "Player", Server()->ClientName(ClientID));
}

void CGameControllerBOMB::StartBombRound()
{
	m_Bomb.m_ClientID = -1;
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 20;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		GameServer()->m_apPlayers[i]->m_Score++;
	}
}

void CGameControllerBOMB::EndBombRound()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i])
		{
			if(GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
				GameServer()->m_apPlayers[i]->m_Score++;
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_RED);
			GameServer()->m_apPlayers[i]->m_DieTick = 0;
		}
	}
	m_Bomb.m_ClientID = -1;
	EndRound();
}

void CGameControllerBOMB::MakeRandomBomb()
{
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 20;

	int Alive[MAX_CLIENTS];
	int NumAlives = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
			Alive[NumAlives++] = i;

	if(NumAlives)
		MakeBomb(Alive[rand() % NumAlives]);
}

void CGameControllerBOMB::DoWincheck()
{
	if(m_GameOverTick == -1 && !m_Warmup && !GameServer()->m_World.m_ResetRequested)
	{
		int NumPlayers = 0;

		for(int i = 0; i < MAX_CLIENTS; i++)
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_RED)
				NumPlayers++;
		if(!m_Running && NumPlayers > 1)
		{
			m_Running = true;
			EndBombRound();
			return;
		}
		if(!m_SuddenDeath && g_Config.m_SvTimelimit && (Server()->Tick()-m_RoundStartTick) >= g_Config.m_SvTimelimit * Server()->TickSpeed() * 60)
			m_SuddenDeath = true;

		if(m_Running && NumPlayers < 2)
		{
			// m_Running = false;
			EndBombRound();
			return;
		}
	}
}

void CGameControllerBOMB::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{	
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_RED, false, true);
		}
	}
}

void CGameControllerBOMB::OnCharacterSpawn(CCharacter *pChr)
{
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
}

int CGameControllerBOMB::OnCharacterDeath(CCharacter *pChr, CPlayer *pKiller, int Weapon, int Flags)
{
	int id = pChr->GetPlayer()->GetCID();
	if(id == m_Bomb.m_ClientID)
	{
		GameServer()->CreateExplosion(pChr->m_Pos, m_Bomb.m_ClientID, WEAPON_WORLD, true, -1);
		GameServer()->CreateSound(pChr->m_Pos, SOUND_GRENADE_EXPLODE);
	}
	pChr->GetPlayer()->SetTeam(TEAM_SPECTATORS);
	GameServer()->m_apPlayers[id]->m_DieTick = 0;
	return 0;
}

void CGameControllerBOMB::Tick()
{
	if(m_Bomb.m_ClientID != -1 && (!GameServer()->m_apPlayers[m_Bomb.m_ClientID] || GameServer()->m_apPlayers[m_Bomb.m_ClientID]->GetTeam() == TEAM_SPECTATORS))
		m_Bomb.m_ClientID = -1;

	if(m_GameOverTick == -1 && m_Running)
	{
		if(!m_Warmup && (m_Bomb.m_ClientID == -1 ||
			!GameServer()->m_apPlayers[m_Bomb.m_ClientID]))
			MakeRandomBomb();
		else if(!m_Warmup)
		{
			if(m_Bomb.m_Tick)
				m_Bomb.m_Tick--;

			if(m_Bomb.m_Tick <= 0)
			{
				// CCharacter* pChr = GameServer()->m_apPlayers[m_Bomb.m_ClientID]->GetCharacter();
				// GameServer()->CreateExplosion(pChr->m_Pos, m_Bomb.m_ClientID, WEAPON_WORLD, true, -1);
				// GameServer()->CreateSound(pChr->m_Pos, SOUND_GRENADE_EXPLODE);
				GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter(); // Must be spectator after
				MakeRandomBomb();
			}
			else if(m_Bomb.m_Tick % SERVER_TICK_SPEED == 0)
			{
				GameServer()->CreateSoundGlobal(SOUND_HOOK_NOATTACH);
				GameServer()->CreateDamageInd(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, 0, m_Bomb.m_Tick / SERVER_TICK_SPEED);
			}
		}
	} else if (!m_Running)
	{
		if(Server()->Tick() % SERVER_TICK_SPEED == 0)
		{
			int tick = (Server()->Tick()/SERVER_TICK_SPEED)%4;		
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "%s", tick == 0 ? "." : tick == 1 ? ".." : tick == 2 ? "..." : "");
			GameServer()->SendBroadcast_VL("Waiting for players{str:Dots}", -1, "Dots", aBuf);
		}
	}
	DoWincheck();
	IGameController::Tick();
}
