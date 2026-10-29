
#include "gameServerData.hpp"

GameServerData::GameServerData(int writeFd, Connection& connection, GameType gameType): writeFd(writeFd), connection(&connection), gameType(gameType) {}

void GameServerData::OnServerStarted(int tcpPort){
   isStarted = true;
   this->tcpPort = tcpPort;
}