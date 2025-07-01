#include "accounts.h"

CAccounts::CAccounts()
: db("data/accounts.db") // or whatever your actual DB file is
{
}

void CAccounts::Init()
{
    // your initialization code
}

bool CAccounts::Login(const std::string& username, const std::string& password)
{
    return db.authenticate(username, password);
}

bool CAccounts::Register(const std::string& username, const std::string& password)
{
    return db.add_user(username, password, 0, 0);
}
