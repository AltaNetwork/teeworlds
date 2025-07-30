#ifndef GAME_SERVER_ENTITIES_SPECIAL_INDICATOR_H
#define GAME_SERVER_ENTITIES_SPECIAL_INDICATOR_H

class CIndicator : public CEntity
{
public:
	CIndicator(CGameWorld *pGameWorld, vec2 Pos, int Owner, int Flags);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	int m_Flags;
};

#endif
