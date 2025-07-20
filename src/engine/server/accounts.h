#ifndef ENGINE_SERVER_ACCOUNTS_H
#define ENGINE_SERVER_ACCOUNTS_H

#include <string>
#include "database.h"

class CAccounts
{
	// enum
	// {
	// 	NIGGAONE=0,
	// 	NIGGATWO,
	// 	NIGGATHREE
	// };

	// struct CNIGGA
	// {
	// 	int m_NIGGA;
	// };
	// UserDatabase db;
public:
	CAccounts();
	void Init();
	bool Login(const std::string& username, const std::string& password);
	bool Register(const std::string& username, const std::string& password);
};

#endif
