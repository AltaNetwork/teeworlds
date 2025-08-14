#ifndef GAME_SERVER_ENTITIES_SPECIAL_LOREMIPSUM_H
#define GAME_SERVER_ENTITIES_SPECIAL_LOREMIPSUM_H

class CLoremIpsum : public CEntity
{
public:
	CLoremIpsum(CGameWorld *pGameWorld, vec2 Pos);

	virtual void Reset();
	virtual void Tick();
	virtual void Snap(int SnappingClient);

private:
    int m_Variable;
};

#endif
