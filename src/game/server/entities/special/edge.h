#ifndef GAME_SERVER_ENTITIES_SPECIAL_DUELEDGE_H
#define GAME_SERVER_ENTITIES_SPECIAL_DUELEDGE_H

class CEdge : public CEntity
{
public:
	CEdge(CGameWorld *pGameWorld, vec2 Pos, bool Tourna);

	virtual void Reset();
	virtual void Snap(int SnappingClient);

private:
	bool m_Tournament;
};

#endif
