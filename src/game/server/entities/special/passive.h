#ifndef GAME_SERVER_ENTITIES_SPECIAL_PASSIVEINDICATOR_H
#define GAME_SERVER_ENTITIES_SPECIAL_PASSIVEINDICATOR_H

class CPassiveIndicator : public CEntity
{
public:
	CPassiveIndicator(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	int m_LifeSpan;
};

#endif
