/* (c) heinrich5991 */

#include "bomb.h"

#include <engine/shared/config.h>

#include <game/mapitems.h>
#include <game/server/gamecontext.h>

CGameControllerBOMB::CGameControllerBOMB(CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "BOMB      S-DDR";
	m_Bomb.m_ClientID = -1;
	m_BombEndTick = -1;
	m_Running = false;
	// for(int i = 0; i < MAX_CLIENTS; i++)
	// 	m_aClients[i].m_State = STATE_ACTIVE;
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
		// if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State >= STATE_ALIVE)
		GameServer()->m_apPlayers[i]->m_Score++;
		// if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
		// {
		// 	m_aClients[i].m_State = STATE_ALIVE;
		// }
	}
}

void CGameControllerBOMB::EndBombRound()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State >= STATE_ALIVE)
			GameServer()->m_apPlayers[i]->m_Score++;
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State == STATE_ACTIVE)
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_RED, true);
			GameServer()->m_apPlayers[i]->Respawn();
			m_aClients[i].m_State = STATE_ALIVE;
			GameServer()->m_apPlayers[i]->m_Score++;
		}
	}
	EndRound();
}

void CGameControllerBOMB::MakeRandomBomb()
{
	m_Bomb.m_Tick = SERVER_TICK_SPEED * 20;

	int Alive[MAX_CLIENTS];
	int NumAlives = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i] && m_aClients[i].m_State >= STATE_ALIVE)
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
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() != TEAM_SPECTATORS)
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
			EndBombRound();
			return;
		}
	}
}

void CGameControllerBOMB::PostReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
		if(GameServer()->m_apPlayers[i])
		{
			GameServer()->m_apPlayers[i]->SetTeam(TEAM_RED);
			GameServer()->m_apPlayers[i]->m_DieTick = 0;
		}
}

void CGameControllerBOMB::OnCharacterSpawn(CCharacter *pChr)
{
	pChr->GiveWeapon(WEAPON_HAMMER, -1);
	pChr->GetPlayer()->m_Effects = CPlayer::EFFECT_NORESPAWN;
}

int CGameControllerBOMB::OnCharacterDeath(CCharacter *pChr, CPlayer *pKiller, int Weapon)
{
	if(m_aClients[pChr->GetPlayer()->GetCID()].m_State >= STATE_ACTIVE)
	{
		// GameServer()->SendBroadcast("You will automatically rejoin the game when the round is over", pChr->GetPlayer()->GetCID());
		m_aClients[pChr->GetPlayer()->GetCID()].m_State = STATE_ACTIVE;
		GameServer()->CreateExplosion(pChr->m_Pos, m_Bomb.m_ClientID, WEAPON_GAME, false);
		GameServer()->CreateSound(pChr->m_Pos, SOUND_GRENADE_EXPLODE);
	}
	return 0;
}

// void CGameControllerBOMB::OnPlayerInfoChange(CPlayer *pPlayer)
// {
// 	// if(m_Bomb.m_ClientID == pPlayer->GetCID())
// 	// {
// 	// 	str_copy(pPlayer->m_TeeInfos.m_SkinName, "bomb", sizeof(pPlayer->m_TeeInfos.m_SkinName));
// 	// 	pPlayer->m_TeeInfos.m_UseCustomColor = 0;
// 	// }
// 	// else
// 	// {
// 	// 	str_copy(pPlayer->m_TeeInfos.m_SkinName, "cammostripes", sizeof(pPlayer->m_TeeInfos.m_SkinName));
// 	// 	pPlayer->m_TeeInfos.m_UseCustomColor = 1;
// 	// 	pPlayer->m_TeeInfos.m_ColorBody = 16777215;
// 	// 	pPlayer->m_TeeInfos.m_ColorFeet = 16777215;
// 	// }
// }

void CGameControllerBOMB::Tick()
{
	if(m_Bomb.m_ClientID != -1 && (!GameServer()->m_apPlayers[m_Bomb.m_ClientID] || GameServer()->m_apPlayers[m_Bomb.m_ClientID]->GetTeam() == TEAM_SPECTATORS))
		m_Bomb.m_ClientID = -1;

	if(m_GameOverTick == -1 && m_Running)
	{
		if(!m_Warmup && m_Bomb.m_ClientID == -1)
			MakeRandomBomb();
		else if(!m_Warmup)
		{
			if(m_Bomb.m_Tick)
				m_Bomb.m_Tick--;

			// for(int i = 0; i < MAX_CLIENTS; i++)
			// 	if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetCharacter())
					// GameServer()->m_apPlayers[i]->GetCharacter()->SetHealth(m_Bomb.m_Tick / SERVER_TICK_SPEED + 1);

			if(m_Bomb.m_Tick <= 0)
			{
				// GameServer()->SendBroadcast("BOOM!", -1);
				GameServer()->m_apPlayers[m_Bomb.m_ClientID]->KillCharacter(); // Must be spectator after
			}
			else if(m_Bomb.m_Tick % SERVER_TICK_SPEED == 0)
			{
				// char buf[96];
				// str_format(buf,sizeof(buf), "\n\n\n\n\n\n\n\n\n\n%d", m_Bomb.m_Tick / SERVER_TICK_SPEED);
				// str_format(buf,sizeof(buf), "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n%d\n%s", m_Bomb.m_Tick / SERVER_TICK_SPEED, Server()->ClientName(m_Bomb.m_ClientID));
				// GameServer()->SendBroadcast(buf, -1);
				GameServer()->CreateSoundGlobal(SOUND_HOOK_NOATTACH);
				GameServer()->CreateDamageInd(GameServer()->m_apPlayers[m_Bomb.m_ClientID]->m_ViewPos, 0, m_Bomb.m_Tick / SERVER_TICK_SPEED);
			}
		}
	}

	DoWincheck();
	IGameController::Tick();
}
