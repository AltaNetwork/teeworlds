

#ifndef GAME_SERVER_ENTITIES_DUEL_H
#define GAME_SERVER_ENTITIES_DUEL_H

#include <game/server/entity.h>

class CDuel : public CEntity
{
public:
    int m_Player;
    int m_Opponent;
    int m_Wager;

    char BufPlayer[24];
    char BufOpponent[24];

    int m_PlayerPoints;
    int m_OpponentPoints;

    bool m_Started;
    int m_AutoCancelTick;

	CDuel(CGameWorld *pGameWorld, int Opponent, int Inviter, int Wager = 0);

	void ResetPlayer(int p_ID);
	void PreparePlayer(int p_ID);

	void EndDuel();

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif
