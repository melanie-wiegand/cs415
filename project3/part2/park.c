#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

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

void print_time(const char* msg, int pid)
{
    // prints current time before the message
    time_t time = time(NULL);
    struct tm* tm = localtime(&time);
    printf("[Time: %02d:%02d:%02d] Passenger %d %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, pid, msg);
}

void *passenger_routine(void *arg)
{
    printf(" entered the park\n")
    // random wait
    randsleep();

    printf("")

}

void *car_routine(void *arg)
{

}




int main(int argc, char* argv[])
{
    int opt;

    // check for flags
    while ((opt = getopt(argc, argv, "n:c:p:w:r:h")) != -1)
    {
        switch(opt)
        {
            case 'n':
                n = atoi(optarg);
                break;
            case 'c':
                c = atoi(optarg);
                break;
            case 'p':
                p = atoi(optarg);
                break;
            case 'w':
                w = atoi(optarg);
                break;
            case 'r':
                r = atoi(optarg);
                break;
            case 'h':
            // help message
                printf("USAGE:\n");
                printf("  %s [OPTIONS]\n\n", argv[0]);
                printf("OPTIONS:\n");
                printf("  -n INT   Number of passenger threads\n");
                printf("  -c INT   Number of cars\n");
                printf("  -p INT   Capacity per car\n");
                printf("  -w INT   Car waiting period in seconds\n");
                printf("  -r INT   Ride duration in seconds\n");
                printf("  -h       Display this help message\n");
                break;
        }
    }
    

    printf("===== DUCK PARK SIMULATION =====\n");
    printf("[Monitor] Simulation started with parameters:\n");
    printf("- Number of passenger threads: %d\n", n);
    printf("- Number of cars: %d\n", c);
    printf("- Capacity per car: %d\n", p);
    printf("- Park exploration time: 0-5 seconds\n");
    printf("- Car waiting period: %d seconds\n", w);
    printf("- Ride duration: %d seconds\n", r);

    // seed random sleep
    srand(time(NULL));
    pthread_t passenger;
    pthread_t car;

    // create threads for passenger and car
    for (int i = 0; i < n; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&passengers[i], NULL, passenger, id);
    }
    pthread_create(&car, NULL, car_routine, NULL);

    pthread_join(passenger, NULL);
    pthread_join(car, NULL);

    return 0;
}