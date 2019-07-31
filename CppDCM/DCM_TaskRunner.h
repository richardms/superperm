#ifndef _CPPDCM_DCM_TASK_RUNNER_H
#define _CPPDCM_DCM_TASK_RUNNER_H
#include <array>
#include <bitset>
#include <string>
#include <vector>

#include "CSpdlog.h"
#include "DCM_Ascii.h"
#include "DCM_Client.h"
#include "DCM_Task.h"
#include "DCM_calc.h"
#include "DCM_static.h"

namespace DCM {

template <size_t DBITS, size_t N>
class TaskRunner : public CSpdlog {
public:
  //	Time we AIM to spend between system calls to check on the time;
  //	we count nodes between these checks

  static const int TIME_BETWEEN_TIME_CHECKS = (5);

  //	Time for (short) delays when the server wants us to wait

  static const int MIN_SERVER_WAIT = 2;
  static const int VAR_SERVER_WAIT = 4;

  //	When the time spent on a task exceeds this threshold, we start exponentially reducing the number of nodes
  //	we explore in each subtree, with an (1/e)-life given

  static const int TAPER_THRESHOLD = (60 * MINUTE);

  static const int TAPER_DECAY = (5 * MINUTE);

  //	Initial number of nodes to check before we bother to check elapsed time;
  //	we rescale the actual value (in nodesBeforeTimeCheck) if it is too large or too small

  static const int NODES_BEFORE_TIME_CHECK = 20000000L;

  //	Set a floor and ceiling so we can't waste an absurd amount of time doing time checks,
  //	or take too long between them.

  static const int MIN_NODES_BEFORE_TIME_CHECK = 10000000L;
  static const int MAX_NODES_BEFORE_TIME_CHECK = 1000000000L;

  static constexpr uint32_t pre_calc_fn(uint32_t n = N) {
    return (n == 1) ? 1 : n * pre_calc_fn(n - 1);
  }

  static constexpr uint32_t pre_calc_maxInt(uint32_t n = N) {
    return (n == 1) ? N : (pre_calc_maxInt(n - 1) << DBITS) + N;
  }

  static constexpr uint32_t n = N;                                 //	The number of symbols in the permutations we are considering
  static constexpr uint32_t fn = pre_calc_fn();                    //	n!
  static constexpr uint32_t nm = N - 1;                            //	n-1
  static constexpr uint32_t maxW = fn;                             //	Largest number of wasted characters we allow for
  static constexpr uint32_t nmbits = DBITS * nm;                   //	(n-1)*DBITS
  static constexpr uint32_t maxInt = pre_calc_maxInt() + 1;        //	Highest integer representation of an n-digit sequence we can encounter, plus 1
  static constexpr uint32_t maxIntM = ((maxInt - 1) >> DBITS) + 1; //	Highest integer representation of an (n-1)-digit sequence we can encounter, plus 1
  static constexpr uint32_t noc = pre_calc_fn(N - 1);              //	Number of 1-cycles
  static constexpr uint32_t nocThresh = noc / 2;                   //	Threshold for unvisited 1-cycles before we try new bounds
  static constexpr uint32_t ndCount = (maxIntM * nm);

  struct DigitScore {
    int digit;
    int score;
    int fullNum;
    int nextPart;
    int nextPerm;
  };

  using str_array_t = std::array<uint8_t, 2 * fn>;
  using n_int_array_t = std::array<uint32_t, N>;
  using n_uint8_array_t = std::array<uint8_t, N>;
  using maxint_bitset_t = std::array<bool, maxInt>;
  using maxint_int_array_t = std::array<uint32_t, maxInt>;
  using known_list_t = std::vector<std::array<int, 2>>;

  TaskRunner() : CSpdlog{"runner"} {
    if (!sharedDataIsInitialised) {
      initSharedData();
    }
  }

  std::string const &asciiString() {
    AsciiIteratorGenerator ascii_gen(curstr, pos);
    asciiString_.reserve(pos);
    asciiString_.assign(ascii_gen.cbegin(), ascii_gen.cend());
  }

  std::string const &asciiString2() {
    AsciiIteratorGenerator ascii_gen(curi, pos);
    asciiString2_.reserve(pos);
    asciiString2_.assign(ascii_gen.cbegin(), ascii_gen.cend());
  }

  void runTask(Task const &task, Client *client_) {
    currentTask = task;
    client = client_;

    if (currentTask.command == Task::None)
      return;

    doTask();
  }

private:
  // Static (i.e. shared) data
  static bool sharedDataIsInitialised;

  static const known_list_t known;

  static std::array<DigitScore, ndCount> nextDigits;

  static maxint_bitset_t valid; //	Flags saying whether integer rep of digit sequence corresponds to a valid permutation

  static maxint_int_array_t successor1; //	For each permutation, its weight-1 successor
  static maxint_int_array_t successor2; //	For each permutation, its weight-2 successor

  static maxint_int_array_t ldd; //	For each digit sequence, n - (the longest run of distinct digits, starting from the last)

  static maxint_int_array_t oneCycleCounts;  //	Number of unvisited permutations in each 1-cycle
  static maxint_int_array_t oneCycleIndices; //	The 1-cycle to which each permutation belongs

  static const std::vector<int> ocpThreshold;

  // Member (i.e. non-shared) data
  Task currentTask;
  Client *client;

  std::string curstr; //	Current string as integer digits
  std::string curi;
  std::string asciiString_;  //	String as ASCII digits
  std::string asciiString2_; //	String as ASCII digits
  std::string bestSeen;      //	Longest string seen in search, as integer digits
  int bestSeenP;
  uint32_t max_perm;                        //	Maximum number of permutations visited by any string seen so far
  std::array<int32_t, maxW> mperm_res;      //	For each number of wasted characters, the maximum number of permutations that can be visited
  std::array<int32_t, maxW> klbLen;         //	For each number of wasted characters, the lengths of the strings that visit known-lower-bound permutations
  std::array<std::string, maxW> klbStrings; //	For each number of wasted characters, a list of all strings that visit known-lower-bound permutations

  maxint_bitset_t unvisited; //	Flags set false when we visit a permutation, indexed by integer rep of permutation

  std::array<uint32_t, maxInt + 1> oneCycleBins;

  int tot_bl;                 //	The total number of wasted characters we are allowing in strings, in current search
  bool done = false;          //	Global flag we can set for speedy fall-through of recursion once we know there is nothing else we want to do
  bool splitMode = false;     //	Set true when we are splitting the task
  bool isSuper = false;       //	Set true when we have found a superpermutation
  bool cancelledTask = false; //	Set true when the server cancelled our connection to the task

  int64_t totalNodeCount, subTreesSplit, subTreesCompleted;
  int64_t nodesChecked; //	Count of nodes checked since last time check
  int64_t nodesBeforeTimeCheck = NODES_BEFORE_TIME_CHECK;
  int64_t nodesToProbe0, nodesToProbe, nodesLeft;
  time_t startedRunning;          //	Time program started running
  time_t startedCurrentTask = 0;  //	Time we started current task
  time_t timeOfLastTimeCheck;     //	Time we last checked the time
  time_t timeOfLastTimeReport;    //	Time we last reported elapsed time to the user
  time_t timeOfLastServerCheckin; //	Time we last contacted the server

  int timeBetweenServerCheckins = Task::DEFAULT_TIME_BETWEEN_SERVER_CHECKINS;
  int timeBeforeSplit = Task::DEFAULT_TIME_BEFORE_SPLIT;
  int maxTimeInSubtree = Task::DEFAULT_MAX_TIME_IN_SUBTREE;

  bool ocpTrackingOn;

  static void initSharedData() {
    if (sharedDataIsInitialised) {
      return;
    }

    valid.fill(false);

    perm_tab_t perm_tabs{n + 1};

    makePerms(n, perm_tabs);
    auto &p0 = perm_tabs[n];
    for (int i = 0; i < fn; i++) {
      int tperm = 0, tperm1 = 0, tperm2 = 0;
      for (int j0 = 0; j0 < n; j0++) {
        tperm += (p0[n * i + j0] << (j0 * DBITS));

        //	Left shift digits by one to get weight-1 successor
        tperm1 += (p0[n * i + (j0 + 1) % n] << (j0 * DBITS));

        //	Left shift digits by 2 and swap last pair
        int k0;
        if (j0 == n - 1)
          k0 = 0;
        else if (j0 == n - 2)
          k0 = 1;
        else
          k0 = (j0 + 2) % n;

        tperm2 += (p0[n * i + k0] << (j0 * DBITS));
      };
      valid[tperm] = true;
      successor1[tperm] = tperm1;
      successor2[tperm] = tperm2;
    }

    n_int_array_t dseq;
    n_int_array_t dseq2;

    dseq.fill(1);

    bool more = true;
    while (more) {
      int tperm = 0;
      for (int j0 = 0; j0 < n; j0++) {
        tperm += (dseq[j0] << (j0 * DBITS));
      };

      if (valid[tperm]) {
        ldd[tperm] = 0;
        dseq2 = dseq;
        rClassMin<N>(dseq2);
        int r = 0;
        for (int j0 = 0; j0 < n; j0++) {
          r += (dseq2[j0] << (j0 * DBITS));
        };
        oneCycleIndices[tperm] = r;
      } else {
        int ok = true;
        for (int l = 2; l <= n; l++) {
          for (int i = 0; i < l && ok; i++)
            for (int j = i + 1; j < l; j++) {
              if (dseq[n - 1 - i] == dseq[n - 1 - j]) {
                ok = false;
                break;
              };
            };

          if (!ok) {
            ldd[tperm] = n - (l - 1);
            break;
          }
        }
      }

      for (int h = n - 1; h >= 0; h--) {
        if (++dseq[h] > n) {
          if (h == 0) {
            more = false;
            break;
          };
          dseq[h] = 1;
        } else
          break;
      }
    }

    int dsum = n * (n + 1) / 2;

    //	Loop through all (n-1)-digit sequences
    dseq.fill(1);
    more = true;
    while (more) {
      int part = 0;
      for (int j0 = 0; j0 < n - 1; j0++) {
        part += (dseq[j0] << (j0 * DBITS));
      };
      auto *nd = &(nextDigits[(n - 1) * part]);

      //	Sort potential next digits by the ldd score we get by appending them

      int q = 0;
      for (int d = 1; d <= n; d++)
        if (d != dseq[n - 2]) {
          int t = (d << nmbits) + part;
          nd[q].digit = d;
          int ld = nd[q].score = ldd[t];

          //	The full number n-digit number we get if we append the chosen digit to the previous n-1

          nd[q].fullNum = t;

          //	The next (n-1)-digit partial number that follows (dropping oldest of the current n)

          int p = nd[q].nextPart = t >> DBITS;

          //	If there is a unique permutation after 0 or 1 wasted characters, precompute its number

          if (ld == 0)
            nd[q].nextPerm = t; //	Adding the current chosen digit gets us there
          else if (ld == 1)     //	After the current chosen digit, a single subsequent choice gives a unique permutation
          {
            int d2 = dsum - d;
            for (int z = 1; z <= n - 2; z++)
              d2 -= dseq[z];
            nd[q].nextPerm = (d2 << nmbits) + p;
          } else
            nd[q].nextPerm = -1;
          q++;
        };

      qsort(nd, n - 1, sizeof(DigitScore), compareDS);

      for (int h = n - 2; h >= 0; h--) {
        if (++dseq[h] > n) {
          if (h == 0) {
            more = false;
            break;
          };
          dseq[h] = 1;
        } else
          break;
      };
    };

    sharedDataIsInitialised = true;
  }

  void initData() {
    mperm_res.fill(N);
    if (known_map.count(N)) {
      auto known = known_map[N];
      for (auto const &k : known) {
        mperm_res[k[0]] = k[1];
      }
    }

    mperm_res[0] = n; //	With no wasted characters, we can visit n permutations
  }

  void doTask() {
    tot_bl = currentTask.w_value;

    initData();

    //	Initialise all permutations as unvisited
    unvisited.fill(false);

    //	Initialise 1-cycle information
    oneCycleCounts.fill(n);
    oneCycleBins.fill(0);
    oneCycleBins[n] = noc;

    //	Start the current string with the specified prefix;
    //	this will involve visiting various permutations and changes to 1-cycle counts

    int tperm0 = 0;
    int pf = 0;

    curstr = fromAscii(currentTask.prefix);
    bestSeen = curstr;
    curi = fromAscii(currentTask.branchOrder);

    curstr.reserve(maxW);
    curi.reserve(maxW);
    bestSeen.reserve(maxW);

    for (auto const d : curstr) {
      tperm0 = (tperm0 >> DBITS) | (d << nmbits);
      if (valid[tperm0]) {
        if (unvisited[tperm0]) {
          pf++;
          unvisited[tperm0] = false;

          int prevC, oc;
          oc = oneCycleIndices[tperm0];
          prevC = oneCycleCounts[oc]--;
          if (prevC - 1 < 0 || prevC > n) {
            error("oneCycleBins index is out of range (prevC={})", prevC);
            exit(EXIT_FAILURE);
          };

          oneCycleBins[prevC]--;
          oneCycleBins[prevC - 1]++;
        }
      };
    }
    int partNum0 = tperm0 >> DBITS;
    bestSeenP = pf;

    //	Maybe track 1-cycle counts

    ocpTrackingOn = (tot_bl >= ocpThreshold[n]);

    //	Set baseline times

    nodesChecked = 0;
    totalNodeCount = 0;
    subTreesSplit = 0;
    subTreesCompleted = 0;
    time(&startedCurrentTask);
    timeOfLastTimeReport = timeOfLastTimeCheck = startedCurrentTask;

    timeBeforeSplit = currentTask.timeBeforeSplit;
    maxTimeInSubtree = currentTask.maxTimeInSubtree;
    timeBetweenServerCheckins = currentTask.timeBetweenServerCheckins;

    //	Recursively fill in the string

    done = false;
    splitMode = false;
    max_perm = currentTask.perm_to_exceed;
    isSuper = (max_perm == fn);

    if (isSuper || max_perm + 1 < currentTask.prev_perm_ruled_out) {
      fillStr<false>(pf, partNum0);
    };

    //	Finish with current task with the server

    if (!cancelledTask)
      client->finishTask(currentTask, toAscii(curstr), max_perm + 1, totalNodeCount);

    //	Give stats on the task

    time_t timeNow;
    time(&timeNow);
    int tskTime = (int)difftime(timeNow, startedCurrentTask);
    int tskMin = tskTime / 60;
    int tskSec = tskTime % 60;

    info("Finished current search, bestSeenP={}, nodes visited={}, time taken={} min {} sec",
         bestSeenP, totalNodeCount, tskMin, tskSec);
    if (splitMode) {
      info("Delegated {} sub-trees, completed {} locally", subTreesSplit, subTreesCompleted);
    };
    info("--------------------------------------------------------");
  }

  void nodesAndTime() {
    totalNodeCount++;
    if (++nodesChecked >= nodesBeforeTimeCheck) {
      //	We have hit a threshold for nodes checked, so time to check the time

      time_t timeNow;
      time(&timeNow);
      double timeSpentOnTask = difftime(timeNow, startedCurrentTask);
      double timeSinceLastTimeCheck = difftime(timeNow, timeOfLastTimeCheck);
      double timeSinceLastTimeReport = difftime(timeNow, timeOfLastTimeReport);
      double timeSinceLastServerCheckin = difftime(timeNow, timeOfLastServerCheckin);

      //	Check for QUIT files

      // for (int k = 2; k < 4; k++) {
      //   FILE *fp = fopen(sqFiles[k], "r");
      //   if (fp != NULL) {
      //     warn("Detected the presence of the file %s, so stopping.\n", sqFiles[k]);
      //     logString(buffer);
      //     unregisterClient();
      //     exit(0);
      //   };
      // };

      // if (timeQuotaHardMins > 0) {
      //   double elapsedTime = difftime(timeNow, startedRunning);
      //   if (elapsedTime / 60 > timeQuotaHardMins) {
      //     logString("A 'timeLimitHard' quota has been reached, so the program will relinquish the current task with the server then quit.\n");
      //     unregisterClient();
      //     exit(0);
      //   };
      // };

      // if (timeSinceLastTimeReport > MINUTE) {
      //   int tskTime = (int)timeSpentOnTask;
      //   int tskMin = tskTime / 60;
      //   int tskSec = tskTime % 60;

      //   printf("Time spent on task so far = ");
      //   if (tskMin == 0)
      //     printf("       ");
      //   else
      //     printf(" %2d min", tskMin);
      //   printf(" %2d sec.", tskSec);
      //   printf("  Nodes searched per second = %" PRId64 "\n", (int64_t)((double)nodesBeforeTimeCheck / (timeSinceLastTimeCheck)));
      //   timeOfLastTimeReport = timeNow;
      // };

      //	Adjust the number of nodes we check before doing a time check, to bring the elapsed
      //	time closer to the target

      nodesBeforeTimeCheck = timeSinceLastTimeCheck <= 0 ? 2 * nodesBeforeTimeCheck : (int64_t)((TIME_BETWEEN_TIME_CHECKS / timeSinceLastTimeCheck) * nodesBeforeTimeCheck);

      if (nodesBeforeTimeCheck <= MIN_NODES_BEFORE_TIME_CHECK)
        nodesBeforeTimeCheck = MIN_NODES_BEFORE_TIME_CHECK;
      else if (nodesBeforeTimeCheck >= MAX_NODES_BEFORE_TIME_CHECK)
        nodesBeforeTimeCheck = MAX_NODES_BEFORE_TIME_CHECK;

      timeOfLastTimeCheck = timeNow;
      nodesChecked = 0;

      if (timeSinceLastServerCheckin > timeBetweenServerCheckins) {
        //	When we check in for this task, we might be told it's redundant

        int sres = client->checkIn(currentTask);
        if (sres >= 2)
          done = true;
        if (sres == 3)
          cancelledTask = true;
      };

      if (!splitMode) {
        if (timeSpentOnTask > timeBeforeSplit) {
          //	We have hit a threshold for elapsed time since we started this task, so split the task

          nodesToProbe0 = nodesToProbe = (int64_t)(nodesBeforeTimeCheck * maxTimeInSubtree) / (TIME_BETWEEN_TIME_CHECKS);
          info("Splitting current task, will examine up to {} nodes in each subtree ...", nodesToProbe);
          splitMode = true;
        };
      };

      //	Taper off nodesToProbe if we have been running too long

      if (timeSpentOnTask > TAPER_THRESHOLD) {
        nodesToProbe = (int64_t)(nodesToProbe0 * exp(-(timeSpentOnTask - TAPER_THRESHOLD) / TAPER_DECAY));
        info("Task taking too long, will only examine up to {} nodes in each subtree ...", nodesToProbe);
      };
    };
  }

  template <bool NODE_LIMITED>
  bool fillStr(int pfound, int partNum) noexcept {
    if (done)
      return true;
    if (NODE_LIMITED && --nodesLeft < 0)
      return false;
    nodesAndTime();
    // info("fillStr {} {} {} {}",
    //      NODE_LIMITED ? 1 : 0,
    //      curstr.size(),
    //      pfound,
    //      partNum);

    //info("        {}", toAscii(curstr));

    bool res = true;

    if (!NODE_LIMITED && splitMode) {
      nodesLeft = nodesToProbe;
      if (fillStr<true>(pfound, partNum)) {
        if ((subTreesCompleted++) % 10 == 9) {
          warn("Completed {} sub-trees locally so far ...", subTreesCompleted);
        };
      } else {
        splitTask(curstr.size());
        if ((subTreesSplit++) % 10 == 9) {
          warn("Delegated {} sub-trees so far ...", subTreesSplit);
        };
      };
      return false;
    };

    if (pfound > bestSeenP) {
      bestSeenP = pfound;
      bestSeen = curstr;
    };

    int tperm, ld;
    int alreadyWasted = curstr.size() - pfound - n + 1; //	Number of character wasted so far
    int spareW = tot_bl - alreadyWasted;                //	Maximum number of further characters we can waste while not exceeding tot_bl

    //	Loop to try each possible next digit we could append.
    //	These have been sorted into increasing order of ldd[tperm], the minimum number of further wasted characters needed to get a permutation.

    DigitScore *nd = &nextDigits[nm * partNum];

    //	To be able to fully exploit foreknowledge that we are heading for a visited permutation after 1 wasted character, we need to ensure
    //	that we still traverse the loop in order of increasing waste.
    //
    //	For example, for n=5 we might have 1123 as the last 4 digits, with the choices:
    //
    //		1123 | add 4 -> 1234 ld = 1
    //		1123 | add 5 -> 1235 ld = 1
    //
    //  The affected choices will always be the first two in the loop, and
    //	we only need to swap them if the first permutation is visited and the second is not.

    bool swap01 = (nd->score == 1 && (!unvisited[nd->nextPerm]) && unvisited[nd[1].nextPerm]);

    //	Also, it is not obligatory, but useful, to swap the 2nd and 3rd entries (indices 1 and 2) if we have (n-1) distinct digits in the
    //	current prefix, with the first 3 choices ld=0,1,2, but the 1st and 2nd entries lead to a visited permutation.  This will happen
    //	if we are on the verge of looping back at the end of a 2-cycle.
    //
    //		1234 | add 5 -> 12345 ld = 0 (but 12345 has already been visited)
    //		1234 | add 1 -> 12341 ld = 1 (but 23415 has been visited already)
    //		1234 | add 2 -> 12342 ld = 2

    bool swap12 = false; //	This is set later if the conditions are met

    bool deferredRepeat = false; //	If we find a repeated permutation, we follow that branch last

    int childIndex = 0;

    curstr.push_back(0);
    curi.push_back(0);

    for (int y = 0; y < nm; y++) {
      int z;
      if (swap01) {
        if (y == 0)
          z = 1;
        else if (y == 1) {
          z = 0;
          swap01 = false;
        } else
          z = y;
      } else if (swap12) {
        if (y == 1)
          z = 2;
        else if (y == 2) {
          z = 1;
          swap12 = false;
        } else
          z = y;
      } else
        z = y;

      DigitScore *ndz = nd + z;
      ld = ndz->score;

      //	ld tells us the minimum number of further characters we would need to waste
      //	before visiting another permutation.

      int spareW0 = spareW - ld;

      //	Having taken care of ordering issues, we can treat a visited permutation after 1 wasted character as an extra wasted character

      if (ld == 1 && !unvisited[ndz->nextPerm])
        spareW0--;

      if (spareW0 < 0)
        break;

      curstr.back() = ndz->digit;
      tperm = ndz->fullNum;

      int vperm = (ld == 0);
      if (vperm && unvisited[tperm]) {
        if (pfound + 1 > max_perm) {
          max_perm = pfound + 1;
          isSuper = (max_perm == fn);
          witnessCurrentString();
          maybeUpdateLowerBound(tperm, curstr, tot_bl, max_perm);

          if (pfound + 1 > bestSeenP) {
            bestSeenP = pfound + 1;
            bestSeen = curstr;
          };

          if (max_perm + 1 >= currentTask.prev_perm_ruled_out && !isSuper) {
            done = true;
            curstr.pop_back();
            curi.pop_back();
            return true;
          };
        } else if (isSuper && pfound + 1 == max_perm) {
          witnessCurrentString();
          maybeUpdateLowerBound(tperm, curstr, tot_bl, max_perm);
        };

        unvisited[tperm] = false;
        if (ocpTrackingOn) {
          int prevC = 0, oc = 0;
          oc = oneCycleIndices[tperm];
          prevC = oneCycleCounts[oc]--;
          oneCycleBins[prevC]--;
          oneCycleBins[prevC - 1]++;

          curi.back() = childIndex++;
          res = fillStr<NODE_LIMITED>(pfound + 1, ndz->nextPart);

          oneCycleBins[prevC - 1]--;
          oneCycleBins[prevC]++;
          oneCycleCounts[oc] = prevC;
        } else {
          curi.back() = childIndex++;
          res = fillStr<NODE_LIMITED>(pfound + 1, ndz->nextPart);
        };
        unvisited[tperm] = true;
      } else if (spareW > 0) {
        if (vperm) {
          deferredRepeat = true;
          swap12 = !unvisited[nd[1].nextPerm];
        } else {
          int d = pruneOnPerms(spareW0, pfound - max_perm);
          if (d > 0 || (isSuper && d >= 0)) {
            curi.back() = childIndex++;
            res = fillStr<NODE_LIMITED>(pfound, ndz->nextPart);
          } else {
            break;
          };
        };
      };
      if (NODE_LIMITED && !res) {
        curstr.pop_back();
        curi.pop_back();
        return false;
      }
    };

    //	If we encountered a choice that led to a repeat visit to a permutation, we follow (or prune) that branch now.
    //	It will always come from the FIRST choice in the original list, as that is where any valid permutation must be.

    if (deferredRepeat) {
      int d = pruneOnPerms(spareW - 1, pfound - max_perm);
      if (d > 0 || (isSuper && d >= 0)) {
        curstr.back() = nd->digit;
        curi.back() = childIndex++;
        fillStr<NODE_LIMITED>(pfound, nd->nextPart);
      };
    };

    curstr.pop_back();
    curi.pop_back();
    // info("< {} {}", __LINE__, curstr.size());
    return res;
  };

  void witnessCurrentString() {
    std::string asciiString = toAscii(curstr);

    //	Log the new best string locally
    warn("Found {} permutations in string {}", max_perm, asciiString);

    client->witnessString(currentTask, tot_bl, asciiString);
  }

  void witnessLowerBound(std::string const &s, int w, int p) {
    std::string asciiString = toAscii(s);

    //	Log the string locally
    warn("Found new lower bound string for w={} with {} permutations in string {}", w, p, asciiString);

    client->witnessString(currentTask, w, asciiString);
  }

  //	Create a new task to delegate a branch exploration that the current task would have performed
  //	Returns 1,2,3 for OK/Done/Cancelled

  int splitTask(int pos) {
    int res = 0;

    client->splitTask(currentTask, curstr, curi);

    return res;
  }

  //	Compare two digitScore structures for quicksort()

  static int compareDS(const void *ii0, const void *jj0) {
    DigitScore *ii = (DigitScore *)ii0, *jj = (DigitScore *)jj0;
    if (ii->score < jj->score)
      return -1;
    if (ii->score > jj->score)
      return 1;
    if (ii->digit < jj->digit)
      return -1;
    if (ii->digit > jj->digit)
      return 1;
    return 0;
  }

  //	With w characters available to waste, can we visit enough new permutations to match or increase max_perm?
  //
  //	We have one upper bound on the new permutations in mperm_res[w], and another we can calculate from the numbers of 1-cycles with various
  //	counts of unvisited permutations.

  //	We add the smaller of these bounds to d0, which is count of perms we've already seen, minus max_perm
  //	(or if calculating, we return as soon as the sign of the sum is determined)

  int pruneOnPerms(int w, int d0) {
    int res = d0 + mperm_res[w];
    if (!ocpTrackingOn || res < 0)
      return res;
    int res0 = d0;
    w++; //	We have already subtracted waste characters needed to reach first permutation, so we get the first 1-cycle for free

    //	oneCycleBins[b] contains count of how many 1-cycles have b unvisited permutations, where b is from 0 to n.

    for (int b = n; b > 0; b--) {
      int ocb = oneCycleBins[b];
      if (w <= ocb) {
        res0 += w * b;
        if (res0 >= res)
          return res;
        else {
          return res0;
        };
      } else {
        res0 += ocb * b;
        w -= ocb;
      };
      if (res0 >= res)
        return res;
      if (res0 > 0) {
        return res0;
      };
    };
    return res0;
  }

  //	Try to get a new lower bound for weight w+1 by appending digits.
  //
  //	See how many permutations we get by following a single weight-2 edge, and then as many weight-1 edges
  //	as possible before we hit a permutation already visited.

  void maybeUpdateLowerBoundAppend(int tperm, std::string &str, int w, int p) {
    std::vector<int> unv(N + 1);

    unvisited[tperm] = false;

    //	Follow weight-2 edge

    int t = successor2[tperm];

    //	Follow successive weight-1 edges

    int nu = 0, okT = 0;
    while (unvisited[t]) {
      unv[nu++] = t;        //	Record, so we can unroll
      unvisited[t] = false; //	Mark as visited
      okT = t;              //	Record the last unvisited permutation integer
      t = successor1[t];
    };

    int m = p + nu;
    if (nu > 0 && m > mperm_res[w + 1]) {
      mperm_res[w + 1] = m;

      str.push_back(str[str.size() - (n - 1)]);
      str.push_back(str[str.size() - n]);
      str.append(str.substr(str.size() - (n - 2), nu - 1));

      klbStrings[w + 1] = str;
      witnessLowerBound(klbStrings[w + 1], w + 1, m);

      maybeUpdateLowerBoundAppend(okT, str, w + 1, m);
    };

    for (int i = 0; i < nu; i++)
      unvisited[unv[i]] = true;
    unvisited[tperm] = true;
  }

  //	Try to get a new lower bound for weight w+1 by splicing in digits.
  //
  //	This particular startegy will only work for n=6 and strings that contain a weight-3 edge.
  //
  //	We follow a weight-2 edge instead of the weight-3 edge, then follow four weight-1 edges, then
  //	a weight-3 edge will again take us back to the permutation we would have reached without the detour.
  //	If all five permutations in question were unvisited, we will have added them at the cost of an increase in
  //	weight of 1.

  void maybeUpdateLowerBoundSplice(std::string const &str, int w, int p) {
    if (n != 6 || mperm_res[w + 1] >= p + 5)
      return; //	Nothing to gain

    std::vector<bool> wasted(str.size(), false);
    std::vector<int> perms(str.size(), 0);

    //	Identify wasted characters in the current string
    int tperm0 = 0;
    for (int j0 = 0; j0 < str.size(); j0++) {
      int d = curstr[j0];
      tperm0 = (tperm0 >> DBITS) | (d << nmbits);
      perms[j0] = tperm0;
      wasted[j0] = !valid[tperm0];
    };

    //	Look for weight-3 edges.  Skip the initial n-1 characters, which will always be marked as wasted.

    for (int j0 = n; j0 + 3 < str.size(); j0++) {
      if (!wasted[j0] && wasted[j0 + 1] && wasted[j0 + 2] && !wasted[j0 + 3]) {
        //	We are at a weight-3 edge

        int t0 = perms[j0];
        int t = successor2[t0];
        int nu = 0, okT = 0;
        while (unvisited[t] && nu < 5) {
          nu++;
          okT = t; //	Record the last unvisited permutation integer
          t = successor1[t];
        };
        if (nu == 5) {
          //	We found 5 unvisited permutations we can visit this way

          mperm_res[w + 1] = p + 5;
          int sz = j0 + 1;
          std::string &ks = klbStrings[w + 1];
          ks = curstr.substr(0, sz);
          ks.push_back(ks[sz - (n - 1)]);
          ks.push_back(ks[sz - (n - 0)]);
          ks.append(ks.substr(sz - (n - 2), nu - 1));
          ks.append(str.substr(j0 + 1));

          witnessLowerBound(klbStrings[w + 1], w + 1, p + 5);
          break;
        };
      };
    };
  }

  void maybeUpdateLowerBound(int tperm, std::string const &str, int w, int p) {
    std::string working = str;
    maybeUpdateLowerBoundSplice(working, w, p);
    maybeUpdateLowerBoundAppend(tperm, working, w, p);
  }
};

template <size_t DBITS, size_t N>
bool TaskRunner<DBITS, N>::sharedDataIsInitialised = false;

template <size_t DBITS, size_t N>
typename const TaskRunner<DBITS, N>::known_list_t TaskRunner<DBITS, N>::known = {};

template <size_t DBITS, size_t N>
typename std::array<typename TaskRunner<DBITS, N>::DigitScore, TaskRunner<DBITS, N>::ndCount> TaskRunner<DBITS, N>::nextDigits;

template <size_t DBITS, size_t N>
typename TaskRunner<DBITS, N>::maxint_bitset_t TaskRunner<DBITS, N>::valid;

template <size_t DBITS, size_t N>
typename TaskRunner<DBITS, N>::maxint_int_array_t TaskRunner<DBITS, N>::successor1;

template <size_t DBITS, size_t N>
typename TaskRunner<DBITS, N>::maxint_int_array_t TaskRunner<DBITS, N>::successor2;

template <size_t DBITS, size_t N>
typename TaskRunner<DBITS, N>::maxint_int_array_t TaskRunner<DBITS, N>::ldd; //	For each digit sequence, n - (the longest run of distinct digits, starting from the last)

template <size_t DBITS, size_t N>
typename TaskRunner<DBITS, N>::maxint_int_array_t TaskRunner<DBITS, N>::oneCycleCounts; //	Number of unvisited permutations in each 1-cycle
template <size_t DBITS, size_t N>
typename TaskRunner<DBITS, N>::maxint_int_array_t TaskRunner<DBITS, N>::oneCycleIndices; //	The 1-cycle to which each permutation belongs

template <size_t DBITS, size_t N>
typename const std::vector<int> TaskRunner<DBITS, N>::ocpThreshold{1000, 1000, 1000, 1000, 6, 24, 120, 720}; //	The 1-cycle to which each permutation belongs

} // namespace DCM
#endif
