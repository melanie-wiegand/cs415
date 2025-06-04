// FIFO Duck Park Simulation with proper ride queue
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define MAX_PASSENGERS 1000
#define MAX_CARS 20

// Simulation Parameters
int n = 10; // number of passengers
int c = 2;  // number of cars
int p = 4;  // car capacity
int w = 10; // car waiting time
int r = 5;  // ride duration

volatile int time_up = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t passenger_ready = PTHREAD_COND_INITIALIZER;

// Ride Queue (FIFO)
int ride_queue[MAX_PASSENGERS];
int ride_front = 0, ride_rear = 0;

// Each passenger gets their own boarding signal
pthread_cond_t passenger_board[MAX_PASSENGERS];

// Track if a passenger has boarded
int passenger_boarded[MAX_PASSENGERS] = {0};

// Car struct
typedef struct {
    int id;
    int pass_ids[MAX_PASSENGERS];
    int boarded_count;
    pthread_cond_t car_ready;
    pthread_cond_t ride_done;
} Car;

Car cars[MAX_CARS];
int current_car_turn = 0;

void enqueue_passenger(int pid) {
    ride_queue[ride_rear++] = pid;
}

int dequeue_passenger() {
    return ride_queue[ride_front++];
}

int queue_size() {
    return ride_rear - ride_front;
}

void randsleep() {
    sleep((rand() % 10) + 1);
}

void *timer_routine(void *arg) {
    sleep(30);
    pthread_mutex_lock(&mutex);
    time_up = 1;
    pthread_cond_broadcast(&passenger_ready);
    for (int i = 0; i < c; ++i)
        pthread_cond_broadcast(&cars[i].ride_done);
    pthread_mutex_unlock(&mutex);
    printf("[Monitor] Simulation time ended.\n");
    return NULL;
}

void print_time(const char* msg, const char* subject, int pid) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    printf("[Time: %02d:%02d:%02d] %s %d %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, subject, pid, msg);
}

void *passenger_routine(void *arg) {
    int pid = *(int*)arg;

    char *subject = "Passenger";
    while (!time_up) {
        print_time("entered the park", subject, pid);
        randsleep();

        print_time("got a ticket", subject, pid);

        pthread_mutex_lock(&mutex);
        print_time("joined the ride queue", subject, pid);
        enqueue_passenger(pid);
        pthread_cond_broadcast(&passenger_ready);
        pthread_mutex_unlock(&mutex);

        pthread_mutex_lock(&mutex);
        while (!passenger_boarded[pid] && !time_up) {
            pthread_cond_wait(&passenger_board[pid], &mutex);
        }
        pthread_mutex_unlock(&mutex);

        if (time_up) break;

        char msg[64];
        snprintf(msg, sizeof(msg), "boarded Car %d", current_car_turn + 1);
        print_time(msg, subject, pid);

        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cars[current_car_turn].ride_done, &mutex);
        pthread_mutex_unlock(&mutex);

        snprintf(msg, sizeof(msg), "deboarded Car %d", current_car_turn + 1);
        print_time(msg, subject, pid);
    }

    return NULL;
}

void *car_routine(void *arg) {
    int cid = *(int*)arg;
    Car *car = &cars[cid];

    while (!time_up) {
        pthread_mutex_lock(&mutex);

        while (current_car_turn != cid && !time_up)
            pthread_cond_wait(&car->car_ready, &mutex);

        if (time_up) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        int wait_time = 0;
        while (queue_size() < p && wait_time < w && !time_up) {
            pthread_mutex_unlock(&mutex);
            sleep(1);
            wait_time++;
            pthread_mutex_lock(&mutex);
        }

        print_time("invoked load()", "Car", cid + 1);

        int to_board = (queue_size() < p) ? queue_size() : p;
        car->boarded_count = 0;

        for (int i = 0; i < to_board; ++i) {
            int pid = dequeue_passenger();
            car->pass_ids[car->boarded_count++] = pid;
            passenger_boarded[pid] = 1;
            pthread_cond_signal(&passenger_board[pid]);
        }

        print_time("starting ride", "Car", cid + 1);
        pthread_mutex_unlock(&mutex);

        sleep(r);

        pthread_mutex_lock(&mutex);
        print_time("unloading", "Car", cid + 1);
        pthread_cond_broadcast(&car->ride_done);

        current_car_turn = (current_car_turn + 1) % c;
        pthread_cond_broadcast(&cars[current_car_turn].car_ready);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "n:c:p:w:r:")) != -1) {
        switch (opt) {
            case 'n': n = atoi(optarg); break;
            case 'c': c = atoi(optarg); break;
            case 'p': p = atoi(optarg); break;
            case 'w': w = atoi(optarg); break;
            case 'r': r = atoi(optarg); break;
        }
    }

    srand(time(NULL));

    for (int i = 0; i < n; ++i) pthread_cond_init(&passenger_board[i], NULL);
    for (int i = 0; i < c; ++i) {
        cars[i].id = i;
        cars[i].boarded_count = 0;
        pthread_cond_init(&cars[i].car_ready, NULL);
        pthread_cond_init(&cars[i].ride_done, NULL);
    }

    pthread_t timer;
    pthread_create(&timer, NULL, timer_routine, NULL);

    pthread_t car_threads[c];
    pthread_t passenger_threads[n];
    int car_ids[c], pass_ids[n];

    for (int i = 0; i < c; ++i) {
        car_ids[i] = i;
        pthread_create(&car_threads[i], NULL, car_routine, &car_ids[i]);
    }

    for (int i = 0; i < n; ++i) {
        pass_ids[i] = i;
        pthread_create(&passenger_threads[i], NULL, passenger_routine, &pass_ids[i]);
        usleep(100000);
    }

    for (int i = 0; i < n; ++i) pthread_join(passenger_threads[i], NULL);

    return 0;
}
