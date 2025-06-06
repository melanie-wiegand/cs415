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

time_t start_time;


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
    int boarding_bool;
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


void randsleep(int min, int max)
{
    int sleeptime = (rand() % (max - min)) + min; 
    sleep(sleeptime);
}

void print_time(const char* msg, const char* subject, int pid)
{
    // no prints after sim time is up
    if (time_up)
    {
        return;
    }
    // prints current time before the message
    time_t t = time(NULL);
    int total_time = (int)difftime(t, start_time);
    
    int h = total_time / 3600;
    int m = (total_time % 3600) / 60;
    int s = total_time % 60;

    printf("[Time: %02d:%02d:%02d] %s %d %s\n", h, m, s, subject, pid, msg);
}

void* monitor_routine(void* arg)
{
    while (!time_up)
    {
        sleep(5); // print every five seconds

        pthread_mutex_lock(&mutex);

        time_t t = time(NULL);
        int total_time = (int)difftime(t, start_time);
        
        int h = total_time / 3600;
        int m = (total_time % 3600) / 60;
        int s = total_time % 60;

        printf("\n[Monitor] System State at %02d:%02d:%02d\n", h, m, s);

        // ticket queue
        printf("Ticket Queue: [");
        for (int i = ticket_front; i < ticket_rear; ++i)
        {
            printf("Passenger %d", ticket_queue[i]);
            if (i < ticket_rear - 1) printf(", ");
        }
        printf("]\n");

        // ride queue
        printf("Ride Queue: [");
        for (int i = ride_front; i < ride_rear; ++i)
        {
            printf("Passenger %d", ticket_queue[i]);
            if (i < ride_rear - 1) printf(", ");
        }
        printf("]\n");

        // car status
        for (int i = 0; i < c; ++i)
        {
            Car* car = &cars[i];
            const char* status = car->boarding_bool ? "LOADING" : "WAITING";
            printf("Car %d Status: %s (%d/%d passengers)\n", i + 1, status, car->boarded_count, p);
        }

        // passenger status
        // passengers on rides (sum boarded counts of all cars)
        int num_riding = 0;
        // passengers in queue (sum ticket and ride queues)
        int num_queued = (ride_rear - ride_front) + (ticket_rear - ticket_front);
        for (int i = 0; i < c; ++i)
        {
            num_riding += cars[i].boarded_count;
        }
        // everyone else
        int num_exploring = n - num_queued - num_riding;

       
        printf("Passengers in park: %d (%d exploring, %d in queues, %d on rides)\n\n", 
            n, num_exploring, num_queued, num_riding);

        pthread_mutex_unlock(&mutex);
    }
}

void *passenger_routine(void *arg)
{
    int pid = *(int*)arg;
    char *subject = "Passenger";

    while (!time_up)
    {
        print_time("entered the park", subject, pid);
        // rand time to start exploring
        randsleep(1, 5);
        if (time_up) return NULL;
        print_time("is exploring the park...", subject, pid);
        // rand time to explore
        randsleep(1, 10);
        if (time_up) return NULL;
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
                if (car->boarding_bool && car->passengers_needed > 0) 
                {
                    for (int j = ride_front; j < ride_rear; ++j)
                    {
                        if (ride_queue[j] == pid)
                        {
                            
                            for (int k = j; k < ride_rear - 1; ++k)
                            {
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
            // wait before exploring park again
            randsleep(1, 10);
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

        print_time("invoked load()", subject, cid + 1);

        // int boarding = ((ride_rear - ride_front) < p) ? (ride_rear - ride_front) : p;
        // if (boarding == 0)
        // {
        //     pthread_mutex_unlock(&mutex);
        //     sleep(1);
        //     continue;
        // }

        // usleep(50000);
        
        car->passengers_needed = p;
        car->boarded_count = 0;
        // queue -= boarding;

        // pthread_cond_broadcast(&passenger_ready);

        // while (car->boarded_count == 0 && !time_up)
        // {
        //     pthread_cond_wait(&passenger_ready, &mutex);
        // }

        // if (time_up) {
        //     pthread_mutex_unlock(&mutex);
        //     break;
        // }

        car->boarding_bool = 1;

        float wait_counter = 0.0;
        while (wait_counter < w && !time_up) 
        {
            pthread_cond_broadcast(&passenger_ready);

            // car full
            if (car->passengers_needed == 0)
            {
                break; 
            }

            pthread_mutex_unlock(&mutex);
            usleep(500*1000);
            wait_counter+= 0.5;
            pthread_mutex_lock(&mutex);   

        }

        car->boarding_bool = 0;
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

    start_time = time(NULL);
    
    // timer thread
    pthread_t timer;
    pthread_create(&timer, NULL, timer_routine, NULL);

    // monitor thread
    pthread_t monitor;
    pthread_create(&monitor, NULL, monitor_routine, NULL);

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

    usleep(500000);
    for (int i = 0; i < n; ++i) 
    {
        pass_ids[i] = i + 1;
        if (i % 6 == 5)
        {
            // every few passengers, wait longer (mimicking example behavior)
            randsleep(1, 5);
        }
        else
        {
            usleep(500000);
        }
        // sleep(500000);
        pthread_create(&passenger_threads[i], NULL, passenger_routine, &pass_ids[i]);
        
    }

    for (int i = 0; i < n; ++i) 
    {
        pthread_join(passenger_threads[i], NULL);
    }

    return 0;
}