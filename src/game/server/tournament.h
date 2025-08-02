// #pragma once
// #include <vector>
// #include <algorithm>

// class CGameContext;

// // enum
// // {
// // 	LMB_NONREGISTERED=0,
// // 	LMB_REGISTERED,
// // 	LMB_PARTICIPATE,
// // };

// class CLMB
// {
// public:
// 	CLMB();
// 	virtual ~CLMB();

// 	void OpenRegistration();

// 	void SetGameServer(CGameContext * pGameServer) { m_pGameServer = pGameServer; }

// 	void Tick();
// 	int State() const { return m_State; }
// 	int ParticipantNum() const { return m_Participants.size(); }
// 	std::vector<int>::iterator FindParticipant(int ID);
// 	bool IsParticipant(int ID);

// 	bool RegisterPlayer(int ID);
// 	void RemoveParticipant(int CID);

// 	void TeleportParticipants();
// 	void Reset(int ClientID);

// 	int m_LastLMB;

// 	enum
// 	{
// 		STATE_STANDBY=0,
// 		STATE_REGISTRATION,
// 		STATE_RUNNING,
// 	};


// private:

// 	CGameContext * m_pGameServer;
// 	std::vector<int> m_Participants;
// 	int m_StartTick;
// 	int m_EndTick;

// 	int m_State;
// };
