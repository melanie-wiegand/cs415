
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define DEFAULT_WAITING_TIME 5
#define DEFAULT_RIDE_DURATION 7

// Mutex and condition variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t car_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t ride_done = PTHREAD_COND_INITIALIZER;

int passenger_boarded = 0;
int ride_in_progress = 0;

// Random sleep to simulate time delay
void random_sleep(int min, int max) {
    int duration = rand() % (max - min + 1) + min;
    sleep(duration);
}

void *passenger_thread(void *arg) {
    printf("[Passenger] Exploring the park...\n");
    random_sleep(1, 3);

    printf("[Passenger] Approaching ticket booth...\n");
    pthread_mutex_lock(&mutex);
    printf("[Passenger] Got a ticket!\n");

    printf("[Passenger] Waiting to board...\n");
    pthread_cond_wait(&car_ready, &mutex);
    printf("[Passenger] Boarding the car.\n");
    passenger_boarded = 1;

    pthread_cond_wait(&ride_done, &mutex);
    printf("[Passenger] Unboarding after the ride.\n");
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *car_thread(void *arg) {
    pthread_mutex_lock(&mutex);
    printf("[Car] Waiting for passengers...\n");

    // Simulate waiting period
    sleep(DEFAULT_WAITING_TIME);

    printf("[Car] Time's up! Beginning boarding process.\n");
    pthread_cond_signal(&car_ready); // allow passenger to board

    while (!passenger_boarded) {
        pthread_mutex_unlock(&mutex);
        usleep(100000); // short sleep
        pthread_mutex_lock(&mutex);
    }

    printf("[Car] Starting the ride...\n");
    ride_in_progress = 1;
    pthread_mutex_unlock(&mutex);

    sleep(DEFAULT_RIDE_DURATION); // simulate ride

    pthread_mutex_lock(&mutex);
    printf("[Car] Ride finished. Unloading passengers.\n");
    pthread_cond_signal(&ride_done);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t passenger, car;

    pthread_create(&passenger, NULL, passenger_thread, NULL);
    pthread_create(&car, NULL, car_thread, NULL);

    pthread_join(passenger, NULL);
    pthread_join(car, NULL);

    printf("[Main] Simulation complete.\n");

    return 0;
}
