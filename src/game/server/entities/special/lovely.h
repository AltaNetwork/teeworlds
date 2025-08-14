#ifndef GAME_SERVER_ENTITIES_SPECIAL_LOVELY_H
#define GAME_SERVER_ENTITIES_SPECIAL_LOVELY_H

class CLovely : public CEntity
{
public:
	CLovely(CGameWorld *pGameWorld, vec2 Pos, int Flags);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
    int m_Lifetime;
};

#endif
