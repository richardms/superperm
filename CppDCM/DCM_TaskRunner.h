#ifndef _CPPDCM_DCM_TASK_RUNNER_H
#define _CPPDCM_DCM_TASK_RUNNER_H
#include <array>
#include <bitset>
#include <string>
#include <vector>

#include "DCM_Ascii.h"
#include "DCM_Task.h"
#include "DCM_calc.h"
#include "DCM_static.h"

namespace DCM {

template <size_t DBITS, size_t N>
class TaskRunner {
public:
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
  using maxint_bitset_t = std::bitset<maxInt>;
  using maxint_int_array_t = std::array<uint32_t, maxInt>;
  using known_list_t = std::vector<std::array<int, 2>>;

  TaskRunner() {
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

  // Member (i.e. non-shared) data

  str_array_t curstr; //	Current string as integer digits
  str_array_t curi;
  std::string asciiString_;  //	String as ASCII digits
  std::string asciiString2_; //	String as ASCII digits
  str_array_t bestSeen;      //	Longest string seen in search, as integer digits

  uint32_t max_perm;                        //	Maximum number of permutations visited by any string seen so far
  std::array<int32_t, maxW> mperm_res;      //	For each number of wasted characters, the maximum number of permutations that can be visited
  std::array<int32_t, maxW> klbLen;         //	For each number of wasted characters, the lengths of the strings that visit known-lower-bound permutations
  std::array<str_array_t, maxW> klbStrings; //	For each number of wasted characters, a list of all strings that visit known-lower-bound permutations

  maxint_bitset_t unvisited; //	Flags set FALSE when we visit a permutation, indexed by integer rep of permutation

  std::array<uint32_t, maxInt + 1> oneCycleBins;

  bool ocpTrackingOn;

  static void initSharedData() {
    if (sharedDataIsInitialised) {
      return;
    }

    valid.set(false);

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
    klbLen.fill(0);
    for (auto &klbStr : klbStrings)
      klbStr.fill(0);

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

    //	Initialise all permutations as unvisited
    unvisited.set();

    //	Initialise 1-cycle information
    oneCycleCounts.fill(n);
    oneCycleBins.fill(0);
    oneCycleBins[n] = noc;

    //	Start the current string with the specified prefix;
    //	this will involve visiting various permutations and changes to 1-cycle counts

    int tperm0 = 0;
    int pf = 0;
    for (int j0 = 0; j0 < currentTask.prefixLen; j0++) {
      int d = currentTask.prefix[j0] - '0';
      bestSeen[j0] = curstr[j0] = d;
      curi[j0] = currentTask.branchOrder[j0] - '0';
      tperm0 = (tperm0 >> DBITS) | (d << nmbits);
      if (valid[tperm0]) {
        if (unvisited[tperm0])
          pf++;
        unvisited[tperm0] = FALSE;

        int prevC, oc;
        oc = oneCycleIndices[tperm0];
        prevC = oneCycleCounts[oc]--;
        oneCycleBins[prevC]--;
        oneCycleBins[prevC - 1]++;
      };
    };
    int partNum0 = tperm0 >> DBITS;
    bestSeenP = pf;
    bestSeenLen = currentTask.prefixLen;

    //	Maybe track 1-cycle counts

    ocpTrackingOn = (tot_bl >= ocpThreshold[n]);

    //	Set baseline times

    nodesChecked = 0;
    totalNodeCount = 0;
    subTreesSplit = 0;
    subTreesCompleted = 0;
    time(&startedCurrentTask);
    time(&timeOfLastCheckin);

    //	Recursively fill in the string

    done = FALSE;
    splitMode = FALSE;
    max_perm = currentTask.perm_to_exceed;
    isSuper = (max_perm == fn);

    if (isSuper || max_perm + 1 < currentTask.prev_perm_ruled_out) {
      fillStr<false>(currentTask.prefixLen, pf, partNum0);
    };
  }

  template <bool NODE_LIMITED>
  void fillStr(int pos, int pfound, int partNum) {
    static char buffer[BUFFER_SIZE];
    if (done)
      return;

    if (!NODE_LIMITED && splitMode) {
      nodesLeft = nodesToProbe;
      if (fillStr<true>(pos, pfound, partNum)) {
        if ((subTreesCompleted++) % 10 == 9) {
          printf("Completed %" PRId64 " sub-trees locally so far ...\n", subTreesCompleted);
        };
      } else {
        splitTask(pos);
        if ((subTreesSplit++) % 10 == 9) {
          printf("Delegated %" PRId64 " sub-trees so far ...\n", subTreesSplit);
        };
      };
      return;
    };

    if (pfound > bestSeenP) {
      bestSeenP = pfound;
      bestSeenLen = pos;
      for (int i = 0; i < bestSeenLen; i++)
        bestSeen[i] = curstr[i];
    };

    totalNodeCount++;
    if (++nodesChecked >= nodesBeforeTimeCheck) {
      //	We have hit a threshold for nodes checked, so time to check the time

      time_t t;
      time(&t);
      double elapsedTime = difftime(t, timeOfLastCheckin);

      //	Adjust the number of nodes we check before doing a time check, to bring the elapsed
      //	time closer to the target

      printf("ElapsedTime=%lf\n", elapsedTime);
      printf("Current nodesBeforeTimeCheck=%" PRId64 "\n", nodesBeforeTimeCheck);

      int64_t nbtc = nodesBeforeTimeCheck;
      nodesBeforeTimeCheck = elapsedTime <= 0 ? 2 * nodesBeforeTimeCheck : (int64_t)((TIME_BETWEEN_SERVER_CHECKINS / elapsedTime) * nodesBeforeTimeCheck);
      if (nbtc != nodesBeforeTimeCheck)
        printf("Adjusted nodesBeforeTimeCheck=%" PRId64 "\n", nodesBeforeTimeCheck);

      nbtc = nodesBeforeTimeCheck;
      if (nodesBeforeTimeCheck <= MIN_NODES_BEFORE_TIME_CHECK)
        nodesBeforeTimeCheck = MIN_NODES_BEFORE_TIME_CHECK;
      else if (nodesBeforeTimeCheck >= MAX_NODES_BEFORE_TIME_CHECK)
        nodesBeforeTimeCheck = MAX_NODES_BEFORE_TIME_CHECK;

      if (nbtc != nodesBeforeTimeCheck)
        printf("Clipped nodesBeforeTimeCheck=%" PRId64 "\n", nodesBeforeTimeCheck);

      timeOfLastCheckin = t;
      nodesChecked = 0;

      /*	Version 7.2: Since a change in the maximum permutation is a rare event, it's not really worth the overhead to
		check for it within a task's run.
	
	//	We have hit a threshold for elapsed time since last check in with the server
	//	Check in and get current maximum for the (n,w) pair we are working on
	
	max_perm = getMax(currentTask.n_value, currentTask.w_value, max_perm,
		currentTask.task_id, currentTask.access_code,clientID,ipAddress,programInstance);
	isSuper = (max_perm==fn);
	if (max_perm+1 >= currentTask.prev_perm_ruled_out && !isSuper)
		{
		done=TRUE;
		return;
		};
	*/

      elapsedTime = difftime(t, startedCurrentTask);
      if (elapsedTime > TIME_BEFORE_SPLIT) {
        //	We have hit a threshold for elapsed time since we started this task, so split the task

        startedCurrentTask = t;
        nodesToProbe = (int64_t)(nodesBeforeTimeCheck * MAX_TIME_IN_SUBTREE) / (TIME_BETWEEN_SERVER_CHECKINS);
        sprintf(buffer, "Splitting current task, will examine up to %" PRId64 " nodes in each subtree ...", nodesToProbe);
        logString(buffer);
        splitMode = TRUE;
      };
    };

    int tperm, ld;
    int alreadyWasted = pos - pfound - n + 1; //	Number of character wasted so far
    int spareW = tot_bl - alreadyWasted;      //	Maximum number of further characters we can waste while not exceeding tot_bl

    //	Loop to try each possible next digit we could append.
    //	These have been sorted into increasing order of ldd[tperm], the minimum number of further wasted characters needed to get a permutation.

    struct digitScore *nd = nextDigits + nm * partNum;

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

    int swap01 = (nd->score == 1 && (!unvisited[nd->nextPerm]) && unvisited[nd[1].nextPerm]);

    //	Also, it is not obligatory, but useful, to swap the 2nd and 3rd entries (indices 1 and 2) if we have (n-1) distinct digits in the
    //	current prefix, with the first 3 choices ld=0,1,2, but the 1st and 2nd entries lead to a visited permutation.  This will happen
    //	if we are on the verge of looping back at the end of a 2-cycle.
    //
    //		1234 | add 5 -> 12345 ld = 0 (but 12345 has already been visited)
    //		1234 | add 1 -> 12341 ld = 1 (but 23415 has been visited already)
    //		1234 | add 2 -> 12342 ld = 2

    int swap12 = FALSE; //	This is set later if the conditions are met

    int deferredRepeat = FALSE; //	If we find a repeated permutation, we follow that branch last

    int childIndex = 0;
    for (int y = 0; y < nm; y++) {
      int z;
      if (swap01) {
        if (y == 0)
          z = 1;
        else if (y == 1) {
          z = 0;
          swap01 = FALSE;
        } else
          z = y;
      } else if (swap12) {
        if (y == 1)
          z = 2;
        else if (y == 2) {
          z = 1;
          swap12 = FALSE;
        } else
          z = y;
      } else
        z = y;

      struct digitScore *ndz = nd + z;
      ld = ndz->score;

      //	ld tells us the minimum number of further characters we would need to waste
      //	before visiting another permutation.

      int spareW0 = spareW - ld;

      //	Having taken care of ordering issues, we can treat a visited permutation after 1 wasted character as an extra wasted character

      if (ld == 1 && !unvisited[ndz->nextPerm])
        spareW0--;

      if (spareW0 < 0)
        break;

      curstr[pos] = ndz->digit;
      tperm = ndz->fullNum;

      int vperm = (ld == 0);
      if (vperm && unvisited[tperm]) {
        if (pfound + 1 > max_perm) {
          max_perm = pfound + 1;
          isSuper = (max_perm == fn);
          witnessCurrentString(pos + 1);
          maybeUpdateLowerBound(tperm, pos + 1, tot_bl, max_perm);

          if (pfound + 1 > bestSeenP) {
            bestSeenP = pfound + 1;
            bestSeenLen = pos + 1;
            for (int i = 0; i < bestSeenLen; i++)
              bestSeen[i] = curstr[i];
          };

          if (max_perm + 1 >= currentTask.prev_perm_ruled_out && !isSuper) {
            done = TRUE;
            return;
          };
        } else if (isSuper && pfound + 1 == max_perm) {
          witnessCurrentString(pos + 1);
          maybeUpdateLowerBound(tperm, pos + 1, tot_bl, max_perm);
        };

        unvisited[tperm] = FALSE;
        if (ocpTrackingOn) {
          int prevC = 0, oc = 0;
          oc = oneCycleIndices[tperm];
          prevC = oneCycleCounts[oc]--;
          oneCycleBins[prevC]--;
          oneCycleBins[prevC - 1]++;

          curi[pos] = childIndex++;
          fillStr(pos + 1, pfound + 1, ndz->nextPart);

          oneCycleBins[prevC - 1]--;
          oneCycleBins[prevC]++;
          oneCycleCounts[oc] = prevC;
        } else {
          curi[pos] = childIndex++;
          fillStr<NODE_LIMITED>(pos + 1, pfound + 1, ndz->nextPart);
        };
        unvisited[tperm] = TRUE;
      } else if (spareW > 0) {
        if (vperm) {
          deferredRepeat = TRUE;
          swap12 = !unvisited[nd[1].nextPerm];
        } else {
          int d = pruneOnPerms(spareW0, pfound - max_perm);
          if (d > 0 || (isSuper && d >= 0)) {
            curi[pos] = childIndex++;
            fillStr<NODE_LIMITED>(pos + 1, pfound, ndz->nextPart);
          } else {
            break;
          };
        };
      };
    };

    //	If we encountered a choice that led to a repeat visit to a permutation, we follow (or prune) that branch now.
    //	It will always come from the FIRST choice in the original list, as that is where any valid permutation must be.

    if (deferredRepeat) {
      int d = pruneOnPerms(spareW - 1, pfound - max_perm);
      if (d > 0 || (isSuper && d >= 0)) {
        curstr[pos] = nd->digit;
        curi[pos] = childIndex++;
        fillStr<NODE_LIMITED>(pos + 1, pfound, nd->nextPart);
      };
    };
  };

  void witnessCurrentString(int size) {
    static char buffer[BUFFER_SIZE];

    //	Convert current digit string to null-terminated ASCII string

    for (int k = 0; k < size; k++)
      asciiString[k] = '0' + curstr[k];
    asciiString[size] = '\0';

    //	Log the new best string locally

    sprintf(buffer, "Found %d permutations in string %s", max_perm, asciiString);
    logString(buffer);

#if !NO_SERVER

    //	Log it with the server

    sprintf(buffer, "action=witnessString&n=%u&w=%u&str=%s&team=%s", n, tot_bl, asciiString, teamName);
    sendServerCommandAndLog(buffer, NULL);
#endif
  }

  void witnessLowerBound(char *s, int size, int w, int p) {
    static char buffer[BUFFER_SIZE];

    //	Convert digit string to null-terminated ASCII string

    for (int k = 0; k < size; k++)
      asciiString[k] = '0' + s[k];
    asciiString[size] = '\0';

    //	Log the string locally

    sprintf(buffer, "Found new lower bound string for w=%d with %d permutations in string %s", w, p, asciiString);
    logString(buffer);

#if !NO_SERVER

    //	Log it with the server

    sprintf(buffer, "action=witnessString&n=%u&w=%u&str=%s&team=%s", n, w, asciiString, teamName);
    sendServerCommandAndLog(buffer, NULL);
#endif
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
    if (ocpTrackingOff || res < 0)
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

} // namespace DCM
#endif
