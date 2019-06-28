#include "DCM_Client.h"

#include <vector>

#include <cpr/cpr.h>
#include <fmt/format.h>

#include "DCM_Task.h"

namespace DCM {

static std::vector<std::string> split_into_lines(std::string const &content) {
  std::vector<std::string> split_content;

  std::regex pattern("\n");
  std::copy(std::sregex_token_iterator(content.begin(), content.end(), pattern, -1),
            std::sregex_token_iterator(), std::back_inserter(split_content));
  return split_content;
}

Client::Client(std::string const &url_, std::string const &version_, std::string const &team_name_)
    : CSpdlog{"client"}, url{url_}, version{version_}, team_name{team_name_} {
  instance = fmt::format("{}", rand());
}

void Client::registerClient() {
  cpr::Parameters params{
      {"version", version},
      {"action", "register"},
      {"programInstance", instance},
      {"team", team_name}};

  std::string reply;
  __sendServerCommand(params, reply);

  auto lines = split_into_lines(reply);
  for (auto &line : lines) {
    auto line_pair = split_param_line(line);
    if (line_pair.first == "Client id") {
      clientID = line_pair.second;
    } else if (line_pair.first == "IP") {
      ipAddress = line_pair.second;
    } else if (line_pair.first == "programInstance") {
      if (instance != line_pair.second) {
        throw std::exception("Program instance number mismatch");
      }
    }
  }

  if (clientID.empty())
    throw std::exception("No client id set");
  if (ipAddress.empty())
    throw std::exception("No IP address received");
}

void Client::unregisterClient() {
  cpr::Parameters params{
      {"version", version},
      {"action", "unregister"},
      {"clientID", fmt::format("{}", clientID)},
      {"IP", ipAddress},
      {"programInstance", instance}};

  std::string reply;
  __sendServerCommand(params, reply);
}

Task Client::getTask() {
  cpr::Parameters params{
      {"version", version},
      {"action", "getTask"},
      {"clientID", fmt::format("{}", clientID)},
      {"IP", ipAddress},
      {"programInstance", fmt::format("{}", instance)},
      {"team", team_name}};

  std::string reply;
  __sendServerCommand(params, reply);

  auto lines = split_into_lines(reply);
  return Task(lines);
}
void Client::finishTask(Task const &currentTask,
                        std::string const &asciiString,
                        int pro,
                        int64_t nodeCount) {
  cpr::Parameters params{
      {"version", version},
      {"action", "finishTask"},
      {"id", currentTask.task_id},
      {"access", currentTask.access_code},
      {"str", asciiString},
      {"pro", fmt::format("{}", pro)},
      {"nodeCount", fmt::format("{}", nodeCount)},
      {"team", team_name}};

  std::string reply;
  __sendServerCommand(params, reply);
}

int Client::checkIn(Task const &currentTask) {
  return 0;
}

//int Client::getMax(Task const &currentTask, int nval, int wval, int oldMax, unsigned int tid, unsigned int acc, unsigned int cid, char *ip, unsigned int pi);
void Client::splitTask(Task const &currentTask,
                       std::string const &asciiString,
                       std::string const &asciiString2) {
  cpr::Parameters params{
      {"version", version},
      {"action", "splitTask"},
      {"id", currentTask.task_id},
      {"access", currentTask.access_code},
      {"newPrefix", asciiString},
      {"branchOrder", asciiString2}};

  std::string reply;
  __sendServerCommand(params, reply);
}
void Client::witnessString(Task const &currentTask, int w, std::string const &asciiString) {
  cpr::Parameters params{
      {"version", version},
      {"action", "witnessString"},
      {"n", fmt::format("{}", currentTask.n_value)},
      {"w", fmt::format("{}", w)},
      {"str", asciiString},
      {"team", team_name}};

  std::string reply;
  __sendServerCommand(params, reply);
}

bool Client::__sendServerCommand(cpr::Parameters const &params, std::string &text) {
  std::lock_guard<std::mutex> guard(lock);
  auto result = cpr::Get(url, params);

  text = result.text;
  return true;
}

} // namespace DCM