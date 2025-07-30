#ifndef GAME_SERVER_ENTITIES_SPECIAL_KOH_H
#define GAME_SERVER_ENTITIES_SPECIAL_KOH_H

class CKoh : public CEntity
{
public:
	CKoh(CGameWorld *pGameWorld, vec2 Pos, int Flags);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
	int m_Flags;
	int m_Radius;
};

#endif
