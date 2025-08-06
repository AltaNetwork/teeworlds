#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "duel.h"

CDuel::CDuel(CGameWorld *pGameWorld, int Opponent, int Inviter, int Wager)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_DUEL)
{
    m_Player = Inviter;
    m_Opponent = Opponent;

    m_PlayerPoints = 0;
    m_OpponentPoints = 0;
    m_Wager = Wager;

    m_Started = false;
    m_AutoCancelTick = SERVER_TICK_SPEED * 3;

    GameWorld()->InsertEntity(this);

    if(!GameServer()->m_apPlayers[m_Opponent] || !GameServer()->m_apPlayers[m_Player] || m_Opponent == m_Player)
    {
        Reset();
        return;
    }

    str_format(BufPlayer, sizeof(BufPlayer), "%s", Server()->ClientName(m_Player));
    str_format(BufOpponent, sizeof(BufOpponent), "%s", Server()->ClientName(m_Opponent));

   	char Buf[32];
	str_format(Buf, sizeof(Buf), "%d", m_Wager);
    GameServer()->SendChatTarget(m_Player, _("Match request sent to '{str:Player}' of {str:Wager} wager"), "Player", Server()->ClientName(m_Opponent), "Wager", Buf);
    GameServer()->SendChatTarget(m_Opponent, _("'{str:Player}' sent you a match request of {str:Wager} wager"), "Player", Server()->ClientName(m_Player), "Wager", Buf);
    GameServer()->SendChatTarget(m_Opponent, _("Use /accept to fight"));
}

void CDuel::ResetPlayer(int p_ID)
{
    CPlayer *m_pPlayer = GameServer()->m_apPlayers[p_ID];
    m_pPlayer->m_SpawnTeam = 0;
    m_pPlayer->m_DuelFlags = 0;
    m_pPlayer->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
    GameServer()->SendBroadcast(" ", p_ID);
}

void CDuel::PreparePlayer(int p_ID)
{
    CPlayer *m_pPlayer = GameServer()->m_apPlayers[p_ID];
    m_pPlayer->m_SpawnTeam = 1;
    m_pPlayer->m_DuelFlags = CPlayer::DUEL_INDUEL;
    m_pPlayer->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
    m_pPlayer->m_DieTick = 0;
}

void CDuel::Reset()
{
    if(m_Started)
        EndDuel();
    GameWorld()->DestroyEntity(this);
}

void CDuel::EndDuel()
{
    if(GameServer()->m_apPlayers[m_Player])
    {
        str_format(BufPlayer, sizeof(BufPlayer), "%s", Server()->ClientName(m_Player));
        ResetPlayer(m_Player);
    }
    if(GameServer()->m_apPlayers[m_Opponent])
    {
        str_format(BufOpponent, sizeof(BufOpponent), "%s", Server()->ClientName(m_Opponent));
        ResetPlayer(m_Opponent);
    }
   	char Buf[32];
	str_format(Buf, sizeof(Buf), "%d - %d", m_PlayerPoints, m_OpponentPoints);
    GameServer()->SendChatTarget(-1 , _("Match '{str:Player}' - '{str:Opponent}' ended with results of {str:Result}"),
        "Player", BufPlayer, "Opponent", BufOpponent, "Result", Buf);
}

void CDuel::Tick()
{
    CPlayer *m_pOpponent = GameServer()->m_apPlayers[m_Opponent];
    CPlayer *m_pNonOpponent = GameServer()->m_apPlayers[m_Player];
    if((m_pNonOpponent == nullptr || m_pOpponent == nullptr) || // Someone disconnected
        (m_pNonOpponent->m_DuelFlags&CPlayer::DUEL_LEAVE || m_pOpponent->m_DuelFlags&CPlayer::DUEL_LEAVE) || // Someone left
        (m_PlayerPoints >= 10 || m_OpponentPoints >= 10 )) // Someone won
    {
        Reset();
        return;
    }

    if(!m_Started)
    {
        m_AutoCancelTick--;
        if(m_pOpponent->m_DuelFlags&CPlayer::DUEL_ACCEPT)
        {
            PreparePlayer(m_Opponent);
            PreparePlayer(m_Player);
            m_Started = true;
        }
        if(m_AutoCancelTick <= 0)
        {
            GameServer()->SendChatTarget(m_Player , _("Player did not react to your request"));
            GameServer()->SendChatTarget(m_Opponent , _("Match request was cancelled"));
            Reset();
        }
        return;
    }

    if(m_pNonOpponent->m_DuelFlags&CPlayer::DUEL_GETPOINT)
    {
        m_PlayerPoints++;
        m_pNonOpponent->m_DuelFlags-=CPlayer::DUEL_GETPOINT;
    }
    if(m_pOpponent->m_DuelFlags&CPlayer::DUEL_GETPOINT)
    {
        m_OpponentPoints++;
        m_pOpponent->m_DuelFlags-=CPlayer::DUEL_GETPOINT;
    }
    if(m_pNonOpponent->m_DieTick > Server()->Tick()-2 ||
       	m_pOpponent->m_DieTick > Server()->Tick()-2)
    {
        m_pNonOpponent->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
        m_pOpponent->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
        m_pNonOpponent->m_DieTick = 0;
        m_pNonOpponent->m_SpawnVTeam = 1;
        m_pOpponent->m_DieTick = 0;
        m_pOpponent->m_SpawnVTeam = 1;
    }

  	char Buf[256];
	str_format(Buf, sizeof(Buf), "%s: %d\n%s: %d                                                  ",
          Server()->ClientName(m_Player), m_PlayerPoints,
          Server()->ClientName(m_Opponent), m_OpponentPoints);
	GameServer()->SendBroadcast(Buf, m_Opponent);
	GameServer()->SendBroadcast(Buf, m_Player);
}

void CDuel::Snap(int SnappingClient)
{
}
