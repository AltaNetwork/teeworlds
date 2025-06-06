/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/generated/protocol.h>
#include <game/server/gamecontext.h>
#include <engine/shared/config.h>
#include "laser.h"

// Windows cannot find M_PI, although it should be in <math.h>
// #ifndef M_PI
// # define M_PI		3.14159265358979323846	/* pi */
// #endif

CLaser::CLaser(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int clockwise, int Type)
: CEntity(pGameWorld, CGameWorld::ENTTYPE_LASER)
{
	m_Pos = Pos;
	m_Type = Type;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_Bounces = 0;
	m_EvalTick = 0;
	m_Clockwise = clockwise;
	m_StartTick = Server()->Tick();
	GameWorld()->InsertEntity(this);
	DoBounce();
}

bool CLaser::HitCharacter(vec2 From, vec2 To)
{
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At);//, pOwnerChar);
	if(m_Bounces == 0)
	    pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At, pOwnerChar);// else
	//     *pHit = GameServer()->m_World.IntersectCharacter(m_Pos, To, 0.f, At);
	if(!pHit || !GameServer()->Tuning()->m_PlayerHit)
		return false;

	m_From = From;
	m_Pos = At;
	m_Energy = -1;
	pHit->TakeDamage(vec2(0.f, 0.f), GameServer()->Tuning()->m_LaserDamage, m_Owner, WEAPON_RIFLE);
	pHit->Melt();
	/////////////////////////////////////////////////////////////////////////////////////// UGH TO FINISH LATER...
		//if(m_Type == WEAPON_SHOTGUN)    {
			float Strength = 10;//Tuning()->m_ShotgunStrength;

			const vec2 &HitPos = pHit->GetCore()->m_Pos;
			// if(!g_Config.m_SvOldLaser)
			// {
				// if(m_PrevPos != HitPos)
				// {
				    vec2 newvel = vec2(normalize(pOwnerChar->GetCore()->m_Pos - HitPos) * Strength);
					//pHit->TakeDamage(vec2(100.f, 100.f), 0, m_Owner, WEAPON_RIFLE);
					pHit->GetCore()->m_Vel += newvel;
			// 	if(pOwnerChar->Core()->m_Pos != HitPos)
			// 	{
			// 		pHit->AddVelocity(normalize(pOwnerChar->Core()->m_Pos - HitPos) * Strength);
			// 	}
			// 	else
			// 	{
			// 		pHit->SetRawVelocity(StackedLaserShotgunBugSpeed);
			// 	}
			// }
			//}
		// else if(m_Type == WEAPON_RIFLE) //
	return true;
	/////////////////////////////////////////////////////////////////////////////////////////////
}

void CLaser::DoBounce()
{
	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		GameServer()->m_World.DestroyEntity(this);
		return;
	}

	// if (g_Config.m_SvPlasmaGun) {
	if (m_Clockwise != 0) {
		vec2 new_dir = m_Dir;

		double deg = 20*m_Clockwise;

		double theta = deg / 180.0 * M_PI;
		double c = cos(theta);
		double s = sin(theta);
		double tx = new_dir.x * c - new_dir.y * s;
		double ty = new_dir.x * s + new_dir.y * c;
		new_dir.x = tx;
		new_dir.y = ty;

		m_Dir = new_dir;

		vec2 To = m_Pos + new_dir * 100; // * m_Energy

		if(!HitCharacter(m_Pos, To))
		{
			m_From = m_Pos;
			m_Pos = To;
			m_Energy = m_Energy - 100;//-1;
		}
	} else {
		vec2 To = m_Pos + m_Dir * m_Energy;

		if(GameServer()->Collision()->IntersectLine(m_Pos, To, 0x0, &To))
		{
			if(!HitCharacter(m_Pos, To))
			{
				// intersected
				m_From = m_Pos;
				m_Pos = To;

				vec2 TempPos = m_Pos;
				vec2 TempDir = m_Dir * 4.0f;

				GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0);
				m_Pos = TempPos;
				m_Dir = normalize(TempDir);

				m_Energy -= distance(m_From, m_Pos) + GameServer()->Tuning()->m_LaserBounceCost;
				m_Bounces++;

				if(m_Bounces > GameServer()->Tuning()->m_LaserBounceNum)
					m_Energy = -1;

				GameServer()->CreateSound(m_Pos, SOUND_RIFLE_BOUNCE);

				if(m_Bounces == 1 && g_Config.m_SvLaserjumps && GameServer()->m_pController->IsInstagib())
					GameServer()->CreateExplosion(m_Pos, m_Owner, WEAPON_GAME, false);
			}
		}
		else
		{
			if(!HitCharacter(m_Pos, To))
			{
				m_From = m_Pos;
				m_Pos = To;
				m_Energy = -1;
			}
		}
	}
}

void CLaser::Reset()
{
	GameServer()->m_World.DestroyEntity(this);
}

void CLaser::Tick()
{
	if(Server()->Tick() > m_EvalTick+(Server()->TickSpeed()*GameServer()->Tuning()->m_LaserBounceDelay)/1000.0f)
		DoBounce();
}

void CLaser::TickPaused()
{
	++m_EvalTick;
}

void CLaser::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	// int LaserType = -1;//m_Type == WEAPON_RIFLE ? 0 : m_Type == WEAPON_SHOTGUN ? 1 : -1;

	CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, m_ID, sizeof(CNetObj_Laser)));
	if(!pObj)
		return;

	//CNetObj_DDNetLaser *pObj = Server()->SnapNewItem<CNetObj_DDNetLaser>(m_ID);
	// CNetObj_DDNetLaser *pObj = static_cast<CNetObj_DDNetLaser *>(Server()->SnapNewItem(NETOBJTYPE_DDNETLASER, m_ID, sizeof(CNetObj_Laser)));
	// if(!pObj)
	// 	return false;

	// pObj->m_ToX = (int)m_Pos.x;
	// pObj->m_ToY = (int)m_Pos.y;
	// pObj->m_FromX = (int)m_From.x;
	// pObj->m_FromY = (int)m_From.y;
	// pObj->m_StartTick = m_StartTick;
	// pObj->m_Owner = m_Owner;
	// pObj->m_Type = 0;//LaserType;
	// pObj->m_Subtype = 0;//Subtype;
	// pObj->m_SwitchNumber = 0;
	// pObj->m_Flags = 0;
	pObj->m_X = (int)m_Pos.x;
	pObj->m_Y = (int)m_Pos.y;
	pObj->m_FromX = (int)m_From.x;
	pObj->m_FromY = (int)m_From.y;
	pObj->m_StartTick = m_EvalTick;
}
