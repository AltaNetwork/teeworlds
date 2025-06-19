

#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

#include <engine/map.h>
#include <game/mapitems.h>
#include <game/zones.h>

class CLayers
{
	int m_GroupsNum;
	int m_GroupsStart;
	int m_LayersNum;
	int m_LayersStart;
	CMapItemGroup *m_pGameGroup;
	CMapItemGroup *m_pZoneGroup;
	CMapItemGroup *m_pEntityGroup;
	CMapItemLayerTilemap *m_pGameLayer;
	CMapItemLayerTilemap *m_pFrontLayer; // Testing hook trough not working :P
	CMapItemLayerTilemap *m_pTeleLayer;
	CMapItemLayerTilemap *m_pSpeedupLayer;
	class IMap *m_pMap;

public:
	CLayers();
	void Init(class IKernel *pKernel);
	int NumGroups() const { return m_GroupsNum; };
	class IMap *Map() const { return m_pMap; };
	CMapItemGroup *GameGroup() const { return m_pGameGroup; };
	CMapItemGroup *ZoneGroup() const { return m_pZoneGroup; };
	CMapItemGroup *EntityGroup() const { return m_pEntityGroup; };
	CMapItemLayerTilemap *GameLayer() const { return m_pGameLayer; };
	CMapItemLayerTilemap *FrontLayer() const { return m_pFrontLayer; };
	CMapItemLayerTilemap *TeleLayer() const { return m_pTeleLayer; }
	CMapItemLayerTilemap *SpeedupLayer() const { return m_pSpeedupLayer; }

	CMapItemGroup *GetGroup(int Index) const;
	CMapItemLayer *GetLayer(int Index) const;
};

#endif
