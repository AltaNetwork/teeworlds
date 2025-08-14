#ifndef GAME_SERVER_ENTITIES_SPECIAL_HAT_H
#define GAME_SERVER_ENTITIES_SPECIAL_HAT_H

class CHat : public CEntity
{
public:
	CHat(CGameWorld *pGameWorld, vec2 Pos, int Owner);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Owner;
	int m_Weapon;
};

#endif
