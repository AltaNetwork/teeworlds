
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#ifndef NON_HASED_VERSION
#include "generated/nethash.cpp"
#define GAME_VERSION "0.6.4"
#define GAME_NETVERSION "0.6" GAME_NETVERSION_HASH //the std game version
static const char GAME_RELEASE_VERSION[8] = {'0', '.', '6', '.', '4', 0};

#define MOD_BUILDDATE GAME_SERVERBUILD_DATE
#define MOD_NAME "LonerStuff"
#define MOD_VERSION "277"
#define MOD_AUTHORS "veλko (veqi)"
#define MOD_CREDITS ""
#define MOD_THANKS "Pointer and DDNet Team" //"Rei, FlowerFell-Sans, necropotame, Neox, heinrich5991, ErrorDreemurr, def-, east"
#define MOD_SOURCES ""
#endif
#endif
