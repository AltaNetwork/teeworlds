#ifndef GAME_SERVER_ENTITIES_SPECIAL_SOUL_H
#define GAME_SERVER_ENTITIES_SPECIAL_SOUL_H

class CSoul : public CEntity
{
public:
	CSoul(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
};

#endif
