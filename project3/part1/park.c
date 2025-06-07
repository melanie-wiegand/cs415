#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

// PART 1 - Single Threaded Solution


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t car_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t ride_done = PTHREAD_COND_INITIALIZER;

pthread_cond_t passenger_ready = PTHREAD_COND_INITIALIZER;

int queue = 0;


int boarded = 0;
int riding = 0;



void randsleep()
{
    int sleeptime = rand() % 5;
    sleep(sleeptime);
}

void print_time(const char* msg, char* subject, int pid)
{
    // prints current time before the message
    time_t t = time(NULL);
    int total_time = (int)difftime(t, start_time);
    
    int h = total_time / 3600;
    int m = (total_time % 3600) / 60;
    int s = total_time % 60;

    printf("[Time: %02d:%02d:%02d] %s %d %s\n", h, m, s, subject, pid, msg);
}

void *passenger_routine(void *arg)
{
    int pid = *(int*)arg;

    char *subject = "Passenger";

    print_time("entered the park", subject, pid);

    // random wait
    randsleep();

    print_time("finished exploring, heading to ticket booth", subject, pid);
    pthread_mutex_lock(&mutex);
    print_time("acquired a ticket", subject, pid);

    print_time("joined the ride queue", subject, pid);
    queue++;
    pthread_cond_signal(&passenger_ready);
    pthread_cond_wait(&car_ready, &mutex);
    print_time("boarded", subject, pid);
    boarded = 1;

    pthread_cond_wait(&ride_done, &mutex);
    print_time("deboarded", subject, pid);
    pthread_mutex_unlock(&mutex);
}

void *car_routine(void *arg)
{
    pthread_mutex_lock(&mutex);

    int pid = *(int*)arg;
    char *subject = "Car";

    print_time("waiting for passengers", subject, pid);
    // waiting
    while (queue == 0)
    {
        pthread_cond_wait(&passenger_ready, &mutex);
    }

    print_time("boarding", subject, pid);
    pthread_cond_signal(&car_ready);

    while (!boarded)
    {
        pthread_mutex_unlock(&mutex);
        sleep(1);
        pthread_mutex_lock(&mutex);
    }

    print_time("starting ride", subject, pid);
    riding = 1;
    pthread_mutex_unlock(&mutex);

    // ride duration
    sleep(5);

    pthread_mutex_lock(&mutex);
    print_time("unloading", subject, pid);
    pthread_cond_signal(&ride_done);
    pthread_mutex_unlock(&mutex);    
}

int main(int argc, char* argv[])
{

    printf("===== DUCK PARK SIMULATION =====\n");
    printf("[Monitor] Simulation started with parameters:\n");
    printf("- Number of passenger threads: 1\n");
    printf("- Number of cars: 1\n");
    printf("- Capacity per car: 1\n");
    printf("- Park exploration time: 0-5 seconds\n");
    printf("- Car waiting period: 10 seconds\n");
    printf("- Ride duration: 5 seconds\n");


    // seed random sleep
    srand(time(NULL));

    pthread_t passenger;
    pthread_t car;

    int pass_id = 1;
    int car_id = 1;

    // create threads for passenger and car
    pthread_create(&passenger, NULL, passenger_routine, &pass_id);
    pthread_create(&car, NULL, car_routine, &car_id);

    // join threads
    pthread_join(passenger, NULL);
    pthread_join(car, NULL);

    return 0;
}