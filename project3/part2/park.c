#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t car_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t ride_done = PTHREAD_COND_INITIALIZER;

int boarded = 0;
int riding = 0;

// default values for parameters
int n = 10;
int c = 2;
int p = 4;
int w = 10;
int r = 5;

// initialize queue counter
int queue = 0;

// car struct
typedef struct
{
    int id;
    int boarded_count;
    int *pass_ids;
    int passengers_needed
    pthread_cond_t car_ready;
    pthread_cond_t ride_done;
    // pthread_mutex_t car_mutex = PTHREAD_MUTEX_INITIALIZER;
} Car;


void randsleep()
{
    int sleeptime = rand() % 5;
    sleep(sleeptime);
}

void print_time(const char* msg, const char* subject, int pid)
{
    // prints current time before the message
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    printf("[Time: %02d:%02d:%02d] %s %d %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, subject, pid, msg);
}

void *passenger_routine(void *arg)
{
    int pid = *(int*)arg;
    char *subject = "Passenger";

    print_time("entered the park", subject, pid);
    randsleep();
    print_time("finished exploring, heading to ticket booth", subject, pid);
    print_time("acquired a ticket", subject, pid);

    pthread_mutex_lock(&mutex);
    print_time("joined the ride queue", subject, pid);
    queue++;

    while (1) 
    {
        for (int i = 0; i < c; ++i) 
        {
            Car *car = &cars[i];
            if (car->passengers_needed > 0) 
            {
                car->pass_ids[car->boarded_count++] = pid;
                car->passengers_needed--;
                if (car->passengers_needed == 0) 
                {
                    pthread_cond_broadcast(&car->car_ready);
                }
                pthread_mutex_unlock(&mutex);
                print_time("boarded", subject, pid);

                // Wait for ride to finish
                pthread_mutex_lock(&mutex);
                pthread_cond_wait(&car->ride_done, &mutex);
                pthread_mutex_unlock(&mutex);

                print_time("deboarded", subject, pid);
                return NULL;
            }
        }
        pthread_cond_wait(&passenger_ready, &mutex);
    }
}

void *car_routine(void *arg)
{
    int cid = *(int*)arg;
    Car *car = &cars[cid];
    char *subject = "Car";

    while (1) {
        pthread_mutex_lock(&mutex);
        print_time("waiting for passengers", subject, cid);

        int wait_counter = 0;
        while (queue < p && wait_counter < w) {
            pthread_mutex_unlock(&mutex);
            sleep(1);
            wait_counter++;
            pthread_mutex_lock(&mutex);
        }

        int boarding = (queue < p) ? queue : p;
        car->passengers_needed = boarding;
        car->boarded_count = 0;
        queue -= boarding;
        pthread_cond_broadcast(&passenger_ready);

        while (car->passengers_needed > 0) {
            pthread_cond_wait(&car->car_ready, &mutex);
        }

        print_time("starting ride", subject, cid);
        pthread_mutex_unlock(&mutex);

        sleep(r);

        pthread_mutex_lock(&mutex);
        print_time("unloading", subject, cid);
        pthread_cond_broadcast(&car->ride_done);
        pthread_mutex_unlock(&mutex);
    }
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

    Car cars[c];
    // int queue = 0;
    int nextcar = 0;

    // seed random sleep
    srand(time(NULL));
    pthread_t passenger;
    pthread_t car;


    // create threads for passenger and car
    for (int i = 0; i < c; ++i) {
        cars[i].id = i + 1;
        cars[i].pass_ids = malloc(sizeof(int) * p);
        cars[i].boarded_count = 0;
        cars[i].passengers_needed = 0;
        pthread_cond_init(&cars[i].car_ready, NULL);
        pthread_cond_init(&cars[i].ride_done, NULL);
    }

    pthread_t passenger_threads[n];
    pthread_t car_threads[c];
    int pass_ids[n];
    int car_ids[c];

        for (int i = 0; i < c; ++i) {
        car_ids[i] = i + 1;
        pthread_create(&car_threads[i], NULL, car_routine, &car_ids[i]);
    }

        for (int i = 0; i < n; ++i) {
        pass_ids[i] = i + 1;
        pthread_create(&passenger_threads[i], NULL, passenger_routine, &pass_ids[i]);
        usleep(100000); 
    }

        for (int i = 0; i < n; ++i) {
        pthread_join(passenger_threads[i], NULL);
    }

    return 0;
}