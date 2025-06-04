#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define MAX_PASSENGERS 100
#define MAX_CARS 20
#define MAX_CAPACITY 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t car_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t passenger_ready = PTHREAD_COND_INITIALIZER;


// number boarded
int boarded = 0;
// number riding
int riding = 0;

volatile int time_up = 0;


// default values for parameters
int n = 10;
int c = 2;
int p = 4;
int w = 10;
int r = 5;


// car ordering + queue
pthread_mutex_t car_order_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t car_turn_cond = PTHREAD_COND_INITIALIZER;
int current_car = 0;
int ride_queue[MAX_PASSENGERS];
int ride_front = 0;
int ride_rear = 0;

// ticket ordering + queue
pthread_mutex_t ticket_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ticket_cond = PTHREAD_COND_INITIALIZER;
int ticket_queue[MAX_PASSENGERS];
int ticket_front = 0; 
int ticket_rear = 0;



//indexing
// int current_car_turn = 0; 
// int total_cars = 3;

// car struct
typedef struct
{
    int id;
    int boarded_count;
    int pass_ids[MAX_CAPACITY];
    int passengers_needed;
    pthread_cond_t car_ready;
    pthread_cond_t ride_done;
    // pthread_mutex_t car_mutex = PTHREAD_MUTEX_INITIALIZER;
} Car;

// list of cars
Car cars[MAX_CARS];

// add passenger to queue
void enqueue(int queue[], int *rear, int id) 
{
    queue[(*rear)++] = id;
}

// remove passenger from queue
int dequeue(int queue[], int *front, int rear) 
{
    if (*front == rear) return -1;
    return queue[(*front)++];
}

// timer
void* timer_routine(void* arg) 
{
    sleep(30);  
    time_up = 1;
    printf("[Monitor] Simulation time ended.\n");

    // pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&passenger_ready);
    for (int i = 0; i < c; ++i)
    {
        pthread_cond_broadcast(&cars[i].ride_done);
    }
    // pthread_mutex_unlock(&mutex);

    return NULL;
}


void randsleep()
{
    int sleeptime = (rand() % 10) + 1; // 1-10 s
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

    while (!time_up)
    {
        print_time("entered the park", subject, pid);
        // rand time to start exploring
        randsleep();
        print_time("is exploring the park...", subject, pid);
        // rand time to explore
        randsleep();
        print_time("finished exploring, heading to ticket booth", subject, pid);
        
        pthread_mutex_lock(&ticket_mutex);
        // join ticket queue
        print_time("waiting in ticket queue", subject, pid);
        enqueue(ticket_queue, &ticket_rear, pid);
        while (ticket_queue[ticket_front] != pid) 
        {
            pthread_cond_wait(&ticket_cond, &ticket_mutex);
        }
        // first in line gets ticket
        dequeue(ticket_queue, &ticket_front, ticket_rear);
        print_time("acquired a ticket", subject, pid);
        pthread_cond_broadcast(&ticket_cond);
        pthread_mutex_unlock(&ticket_mutex);

        pthread_mutex_lock(&mutex); 
        // join ride queue
        enqueue(ride_queue, &ride_rear, pid);
        print_time("joined the ride queue", subject, pid);
        pthread_cond_broadcast(&passenger_ready);
        pthread_mutex_unlock(&mutex);

        int boarded = 0;

        while (!boarded && !time_up) 
        {
           pthread_mutex_lock(&mutex);

            for (int i = 0; i < c; ++i) 
            {
                Car *car = &cars[i];
                if (car->passengers_needed > 0) 
                {
                    if (ride_queue[j] == pid) {
                    // Remove from queue (shift the rest)
                    for (int k = j; k < ride_rear - 1; ++k) {
                        ride_queue[k] = ride_queue[k + 1];
                    }
                    ride_rear--;
                    // board
                    car->pass_ids[car->boarded_count++] = pid;
                    car->passengers_needed--;
                    
                    pthread_cond_broadcast(&passenger_ready);

                    // if (car->passengers_needed == 0) 
                    // {
                    //     pthread_cond_signal(&car->car_ready);
                    // }
                    pthread_mutex_unlock(&mutex);

                    char b_msg[100];
                    snprintf(b_msg, sizeof(b_msg), "boarded Car %d", car->id);
                    print_time(b_msg, subject, pid);
                    
                    // wait for ride end
                    pthread_mutex_lock(&mutex);
                    pthread_cond_wait(&car->ride_done, &mutex);
                    pthread_mutex_unlock(&mutex);

                    char db_msg[100];
                    snprintf(db_msg, sizeof(db_msg), "deboarded Car %d", car->id);
                    print_time(db_msg, subject, pid);
                    
                    boarded = 1;
                    break;
                }
            }

            if (!boarded && !time_up)
            {
                pthread_cond_wait(&passenger_ready, &mutex);
            }

            pthread_mutex_unlock(&mutex);
        }

        if (!time_up)
        {
            randsleep();
        }
    
    }
    return NULL;
}

void *car_routine(void *arg)
{
    int cid = *(int*)arg;
    Car *car = &cars[cid];
    char *subject = "Car";

    while (!time_up) {

        pthread_mutex_lock(&car_order_mutex);
        while (current_car != cid) 
        {
            pthread_cond_wait(&car_turn_cond, &car_order_mutex);
        }
        pthread_mutex_unlock(&car_order_mutex);

        pthread_mutex_lock(&mutex);
        // print_time("waiting for passengers", subject, cid);

        // int wait_counter = 0;
        // while ((ride_rear - ride_front) == 0 && wait_counter < w && !time_up) 
        // {
        //     pthread_mutex_unlock(&mutex);
        //     sleep(1);
        //     wait_counter++;
        //     pthread_mutex_lock(&mutex);
        // }

        // waits for at least one passenger to be in queue
        while ((ride_rear - ride_front) == 0 && !time_up) {
            pthread_cond_wait(&passenger_ready, &mutex);
        }

        if (time_up) 
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        int boarding = ((ride_rear - ride_front) < p) ? (ride_rear - ride_front) : p;
        // if (boarding == 0)
        // {
        //     pthread_mutex_unlock(&mutex);
        //     sleep(1);
        //     continue;
        // }

        // usleep(50000);
        print_time("invoked load()", subject, cid + 1);
        car->passengers_needed = boarding;
        car->boarded_count = 0;
        // queue -= boarding;

        pthread_cond_broadcast(&passenger_ready);

        while (car->boarded_count == 0 && !time_up)
        {
            pthread_cond_wait(&passenger_ready, &mutex);
        }

        // if (time_up) {
        //     pthread_mutex_unlock(&mutex);
        //     break;
        // }

        int wait_counter = 0;
        while (wait_counter < w && !time_up) 
        {
            pthread_mutex_unlock(&mutex);
            sleep(1);
            wait_counter++;
            pthread_mutex_lock(&mutex);   

            // car full
            // if (car->passengers_needed == 0)
            // {
            //     break; 
            // }
        }

        print_time("starting ride", subject, cid + 1);
        pthread_mutex_unlock(&mutex);

        sleep(r);

        pthread_mutex_lock(&mutex);
        print_time("unloading", subject, cid + 1);
        pthread_cond_broadcast(&car->ride_done);

        // pthread_cond_broadcast(&passenger_ready);

        pthread_mutex_unlock(&mutex);

        pthread_mutex_lock(&car_order_mutex);
        current_car = (current_car + 1) % c;
        pthread_cond_broadcast(&car_turn_cond);
        pthread_mutex_unlock(&car_order_mutex);
    }

    return NULL;
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
    printf("- Park exploration time: 1-10 seconds\n");
    printf("- Car waiting period: %d seconds\n", w);
    printf("- Ride duration: %d seconds\n\n", r);

    // Car cars[c];
    // int queue = 0;
    // int nextcar = 0;

    // seed random sleep
    srand(time(NULL));
    // pthread_t passenger;
    // pthread_t car;

    pthread_t timer;
    pthread_create(&timer, NULL, timer_routine, NULL);

    pthread_t passenger_threads[n];
    pthread_t car_threads[c];
    int pass_ids[n];
    int car_ids[c];

    // create threads for passenger and car
    for (int i = 0; i < c; ++i) {
        cars[i].id = i + 1;
        // cars[i].pass_ids = malloc(sizeof(int) * p);
        cars[i].boarded_count = 0;
        cars[i].passengers_needed = 0;
        pthread_cond_init(&cars[i].car_ready, NULL);
        pthread_cond_init(&cars[i].ride_done, NULL);
        car_ids[i] = i;
        pthread_create(&car_threads[i], NULL, car_routine, &car_ids[i]);
    }

    for (int i = 0; i < n; ++i) 
    {
        pass_ids[i] = i + 1;
        pthread_create(&passenger_threads[i], NULL, passenger_routine, &pass_ids[i]);
        usleep(1000000); 
    }

    for (int i = 0; i < n; ++i) 
    {
        pthread_join(passenger_threads[i], NULL);
    }

    return 0;
}