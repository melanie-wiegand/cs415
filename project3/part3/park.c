#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <time.h>

#define MAX_PASSENGERS 100
#define MAX_QUEUE 20

int n_passengers = 20;
int car_capacity = 5;
int car_wait = 10;
int ride_time = 8;
int queue_limit = 10;

pthread_mutex_t ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ride_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t car_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t car_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_boarded = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_unboarded = PTHREAD_COND_INITIALIZER;

int ride_queue[MAX_QUEUE];
int ride_queue_size = 0;
int boarded = 0;
int unboarded = 0;

void* passenger_thread(void* arg);
void* car_thread(void* arg);

void sleep_random(int min, int max) {
    int time = rand() % (max - min + 1) + min;
    sleep(time);
}

void timestamp_log(const char* msg, int id) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    printf("[Time: %02d:%02d:%02d] Passenger %d %s\n",
           tm->tm_hour, tm->tm_min, tm->tm_sec, id, msg);
}

void* passenger_thread(void* arg) {
    int id = *(int*)arg;
    free(arg);

    timestamp_log("entered the park", id);
    sleep_random(2, 5); // Exploring the park
    timestamp_log("finished exploring, heading to ticket booth", id);

    pthread_mutex_lock(&ticket_mutex);
    timestamp_log("waiting in ticket queue", id);
    sleep(1); // Ticket transaction time
    timestamp_log("acquired a ticket", id);
    pthread_mutex_unlock(&ticket_mutex);

    pthread_mutex_lock(&ride_queue_mutex);
    if (ride_queue_size < queue_limit) {
        ride_queue[ride_queue_size++] = id;
        timestamp_log("joined the ride queue", id);
    } else {
        timestamp_log("ride queue full, leaving", id);
        pthread_mutex_unlock(&ride_queue_mutex);
        pthread_exit(NULL);
    }
    pthread_mutex_unlock(&ride_queue_mutex);

    // Wait to board
    pthread_mutex_lock(&car_mutex);
    while (boarded >= car_capacity) {
        pthread_cond_wait(&car_ready, &car_mutex);
    }
    boarded++;
    timestamp_log("boarded Car 1", id);

    if (boarded == car_capacity) {
        pthread_cond_signal(&all_boarded);
    }

    // Wait to unboard
    pthread_cond_wait(&all_unboarded, &car_mutex);
    timestamp_log("unboarded Car 1", id);
    unboarded++;
    pthread_mutex_unlock(&car_mutex);

    pthread_exit(NULL);
}

void* car_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&car_mutex);
        timestamp_log("Car 1 invoked load()", 0);
        boarded = 0;
        unboarded = 0;

        pthread_cond_broadcast(&car_ready);

        // Wait for either full car or timeout
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += car_wait;

        pthread_cond_timedwait(&all_boarded, &car_mutex, &ts);

        timestamp_log("Car 1 departing", 0);
        pthread_mutex_unlock(&car_mutex);
        sleep(ride_time); // Ride time

        pthread_mutex_lock(&car_mutex);
        pthread_cond_broadcast(&all_unboarded);
        pthread_mutex_unlock(&car_mutex);

        sleep(1); // Small delay before next ride
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    pthread_t passengers[MAX_PASSENGERS];
    pthread_t car;

    printf("===== DUCK PARK SIMULATION =====");
    
    pthread_create(&car, NULL, car_thread, NULL);

    for (int i = 0; i < n_passengers; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&passengers[i], NULL, passenger_thread, id);
        sleep(1); // stagger arrivals
    }

    for (int i = 0; i < n_passengers; i++) {
        pthread_join(passengers[i], NULL);
    }

    pthread_cancel(car);
    pthread_join(car, NULL);
    return 0;
}
