#ifndef _CPPDCM_DCM_TASK_H
#define _CPPDCM_DCM_TASK_H
#include <bitset>
#include <regex>
#include <string>
#include <vector>

namespace DCM {

static std::pair<std::string, std::string> split_param_line(std::string const &line) {
  static std::regex line_splitter("(.+): (.+)");
  std::smatch match;
  if (std::regex_match(line, match, line_splitter)) {
    return std::pair<std::string, std::string>(match[1].str(), match[2].str());
  }
  return std::pair<std::string, std::string>("", "");
}

class Task {
public:
  enum Command {
    None,
    Quit,
    Chaffin
  };

  using w_p_t = std::pair<uint32_t, uint32_t>;
  using w_p_list_t = std::vector<w_p_t>;

  Task(std::vector<std::string> const &lines_from_server) {
    command = Command::Chaffin;
    for (auto &line : lines_from_server) {
      if (line == "Quit") {
        command = Task::Command::Quit;
      } else if (line == "No tasks") {
        command = Task::Command::None;
      } else if (!complete()) {
        parse_param(line);
      } else if (line.size() > 0 && line.front() == '(') {
        parse_w_p(line);
      }
    }
  }

  bool complete() {
    return param_set.all();
  }

  Command command;
  std::string task_id;
  std::string access_code;
  uint32_t n_value;
  uint32_t w_value;
  std::string prefix;
  std::string branchOrder;
  uint32_t perm_to_exceed;
  uint32_t prev_perm_ruled_out;
  w_p_list_t w_p_list;

  // "Task id: ","Access code: ","n: ","w: ","str: ","pte: ","pro: ","branchOrder: "
private:
  void parse_param(std::string const &param_line) {
    auto param_pair = split_param_line(param_line);
    if (param_pair.first == "Task id") {
      task_id = param_pair.second;
      param_set.set(0);
    } else if (param_pair.first == "Access code") {
      access_code = param_pair.second;
      param_set.set(1);
    } else if (param_pair.first == "n") {
      n_value = std::stoul(param_pair.second);
      param_set.set(2);
    } else if (param_pair.first == "w") {
      w_value = std::stoul(param_pair.second);
      param_set.set(3);
    } else if (param_pair.first == "str") {
      prefix = param_pair.second;
      param_set.set(4);
    } else if (param_pair.first == "pte") {
      perm_to_exceed = std::stoul(param_pair.second);
      param_set.set(5);
    } else if (param_pair.first == "pro") {
      prev_perm_ruled_out = std::stoul(param_pair.second);
      param_set.set(6);
    } else if (param_pair.first == "branchOrder") {
      branchOrder = param_pair.second;
      param_set.set(7);
    }
  }

  void parse_w_p(std::string const &w_p_line) {
    static std::regex w_p_splitter("\\((.+),(.+)\\)");
    std::smatch match;
    if (std::regex_match(w_p_line, match, w_p_splitter)) {
      w_p_list.emplace_back(
          std::stoul(match[1].str()),
          std::stoul(match[2].str()));
    }
  }

  std::bitset<8> param_set;
};
} // namespace DCM

#endif
