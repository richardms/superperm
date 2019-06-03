#ifndef _CPPDCM_DCM_CLIENT_H
#define _CPPDCM_DCM_CLIENT_H
#include "CSpdlog.h"
#include <cpr/cpr.h>
#include <mutex>
#include <string>

namespace DCM {

class Task;

class Client : public CSpdlog {
public:
  Client(std::string const &url_,
         std::string const &version_,
         std::string const &team_name_);

  void registerClient();
  void unregisterClient();
  Task getTask();
  void finishTask(Task const &currentTask,
                  std::string const &asciiString,
                  int pro,
                  int64_t nodeCount);
  int checkIn(Task const &currentTask);
  //int getMax(int nval, int wval, int oldMax, unsigned int tid, unsigned int acc, unsigned int cid, char *ip, unsigned int pi);
  void splitTask(Task const &currentTask,
                 std::string const &asciiString,
                 std::string const &asciiString2);
  void witnessString(Task const &currentTask, int w, std::string const &asciiString);

private:
  bool __sendServerCommand(cpr::Parameters const &params, std::string &text);

  cpr::Url url;
  std::string const version;
  std::string const team_name;
  std::string instance;
  std::string clientID;
  std::string ipAddress;
  std::mutex lock;
};

} // namespace DCM

#endif
