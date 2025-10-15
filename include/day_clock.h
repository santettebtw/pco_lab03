#ifndef DAY_CLOCK_H
#define DAY_CLOCK_H

#include <pcosynchro/pcosemaphore.h>
#include <atomic>

class DayClock {
public:
    DayClock(int participants)
        : participants(participants),
        start_sem(0), done_sem(0), done_sem2(0), day(0) {}

    void start_next_day() {
        for (int i = 0; i < participants; ++i) {
            start_sem.release();
        }
    }

    void wait_all_done() {
        for (int i = 0; i < participants; ++i) {
            done_sem.acquire();
        }
        for (int i = 0; i < participants; ++i) {
            done_sem2.release();
        }
        ++day;
    }

    void worker_wait_day_start() {
        start_sem.acquire();
    }

    void worker_end_day() {
        done_sem.release();
        done_sem2.acquire();
    }

    [[nodiscard]] int current_day() const {
        return day.load(); 
    }

private:
    const int participants;
    PcoSemaphore start_sem;
    PcoSemaphore done_sem;
    PcoSemaphore done_sem2;
    std::atomic<int> day;
};


#endif /* DAY_CLOCK_H */
