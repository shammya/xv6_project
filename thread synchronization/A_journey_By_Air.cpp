#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstdio>
#include <random>

using namespace std;

class Passenger
{
public:
    int id;
    int kiosk_id;
    bool is_vip;
    int scrty_belt_no;
    bool is_brd_pass_lost;

    Passenger() {}
    Passenger(int id, int kiosk_id, bool is_vip, int scrty_belt_no, bool is_brd_pass_lost)
    {
        this->id = id;
        this->kiosk_id = kiosk_id;
        this->is_vip = is_vip;
        this->scrty_belt_no = scrty_belt_no;
        this->is_brd_pass_lost = is_brd_pass_lost;
    }
};

int kiosk_qty, belt_qty, each_belt_qty, kiosk_time, scrty_chk_time, vip_chnl_time, boarding_time, passenger_count;
int global_time, start_time, vip_count, incompetent_count;
sem_t kiosk_sem, kiosk_mtx;
sem_t *belt_sem, scrty_chk_mtx;
sem_t boarding_mtx, spcl_kiosk_mtx;
sem_t vip_enter, incompetent_enter, vip_chnl_lock, safety_lock, priority_lock;
bool is_vip_waiting;
sem_t print_lock;

void *simulate_vip_movement(void *);

void print_passenger(Passenger *passenger, string message)
{
    sem_wait(&print_lock);
    if (passenger->is_vip)
        cout << "Passenger " << passenger->id << " (VIP) " << message << global_time << endl;
    else
        cout << "Passenger " << passenger->id << message << global_time << endl;
    sem_post(&print_lock);
}

void *run_clock(void *arg)
{

    global_time = 1;
    clock_t start_time = clock();
    while (true)
    {
        global_time = round(clock() - start_time) / CLOCKS_PER_SEC;
    }
}

void *simulate_spcl_kiosk(void *arg)
{
    Passenger *passenger = (Passenger *)arg;
    print_passenger(passenger, " has started waiting to get a new boarding pass at special kiosk at time ");
    sem_wait(&spcl_kiosk_mtx);
    print_passenger(passenger, " is having a new boarding pass at special kiosk at time ");
    sleep(kiosk_time);
    print_passenger(passenger, " has got a new boarding pass at special kiosk at time ");
    sem_post(&spcl_kiosk_mtx);

    passenger->is_brd_pass_lost = false;
    pthread_t vip_chnl_thread;
    pthread_create(&vip_chnl_thread, NULL, simulate_vip_movement, (void *)passenger);
}

void *simulate_incompetent_movement(void *arg)
{
    Passenger *passenger = (Passenger *)arg;
    print_passenger(passenger, " started waiting at vip channel to reach special kiosk at time ");
    sem_wait(&incompetent_enter);
    if (is_vip_waiting)
    {
        sem_wait(&priority_lock);
    }
    incompetent_count++;
    if (incompetent_count == 1)
    {
        sem_wait(&vip_chnl_lock);
    }
    sem_post(&incompetent_enter);
    print_passenger(passenger, " started moving through vip channel to reach special kiosk at time ");
    sleep(vip_chnl_time);
    print_passenger(passenger, " crossed vip channel  to reach special kiosk at time ");

    sem_wait(&safety_lock);
    incompetent_count--;
    if (incompetent_count == 0)
    {
        sem_post(&priority_lock);
        sem_post(&vip_chnl_lock);
    }
    sem_post(&safety_lock);

    pthread_t spcl_kiosk_thread;
    pthread_create(&spcl_kiosk_thread, NULL, simulate_spcl_kiosk, (void *)passenger);
}

void *simulate_boarding(void *arg)
{
    Passenger *passenger = (Passenger *)arg;
    if (passenger->is_brd_pass_lost)
    {
        print_passenger(passenger, " has lost the boarding pass hence sent back to vip channel at time ");
        pthread_t vip_chnl_thread;
        pthread_create(&vip_chnl_thread, NULL, simulate_incompetent_movement, (void *)passenger);
    }
    else
    {
        print_passenger(passenger, " has started waiting to be boarded at time ");
        sem_wait(&boarding_mtx);
        print_passenger(passenger, " has started boarding the plane at time ");
        sleep(boarding_time);
        print_passenger(passenger, " has boarded the plane at time ");
        sem_post(&boarding_mtx);
    }
}

void *simulate_vip_movement(void *arg)
{
    Passenger *passenger = (Passenger *)arg;

    print_passenger(passenger, " started waiting at vip channel  to reach boarding gate at time ");

    sem_wait(&vip_enter);
    vip_count++;
    if (vip_count == 1)
    {
        is_vip_waiting = true;
        sem_wait(&priority_lock);
        sem_wait(&vip_chnl_lock);
    }
    sem_post(&vip_enter);
    print_passenger(passenger, " started moving through vip channel  to reach boarding gate at time ");

    sleep(vip_chnl_time);
    print_passenger(passenger, " crossed vip channel  to reach boarding gate at time ");
    sem_wait(&safety_lock);
    vip_count--;
    if (vip_count == 0)
    {
        sem_post(&vip_chnl_lock);
        sem_post(&priority_lock);
    }
    sem_post(&safety_lock);

    pthread_t boarding_thread;
    pthread_create(&boarding_thread, NULL, simulate_boarding, (void *)passenger);
}

void *simulate_scrty_chk(void *arg)
{
    Passenger *passenger = (Passenger *)arg;
    if (passenger->is_vip)
    {
        pthread_t vip_thread;
        pthread_create(&vip_thread, NULL, simulate_vip_movement, (void *)passenger);
    }
    else
    {
        sem_wait(&scrty_chk_mtx);
        int belt = rand() % belt_qty + 1;
        passenger->scrty_belt_no = belt;

        print_passenger(passenger, " has started waiting for security check in belt " + to_string(passenger->scrty_belt_no) + " from time ");

        sem_post(&scrty_chk_mtx);
        sem_wait(&belt_sem[passenger->scrty_belt_no]);
        print_passenger(passenger, " has started security check in belt " + to_string(passenger->scrty_belt_no) + " at time ");

        sleep(scrty_chk_time);
        print_passenger(passenger, " has crossed the security check from belt " + to_string(passenger->scrty_belt_no) + " at time ");

        sem_post(&belt_sem[passenger->scrty_belt_no]);

        pthread_t boarding_thread;
        pthread_create(&boarding_thread, NULL, simulate_boarding, (void *)passenger);
    }
}

void *simulate_kiosk(void *arg)
{
    Passenger *passenger = (Passenger *)arg;
    int value = 0;

    sem_wait(&kiosk_sem);
    sem_wait(&kiosk_mtx);
    sem_getvalue(&kiosk_sem, &value);
    passenger->kiosk_id = value - 1;
    sem_post(&kiosk_mtx);
    print_passenger(passenger, " has started self-check in  at kiosk " + to_string(passenger->kiosk_id) + " at time ");

    sleep(kiosk_time);

    print_passenger(passenger, " has finished check in at kiosk " + to_string(passenger->kiosk_id) + " at time ");

    sem_post(&kiosk_sem);

    pthread_t scrty_chk_thread;
    pthread_create(&scrty_chk_thread, NULL, simulate_scrty_chk, (void *)passenger);
}

void simulate()
{
    default_random_engine generator;
    poisson_distribution<int> distribution(passenger_count / 2);

    pthread_t time_thread;
    pthread_create(&time_thread, NULL, run_clock, NULL);

    pthread_t *passenger_thread;
    passenger_thread = new pthread_t[passenger_count + 1];
    int vip_count = passenger_count * 0.3;
    int brd_pass_lost_count = passenger_count * 0.4;
    for (int i = 1; i <= passenger_count; i++)
    {
        Passenger *passenger = new Passenger;
        passenger->id = i;
        passenger->kiosk_id = 0;
        passenger->scrty_belt_no = 0;
        if (rand() % 2 == 0 && vip_count > 0)
        {
            passenger->is_vip = true;
            vip_count--;
        }
        else
            passenger->is_vip = false;
        if (rand() % 2 == 1 && brd_pass_lost_count > 0)
        {
            passenger->is_brd_pass_lost = true;
            brd_pass_lost_count--;
        }
        else
            passenger->is_brd_pass_lost = false;

        print_passenger(passenger, " has arrived at the airport at time ");

        pthread_create(&passenger_thread[i], NULL, simulate_kiosk, (void *)passenger);
        sleep(distribution(generator));
    }
}

void initiate(int psngr_count)
{
    // global variables initialization
    cin >> kiosk_qty;
    cin >> belt_qty;
    cin >> each_belt_qty;
    cin >> kiosk_time;
    cin >> scrty_chk_time;
    cin >> vip_chnl_time;
    cin >> boarding_time;
    passenger_count = psngr_count;
    global_time = 1;
    vip_count = 0;
    incompetent_count = 0;
    is_vip_waiting = false;
    start_time = 0;

    // mutex initialization
    sem_init(&kiosk_mtx, 0, 1);
    sem_init(&scrty_chk_mtx, 0, 1);
    sem_init(&boarding_mtx, 0, 1);
    sem_init(&spcl_kiosk_mtx, 0, 1);
    sem_init(&vip_enter, 0, 1);
    sem_init(&incompetent_enter, 0, 1);
    sem_init(&vip_chnl_lock, 0, 1);
    sem_init(&safety_lock, 0, 1);
    sem_init(&priority_lock, 0, 1);
    sem_init(&print_lock, 0, 1);

    // counting semaphore initialization
    sem_init(&kiosk_sem, 1, kiosk_qty);
    belt_sem = new sem_t[belt_qty + 1];
    for (int i = 1; i <= belt_qty; i++)
    {
        sem_init(&belt_sem[i], 1, each_belt_qty);
    }
}

int main()
{
    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);
    initiate(10);
    simulate();
    sleep(100);
    return 0;
}