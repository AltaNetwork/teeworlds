#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "duel.h"

CDuel::CDuel(CGameWorld *pGameWorld, int Opponent, int Inviter, int Wager)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_DUEL)
{
    m_Player = Inviter;
    m_Opponent = Opponent;
    m_VTeamSpawn = GameServer()->m_pController->VTeamDuel(m_Player, m_Opponent);

    m_PlayerPoints = 0;
    m_OpponentPoints = 0;
    m_Wager = Wager;

    m_RememberPos[Inviter] = vec2(0,0);

    m_Started = false;
    m_AutoCancelTick = SERVER_TICK_SPEED * 7;

    GameWorld()->InsertEntity(this);

    if(!GameServer()->m_apPlayers[m_Opponent] || !GameServer()->m_apPlayers[m_Player] || m_Opponent == m_Player)
    {
        Reset();
        return;
    }

    if(GameServer()->m_apPlayers[m_Opponent]->m_DuelFlags&CPlayer::DUEL_INVITED ||
        GameServer()->m_apPlayers[m_Opponent]->m_DuelFlags&CPlayer::DUEL_INDUEL )
    {
        GameServer()->SendChatTarget(m_Player, _("Player cannot recieve match requests at this moment"));
        Reset();
        return;
    }

    GameServer()->m_apPlayers[m_Opponent]->m_DuelFlags |= CPlayer::DUEL_INVITED;

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
    if (m_pPlayer)
    {
        if(m_RememberPos[p_ID])
            m_pPlayer->m_SavePos = m_RememberPos[p_ID];
        m_pPlayer->m_SpawnTeam = 0;
        m_pPlayer->m_SpawnVTeam = 0;
        m_pPlayer->m_DuelFlags = 0;
        m_pPlayer->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
        GameServer()->SendBroadcast(" ", p_ID);
    }
}

void CDuel::PreparePlayer(int p_ID)
{
    CPlayer *m_pPlayer = GameServer()->m_apPlayers[p_ID];
    if (m_pPlayer)
    {
        m_pPlayer->m_SpawnTeam = 1;
        m_pPlayer->m_SpawnVTeam = m_VTeamSpawn;
        m_pPlayer->m_DuelFlags = CPlayer::DUEL_INDUEL;
        if(m_pPlayer->GetCharacter())
            m_RememberPos[p_ID] = m_pPlayer->GetCharacter()->m_Pos;
        m_pPlayer->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
        m_pPlayer->m_DieTick = 0;
    }
}

void CDuel::Reset()
{
    if(m_Started)
        EndDuel();
    GameWorld()->DestroyEntity(this);
}

void CDuel::EndDuel()
{
    int Winner = -1;
    CPlayer *pPlayer = GameServer()->m_apPlayers[m_Player];
    CPlayer *pOpponent = GameServer()->m_apPlayers[m_Opponent];

    // Determine winner based on leave status first
    if(pPlayer && (pOpponent == nullptr || (pOpponent->m_DuelFlags & CPlayer::DUEL_LEAVE)))
    {
        Winner = m_Player;
    }
    else if(pOpponent && (pPlayer == nullptr || (pPlayer->m_DuelFlags & CPlayer::DUEL_LEAVE)))
    {
        Winner = m_Opponent;
    }
    else // If no one left, determine winner by points
    {
        if(pPlayer && pOpponent)
        {
            if (m_PlayerPoints > m_OpponentPoints)
            {
                Winner = m_Player;
            }
            else if (m_OpponentPoints > m_PlayerPoints)
            {
                Winner = m_Opponent;
            }
        }
    }

    if(pPlayer)
        ResetPlayer(m_Player);
    if(pOpponent)
        ResetPlayer(m_Opponent);

    char Buf[32];
    str_format(Buf, sizeof(Buf), "%d - %d", m_PlayerPoints, m_OpponentPoints);
    GameServer()->SendChatTarget(-1 , _("Match '{str:Player}' - '{str:Opponent}' ended with results of {str:Result}"),
        "Player", BufPlayer, "Opponent", BufOpponent, "Result", Buf);

    if(m_Wager < 1)
        return;

    if(Winner > -1 && GameServer()->m_apPlayers[Winner])
    {
        str_format(Buf, sizeof(Buf), "%d", m_Wager);
        GameServer()->m_apPlayers[Winner]->m_Score += m_Wager * 2;
        GameServer()->SendChatTarget(-1, _("'{str:Winner}' won the wager ( {num:Wager} )"), "Winner", Server()->ClientName(Winner), "Wager", Buf);
    }
    else
    {
        if(pPlayer)
            pPlayer->m_Score += m_Wager;
        if(pOpponent)
            pOpponent->m_Score += m_Wager;
        GameServer()->SendChatTarget(-1, _("The match ended in a draw, wagers have been returned."));
    }
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
            m_pNonOpponent->m_Score-=m_Wager;
            m_pOpponent->m_Score-=m_Wager;
            return;
        }
        if(m_AutoCancelTick <= 0)
        {
            GameServer()->SendChatTarget(m_Player , _("Player did not react to your request"));
            GameServer()->SendChatTarget(m_Opponent , _("Match request expired"));
            GameServer()->m_apPlayers[m_Opponent]->m_DuelFlags -= CPlayer::DUEL_INVITED;
            Reset();
            return;
        }
        if(m_pOpponent->m_DuelFlags == CPlayer::DUEL_DENY)
        {
            GameServer()->SendChatTarget(m_Player , _("Player rejected your match request"));
            GameServer()->SendChatTarget(m_Opponent , _("You rejected the match request"));
            GameServer()->m_apPlayers[m_Opponent]->m_DuelFlags -= CPlayer::DUEL_INVITED;
            Reset();
            return;
        }
        return;
    }
    int Scored = -1;
    bool Draw = false;
    if(Server()->Tick()%SERVER_TICK_SPEED == 0)
    {
        CCharacter *m_pOpponentChr = m_pOpponent->GetCharacter();
        CCharacter *m_pNonOpponentChr = m_pNonOpponent->GetCharacter();
        if(m_pNonOpponentChr != nullptr && m_pNonOpponentChr != nullptr)
        {
            if(m_pNonOpponentChr->m_DeepFrozen && m_pOpponentChr->IsGrounded()
            && m_pOpponentChr->m_DeepFrozen && m_pNonOpponentChr->IsGrounded())
            {
                Draw = true;
            }
        }
        str_format(BufPlayer, sizeof(BufPlayer), "%s", Server()->ClientName(m_Player));
        str_format(BufOpponent, sizeof(BufOpponent), "%s", Server()->ClientName(m_Opponent));
    }
    if(m_pNonOpponent->m_DuelFlags&CPlayer::DUEL_DIED)
    {
        m_OpponentPoints++;
        Scored = m_Opponent;
        m_pNonOpponent->m_DuelFlags-=CPlayer::DUEL_DIED;
    }
    if(m_pOpponent->m_DuelFlags&CPlayer::DUEL_DIED)
    {
        m_PlayerPoints++;
        Scored = m_Player;
        m_pOpponent->m_DuelFlags-=CPlayer::DUEL_DIED;
    }
    if((m_pNonOpponent->m_DieTick > Server()->Tick()-2 ||
       	m_pOpponent->m_DieTick > Server()->Tick()-2) ||
        (Draw))
    {
        m_pNonOpponent->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
        m_pOpponent->KillCharacter(WEAPON_GAME, FLAG_NOKILLMSG);
        m_pNonOpponent->m_DieTick = 0;
        m_pOpponent->m_DieTick = 0;
        if(Scored > -1)
        {
            GameServer()->SendChatTarget(m_Player, _("{str:Player} scored!"), "Player", Server()->ClientName(Scored));
            GameServer()->SendChatTarget(m_Opponent, _("{str:Player} scored!"), "Player", Server()->ClientName(Scored));
        }
        if(Draw)
        {
            GameServer()->SendChatTarget(m_Player, _("Draw!"));
            GameServer()->SendChatTarget(m_Opponent, _("Draw!"));
        }
    }

    if(Server()->Tick()%SERVER_TICK_SPEED == 0)
    {
       	char Buf[256];
    	str_format(Buf, sizeof(Buf), "%s: %d\n%s: %d                                                  ",
            Server()->ClientName(m_Player), m_PlayerPoints,
            Server()->ClientName(m_Opponent), m_OpponentPoints);
    	GameServer()->SendBroadcast(Buf, m_Opponent);
    	GameServer()->SendBroadcast(Buf, m_Player);
    }
}
