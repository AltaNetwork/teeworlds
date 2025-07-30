

#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <base/tl/array.h>
#include <base/math.h>

#include <map>
#include <vector>

class CTile;
class CLayers;
class CTeleTile;
class CSpeedupTile;

class CCollision
{
	class CTile *m_pTiles;
	class CTile *m_pTiles2;
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;

	double m_Time;

	array< array<int> > m_Zones;

	bool IsTileSolid(int x, int y);
	// bool IsThrough(int x, int y, int OffsetX, int OffsetY, vec2 Pos0, vec2 Pos1) const;
	int GetTile(int x, int y);
	int GetPureMapIndex(float x, float y) const;
	int GetPureMapIndex(vec2 Pos) const { return GetPureMapIndex(Pos.x, Pos.y); }


public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,
		COLFLAG_THROUGH=8,
		COLFLAG_FREEZE=16,
		COLFLAG_UNFREEZE=32,
	};

	CCollision();
	void Init(class CLayers *pLayers);
	bool CheckPoint(float x, float y) { return IsTileSolid(round_to_int(x), round_to_int(y)); }
	bool CheckPoint(vec2 Pos) { return CheckPoint(Pos.x, Pos.y); }
	int GetCollisionAt(float x, float y) { return GetTile(round_to_int(x), round_to_int(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool TroughCheck = false);
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);

	void SetTime(double Time) { m_Time = Time; }

	//This function return an Handle to access all zone layers with the name "pName"
	int GetZoneHandle(const char* pName);
	int GetZoneValueAt(int ZoneHandle, float x, float y);

	const std::vector<vec2> &TeleOuts(int Number) const;
	const std::vector<vec2> &TeleCheckOuts(int Number) const;

	int IsSpeedup(int Index) const;
	int GetMapIndex(vec2 Pos) const;
	void GetSpeedup(int Index, vec2 *Dir, int *Force, int *MaxSpeed, int *Type) const;
	CTeleTile *TeleLayer() { return m_pTele; }

private:

	CTeleTile *m_pTele;
	CTile *m_pFront;
	// TILE_TELEOUT
	std::map<int, std::vector<vec2>> m_TeleOuts;
	// TILE_TELECHECKOUT
	std::map<int, std::vector<vec2>> m_TeleCheckOuts;
	CSpeedupTile *m_pSpeedup;

};

#endif
