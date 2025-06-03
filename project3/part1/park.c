#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

// PART 1 - Single Threaded Solution

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t car_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t ride_done = PTHREAD_COND_INITIALIZER;

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
    time_t time = time(NULL);
    struct tm* tm = localtime(&time);
    printf("[Time: %02d:%02d:%02d] %s %d %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, subject, pid, msg);
}

void *passenger_routine(void *arg)
{
    int pid = *(int*)arg;

    char *subject = "Passenger";

    print_time("entered the park", pid);

    // random wait
    randsleep();

    print_time("finished exploring, heading to ticket booth", subject, pid);
    pthread_mutex_lock(&mutex);
    print_time("acquired a ticket", subject, pid);

    print_time("joined the ride queue", subject, pid);
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
    sleep(5);

    print_time("boarding", subject, pid);
    pthread_cond_signal(&car_ready);

    while (!boarded)
    {
        pthread_mutex_unlock(&mutex);
        sleep(1);
        pthread_mutex_lock(&mutex);
    }

    print_time("unloading", subject, pid);
    pthread_cond_signal(&ride_done);
    pthread_mutex_unlock(&mutex);    
}


// default values for parameters
// int n = 10;
// int c = 2;
// int p = 4;
// int w = 10;
// int r = 5;

int main(int argc, char* argv[])
{
    // int opt;

    // check for flags
    // while ((opt = getopt(argc, argv, "c:p:w:r:h")) != -1)
    // {
    //     switch(opt)
    //     {
    //         case 'n':
    //             n = atoi(optarg);
    //             break;
    //         case 'c':
    //             c = atoi(optarg);
    //             break;
    //         case 'p':
    //             p = atoi(optarg);
    //             break;
    //         case 'w':
    //             w = atoi(optarg);
    //             break;
    //         case 'r':
    //             r = atoi(optarg);
    //             break;
    //         case 'h':
    //         // help message
    //             printf("USAGE:\n");
    //             printf("  %s [OPTIONS]\n\n", prog_name);
    //             printf("OPTIONS:\n");
    //             printf("  -n INT   Number of passenger threads\n");
    //             printf("  -c INT   Number of cars\n");
    //             printf("  -p INT   Capacity per car\n");
    //             printf("  -w INT   Car waiting period in seconds\n");
    //             printf("  -r INT   Ride duration in seconds\n");
    //             printf("  -h       Display this help message\n");
    //             break;
    //     }
    // }
    

    // printf("===== DUCK PARK SIMULATION =====\n");
    // printf("[Monitor] Simulation started with parameters:\n");
    // printf("- Number of passenger threads: 1\n");
    // printf("- Number of cars: 1\n");
    // printf("- Capacity per car: 4\n");
    // printf("- Park exploration time: 0-5 seconds\n");
    // printf("- Car waiting period: 10 seconds\n");
    // printf("- Ride duration: 5 seconds\n");


    // seed random sleep
    srand(time(NULL));

    pthread_t passenger;
    pthread_t car;

    // create threads for passenger and car
    pthread_create(&passenger, NULL, passenger_routine, NULL);
    pthread_create(&car, NULL, car_routine, NULL);

    pthread_join(passenger, NULL);
    pthread_join(car, NULL);

    return 0;
}