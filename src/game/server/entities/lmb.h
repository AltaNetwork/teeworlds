

#ifndef GAME_SERVER_ENTITIES_LMB_H
#define GAME_SERVER_ENTITIES_LMB_H

#include <game/server/entity.h>

class CLmb : public CEntity
{
public:
	int m_Winner;
	int m_TimeLeft;
	int m_RegTimeLeft;
	bool m_IsTeamfight;

	CLmb(CGameWorld *pGameWorld);

	bool StartLMB();
	void LMBTick();
	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);
};

#endif
