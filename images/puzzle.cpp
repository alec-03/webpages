#include <cstdio>
#include <thread>
#include <vector>
#include <ctime>
#include <set>
#include <semaphore>
#include <iostream>
#include <map>
#include <future>
#include <unordered_map>

using namespace std;

std::binary_semaphore count_sem(1);
std::binary_semaphore pair_sem(1);
std::counting_semaphore thread_sem(0);
std::vector<int> occur;
set<pair<unsigned int, unsigned int>> pairs;
std::map<unsigned int, set<unsigned int>> xMap;
std::map<unsigned int, set<unsigned int>> yMap;
unsigned long chaos;

unsigned int count = 0;

inline void usage() {
    printf("usage: puzzle <nPieces> <length> <width> <iterations>\n");
    exit(1);
}

inline void validateSize(unsigned long size, unsigned int pieces) {
    if(size > RAND_MAX) {
        printf("rand_max cannot address every puzzle piece, size = %lu rand_max = %u\n", size, RAND_MAX);
        exit(2);
    }
    if(size < pieces) {
        printf("you cannot pick more pieces than the puzzle has\n");
        exit(2);
    }
}

inline unsigned int getThreadCount() {
    unsigned int ret = thread::hardware_concurrency();
    if(!ret) {
        ret = 8;
    }
    return ret;
    return 1;
}

inline void captureThreads(unsigned int threadCount) {
    for(unsigned int k = 0; k < threadCount; k++) {
        thread_sem.acquire();
    }
}

inline int findXNeighbor(unsigned int x, unsigned int y, int offset) {
    map<unsigned int, set<unsigned int>>::iterator m;
    pair_sem.acquire();
    m = xMap.find(x);
    if(m != xMap.end()) {
        if(m->second.find(y + offset) != m->second.end()) {
            pair_sem.release();
            count_sem.acquire();
            count += 1;
            count_sem.release();
        }
    }
    pair_sem.release();
    return 0;
}
inline int findYNeighbor(unsigned int x, unsigned int y, int offset) {
    map<unsigned int, set<unsigned int>>::iterator m;
    pair_sem.acquire();
    m = yMap.find(y);
    if(m != yMap.end()) {
        if(m->second.find(x + offset) != m->second.end()) {
            pair_sem.release();
            count_sem.acquire();
            count += 1;
            count_sem.release();
        }
    }
    pair_sem.release();
    return 0;
}

void findMatches(unsigned int x, unsigned int y) {
    findXNeighbor(x, y, -1);
    findYNeighbor(x, y, -1);
    findXNeighbor(x, y, 1);
    findYNeighbor(x, y, 1);
    
    thread_sem.release();
}

inline void printResult(unsigned int iterations) {
    float atLeastOne = 0, it = (float) iterations;
    for(unsigned int i = 1; i < occur.size(); i++) {
        atLeastOne += (float) occur[i];
    }
    atLeastOne /= it;
    unsigned int max = occur.size() - 1, min = 0;
    while(max > 0 && !occur[max]) {
        max--;
    }
    while(min < occur.size() && !occur[min]) {
        min++;
    }
    for(int i = 0; i < 3 && max < occur.size(); i++) {
        max++;
    }
    for(int i = 0; i < 3 && min > 0; i++) {
        min--;
    }

    printf("The odds of finding at least one match was %0.2f%%\n", atLeastOne * 100);
    cout << "number of connections on each iteration:\n\n";
    cout << "n\t| count\t| frequency distribution\n\n";
    for(unsigned int i = min; i < max; i++) {
        cout << i << "\t| " << occur[i] << "\t| ";

        int len = (int) ((float) occur[i] / it * 50.0);
        if(len <= 0 && occur[i]) {
            len = 1;
        }
        for(int j = 0; j < len; j++) {
            cout << '*';
        }
        cout << '\n';
    }
}

int main(int argc, char *argv[]) {
    if(argc != 5) {
        usage();
    }
    unsigned int pieces, iterations, length, width;
    unsigned long size;

    if(sscanf(argv[1], "%u", &pieces) != 1) {
        usage();
    }
    if(sscanf(argv[2], "%u", &length) != 1) {
        usage();
    }
    if(sscanf(argv[3], "%u", &width) != 1) {
        usage();
    }
    if(sscanf(argv[4], "%u", &iterations) != 1) {
        usage();
    }

    size = (long) length * (long) width;
    validateSize(size, pieces);

    occur = vector<int>(pieces * 4, 0);

    unsigned int threadCount = getThreadCount();

    srandom(time(nullptr));
    chaos = random(); // I WANT this variable to be in inconsistent state.
    vector<vector<int>> matrix(length, vector<int>(width, 0));

    for(unsigned int i = 0; i < iterations; ++i) {
        thread_sem.release(threadCount);
        while(pairs.size() < pieces) {
            //simulate Linear Feedback Shift Register
            chaos = chaos << 1 | ((chaos >> (sizeof(unsigned long) - 1)) ^ ((chaos >> (sizeof(unsigned long) - 3)) & 1));
            unsigned long r = random() ^ chaos;
            r %= size;
            //(x, y)
            unsigned int x = r % length, y = r / length;
            pair q = {x, y};
            pair_sem.acquire();
            auto search = pairs.emplace(q);
            if(search.second) {
                yMap[y].insert(x);
                xMap[x].insert(y);
                pair_sem.release();
                thread_sem.acquire();
                thread thread_obj(findMatches, x, y);
                thread_obj.detach(); //TODO find why causes issues
                //thread_obj.join();
            } else {
                pair_sem.release();
            }
        }

        captureThreads(threadCount);
        occur[count]++;
        count = 0;
        pairs.clear();
        xMap.clear();
        yMap.clear();
    }

    printResult(iterations);
    return 0;
}
