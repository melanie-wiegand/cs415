#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>



#define MAX_PASSENGERS 100
#define MAX_CARS 20
#define MAX_CAPACITY 10

// general mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// passenger mutex
pthread_cond_t passenger_ready = PTHREAD_COND_INITIALIZER;

// default values for parameters
int n = 10;
int c = 2;
int p = 4;
int w = 10;
int r = 5;

// flag for end of sim
volatile int time_up = 0;

// initialize start time
time_t start_time;

// pipe for writing monitor info
int monitor_writespace = -1;

// keep track of passengers created
int passenger_count = 0;
pthread_mutex_t passenger_count_mutex = PTHREAD_MUTEX_INITIALIZER;

// summary message counters
int served = 0;
int totalrides = 0;
double ticketwait;
double ridewait;
pthread_mutex_t summary_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    int running_bool;
    // pthread_mutex_t car_mutex = PTHREAD_MUTEX_INITIALIZER;
} Car;

// initialize list of cars
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

// thread to stop simulation after 1 min (30s for testing)
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

// function to randomly make passengers wait
void randsleep(int min, int max)
{
    int sleeptime = (rand() % (max - min)) + min; 
    sleep(sleeptime);
}

// use ipc pipe instead of printing directly
void print_from_monitor(const char* status, const char* subject, int pid) {
    if (monitor_writespace == -1 || time_up) 
    {
        return;
    }

    char buff[256];
    time_t now = time(NULL);
    int diff_time = (int)difftime(curr_time, start_time);
    
    int h = total_time / 3600;
    int m = (total_time % 3600) / 60;
    int s = total_time % 60;
    
    // char h_str[10];
    // char m_str[10];
    // char s_str[10];
    // sprintf(h_str, sizeof(h_str), "%s", h);
    // sprintf(m_str, sizeof(m_str), "%s", m);
    // sprintf(s_str, sizeof(s_str), "%s", s);

    snprintf(buff, sizeof(buff), "[Time: %02d:%02d:%02d] %s %d %s\n", h, m, s, subject, pid, status);
    // write to pipe
    write(monitor_writespace, buff, strlen(buff));
}

// function that puts the timestamps in front of messages
// void print_from_monitor(const char* msg, const char* subject, int pid)
// {
//     // no prints after sim time is up
//     if (time_up)
//     {
//         return;
//     }
//     // prints current time before the message
//     time_t t = time(NULL);
//     int total_time = (int)difftime(t, start_time);
    
//     int h = total_time / 3600;
//     int m = (total_time % 3600) / 60;
//     int s = total_time % 60;

//     printf("[Time: %02d:%02d:%02d] %s %d %s\n", h, m, s, subject, pid, msg);
// }

// // thread to print status message every 5 seconds
// void* monitor_routine(void* arg)
// {
//     while (!time_up)
//     {
//         sleep(5); // print every five seconds

//         pthread_mutex_lock(&mutex);

//         // calculate elapsed time
//         time_t t = time(NULL);
//         int total_time = (int)difftime(t, start_time);
        
//         int h = total_time / 3600;
//         int m = (total_time % 3600) / 60;
//         int s = total_time % 60;

//         printf("\n[Monitor] System State at %02d:%02d:%02d\n", h, m, s);

//         // ticket queue
//         printf("Ticket Queue: [");
//         for (int i = ticket_front; i < ticket_rear; ++i)
//         {
//             printf("Passenger %d", ticket_queue[i]);
//             if (i < ticket_rear - 1) printf(", ");
//         }
//         printf("]\n");

//         // ride queue
//         printf("Ride Queue: [");
//         for (int i = ride_front; i < ride_rear; ++i)
//         {
//             printf("Passenger %d", ride_queue[i]);
//             if (i < ride_rear - 1) printf(", ");
//         }
//         printf("]\n");

//         // car status
//         for (int i = 0; i < c; ++i)
//         {
//             Car* car = &cars[i];
//             const char* status;
//             if (car->running_bool)
//             { 
//                 status = "RUNNING";
//             } 
//             else if (car->boarding_bool)
//             {
//                 status = "LOADING";
//             }
//             else
//             {
//                 status = "WAITING";
//             }
//             printf("Car %d Status: %s (%d/%d passengers)\n", car->id, status, car->boarded_count, p);
//         }

//         // get vals for num passengers
//         pthread_mutex_lock(&passenger_count_mutex);
//         int total_created = passenger_count;
    
//         // passenger status
//         // passengers on rides (sum boarded counts of all cars)
//         int num_riding = 0;
//         // passengers in queue (sum ticket and ride queues)
//         int num_queued = (ride_rear - ride_front) + (ticket_rear - ticket_front);
//         for (int i = 0; i < c; ++i)
//         {
//             // only track cars not boarding
//             if (!cars[i].boarding_bool)
//             {
//                 num_riding += cars[i].boarded_count;
//             }
//         }

//         int exploring_now = total_created - num_queued - num_riding;\
//         printf("Passengers in park: %d (%d exploring, %d in queues, %d on rides)\n\n", 
//              total_created, exploring_now, num_queued, num_riding);

//         pthread_mutex_unlock(&passenger_count_mutex);
//         pthread_mutex_unlock(&mutex);
//     }
// }

// controls passenger threads
void *passenger_routine(void *arg)
{
    pthread_mutex_lock(&passenger_count_mutex);
    passenger_count++;
    pthread_mutex_unlock(&passenger_count_mutex);

    int pid = *(int*)arg;
    char *subject = "Passenger";
    int first = 1;

    while (!time_up)
    {
        // take time to enter if first spawning
        if (first)
        {
            print_from_monitor("entered the park", subject, pid);
            // rand time to start exploring
            randsleep(1, 5);
        }

        print_from_monitor("is exploring the park...", subject, pid);
        // rand time to explore
        randsleep(1, 8);
        if (time_up) 
        {
            return NULL;
        }
        print_from_monitor("finished exploring, heading to ticket booth", subject, pid);
        
        // join ticket queue
        pthread_mutex_lock(&ticket_mutex);
        print_from_monitor("waiting in ticket queue", subject, pid);
        // mark start time
        time_t ticketstart = time(NULL);
        enqueue(ticket_queue, &ticket_rear, pid);
        while (ticket_queue[ticket_front] != pid) 
        {
            pthread_cond_wait(&ticket_cond, &ticket_mutex);
        }
        // first in line gets ticket
        dequeue(ticket_queue, &ticket_front, ticket_rear);
        print_from_monitor("acquired a ticket", subject, pid);
        // end time
        time_t ticketend = time(NULL);

        // calculate ticket wait time
        pthread_mutex_lock(&summary_mutex);
        ticketwait += difftime(ticketend, ticketstart);
        pthread_mutex_unlock(&summary_mutex);

        pthread_cond_broadcast(&ticket_cond);
        pthread_mutex_unlock(&ticket_mutex);
        
        // join ride queue
        pthread_mutex_lock(&mutex);      
        enqueue(ride_queue, &ride_rear, pid);
        print_from_monitor("joined the ride queue", subject, pid);
        // start time
        time_t ridestart = time(NULL);
        pthread_cond_broadcast(&passenger_ready);
        pthread_mutex_unlock(&mutex);

        int boarded = 0;

        // when not boarded, look to board
        while (!boarded && !time_up) 
        {
           pthread_mutex_lock(&mutex);
            for (int i = 0; i < c; ++i) 
            {
                Car *car = &cars[i];
                // find car with open seats
                if (car->boarding_bool && car->passengers_needed > 0) 
                {
                    for (int j = ride_front; j < ride_rear; ++j)
                    {
                        if (ride_queue[j] == pid)
                        {
                            // board car
                            car->pass_ids[car->boarded_count++] = pid;
                            car->passengers_needed--;      
                            pthread_cond_broadcast(&passenger_ready);
                            pthread_mutex_unlock(&mutex);

                            char b_msg[100];
                            snprintf(b_msg, sizeof(b_msg), "boarded Car %d", car->id);
                            print_from_monitor(b_msg, subject, pid);
                            // end time
                            time_t rideend = time(NULL);

                            // calculate ride wait time
                            pthread_mutex_lock(&summary_mutex);
                            ridewait += difftime(rideend, ridestart);
                            pthread_mutex_unlock(&summary_mutex);
                            
                            // wait for ride end
                            pthread_mutex_lock(&mutex);
                            pthread_cond_wait(&car->ride_done, &mutex);
                            pthread_mutex_unlock(&mutex);

                            // deboard
                            char db_msg[100];
                            snprintf(db_msg, sizeof(db_msg), "deboarded Car %d", car->id);
                            print_from_monitor(db_msg, subject, pid);
                            
                            // exit out of ride loop
                            boarded = 1;
                            break;
                        }
                    }
                }
            }

            if (!boarded && !time_up)
            {
                // wait for car to start boarding
                pthread_cond_wait(&passenger_ready, &mutex);
            }

            pthread_mutex_unlock(&mutex);
        }

        if (!time_up)
        {
            // wait before exploring park again
            randsleep(1, 8);
        }
    
    }
    return NULL;
}


// controls car threads
void *car_routine(void *arg)
{
    int cid = *(int*)arg;
    Car *car = &cars[cid];
    char *subject = "Car";

    while (!time_up) {
        pthread_mutex_lock(&car_order_mutex);
        while (current_car != cid) 
        {
            // wait for turn to board
            pthread_cond_wait(&car_turn_cond, &car_order_mutex);
        }
        pthread_mutex_unlock(&car_order_mutex);
        pthread_mutex_lock(&mutex);


        // waits for at least one passenger to be in queue
        while ((ride_rear - ride_front) == 0 && !time_up)
        {
            pthread_cond_wait(&passenger_ready, &mutex);
        }

        if (time_up) 
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // start boarding
        printf("\n");
        print_from_monitor("invoked load()", subject, cid + 1);

        car->passengers_needed = p;
        car->boarded_count = 0;
        car->boarding_bool = 1;

        float wait_counter = 0.0;
        // if not enough passengers, leave after w seconds
        while (wait_counter < w && !time_up) 
        {
            pthread_cond_broadcast(&passenger_ready);
            // leave if car full
            if (car->passengers_needed == 0)
            {
                char msg[200];
                snprintf(msg, sizeof(msg), "is full with %d passengers", p);
                printf("\n");
                print_from_monitor(msg, subject, cid + 1);
                break; 
            }
            pthread_mutex_unlock(&mutex);
            usleep(500000);
            wait_counter+= 0.5;
            pthread_mutex_lock(&mutex);   
        }

        // stop boarding
        car->boarding_bool = 0;

        // dequeue passengers right before ride begins
        for (int i = 0; i < car->boarded_count; ++i) 
        {
            int pid = car->pass_ids[i];
            for (int j = ride_front; j < ride_rear; ++j) 
            {
                if (ride_queue[j] == pid) {
                    for (int k = j; k < ride_rear - 1; ++k) 
                    {
                        ride_queue[k] = ride_queue[k + 1];
                    }
                    ride_rear--;
                    break;
                }
            }
        }

        // let next car board while this car runs
        pthread_mutex_lock(&car_order_mutex);
        current_car = (current_car + 1) % c;
        pthread_cond_broadcast(&car_turn_cond);
        pthread_mutex_unlock(&car_order_mutex);

        // begin ride
        car->running_bool = 1;
        print_from_monitor("departed for its run", subject, cid + 1);
        pthread_mutex_unlock(&mutex);

        sleep(r);

        // ride over
        print_from_monitor("finished its run", subject, cid + 1);
        car->running_bool = 0;

        // increment rides and served passengers
        pthread_mutex_lock(&summary_mutex);
        totalrides++;
        served += car->boarded_count;
        pthread_mutex_unlock(&summary_mutex);

        // reset boarded count
        car->boarded_count = 0;

        // signal end
        pthread_mutex_lock(&mutex);
        print_from_monitor("invoked unload()", subject, cid + 1);
        pthread_cond_broadcast(&car->ride_done);
        pthread_mutex_unlock(&mutex);
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
    printf("- Park exploration time: 1-8 seconds\n");
    printf("- Car waiting period: %d seconds\n", w);
    printf("- Ride duration: %d seconds\n\n", r);

    // seed random sleep
    srand(time(NULL));

    // set start time
    start_time = time(NULL);

    // replace monitor thread with pipe for IPC
    // one index for reading [0], one for writing [1]
    int monitorpipe[2];
    if (pipe(monitorpipe) == -1)
    {
        perror("monitor pipe failed");
        exit(1);
    }

    // monitor is child of parent simulation
    pid_t monitorpid = fork();

    // error
    if (monitorpid < 0)
    {
        perror("could not fork monitor");
        exit(1);
    }

    // monitor process (child)
    else if (monitorpid == 0)
    {
        // close writespace
        close(monitorpipe[1]);
        FILE *readspace = fdopen(monitorpipe[0], "r");
        char buff[200];

        // print contents of readspace
        while (fgets(buff, sizeof(buff), readspace))
        {
            printf("%s", buff);
        }

        fclose(readspace);
        exit(0);
    }

    // simulation process (parent)
    // close readspace
    close(monitorpipe[0]);
    // assign to global variable
    monitor_writespace = monitorpipe[1];

    
    // timer thread
    pthread_t timer;
    pthread_create(&timer, NULL, timer_routine, NULL);

    // // monitor thread
    // pthread_t monitor;
    // pthread_create(&monitor, NULL, monitor_routine, NULL);


    // arrays for passenger and car threads
    pthread_t passenger_threads[n];
    pthread_t car_threads[c];
    // store pids
    int pass_ids[n];
    int car_ids[c];

    // create threads for cars
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

    // small wait before creating passenger threads
    usleep(400000);
    for (int i = 0; i < n; ++i) 
    {
        pass_ids[i] = i + 1;
        if (i % 6 == 5)
        {
            // every few passengers, wait longer (mimicking example behavior)
            randsleep(4, 10);
        }
        else
        {
            // delay between passengers entering
            usleep(500000);
        }
        pthread_create(&passenger_threads[i], NULL, passenger_routine, &pass_ids[i]);       
    }

    // join threads 
    for (int i = 0; i < n; ++i) 
    {
        pthread_join(passenger_threads[i], NULL);
    }

    for (int i = 0; i < c; ++i) 
    {
        pthread_join(car_threads[i], NULL);
    }

    pthread_join(timer, NULL);


    // pthread_join(monitor, NULL);

    close(monitor_writespace);
    // wait for monitor to close
    waitpid(monitorpid, NULL, 0);
    
    // calculate avg waits
    double ticketavg = ticketwait / served;
    double rideavg = ridewait / served;
    double utilization = (((double)served / (double)totalrides)/(double)p) * 100;

    printf("\n[Monitor] FINAL STATISTICS:\n");
    printf("Total simulation time: 00:00:30\n");
    printf("Total passengers served: %d\n", served);
    printf("Total rides completed: %d\n", totalrides);
    printf("Average wait time in ticket queue: %.1f seconds\n", ticketavg);
    printf("Average wait time in ride queue: %.1f seconds\n", rideavg);
    printf("Average car utilization: %.0f%% (%.1f/%d passengers per ride)\n", 
               utilization, (double)served / totalrides, p);

    return 0;
}