/* 
    CSD4676
    Dimitrios Makrogiannis
    AKA PercySS
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define MAX_STUDY_HALL 8 
#define MAX_WAITING_ROOM 40

/* ---------- Structs ---------- */
typedef struct {
    int id;
    float study_time;
    int position;
    pthread_t thread;
} student_T;

typedef struct {
    student_T **studentsIn;
    int status; /* 0 == empty
                  > 1 == is getting fιlled / is full
                */
} study_hall_T;

typedef struct {
    student_T **students;
    int size;
    int front;
    int rear;
    pthread_mutex_t queue_mutex;
} waiting_queue_T;


/* ---------- Function declarations ---------- */
void init_queue();
void init_hall();
void init_students_array();
int empty_hall();
void is_full();
int empty_waiting_room();
void enqueue(waiting_queue_T *, student_T *);
student_T *dequeue(waiting_queue_T *);
void *student_thread(void *arg);
void copyrights();
void cleanup(int);
void print_grids();


/* ---------- Global variables ---------- */

/* Init semaphores */
sem_t study_hall_sem;
sem_t waiting_room_sem;

/* Init mutexes */
pthread_mutex_t study_hall_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t waiting_room_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t printing_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Init cond */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* Global hall and waiting room */
study_hall_T *study_hall;
waiting_queue_T *waiting_room;
student_T **students;

/* Global day_students */
int day_students;
int empty;
int entry;
int sem_value;

int main() {
    int i;

    /* Init semaphores */
    sem_init(&study_hall_sem, 0, MAX_STUDY_HALL - 1);
    sem_init(&waiting_room_sem, 0, MAX_WAITING_ROOM);

    /* Init mutexes */
    pthread_mutex_init(&study_hall_mutex, NULL);
    pthread_mutex_init(&waiting_room_mutex, NULL);
    pthread_mutex_init(&printing_mutex, NULL);

    /* I seed the rand() function */
    srand(time(NULL));


    /* I check for valid values of day_students (20-40) */
    printf("Please enter the number of students for the day (20-40): ");
    while (scanf("%d", &day_students) != 1 || day_students < 20 || day_students > 40) {
        printf("Invalid input. Please enter the number of students for the day (20-40): ");
        while (getchar() != '\n');                                  /* Clear input buffer */
    }

    
    /* Init students array */
    init_students_array();

    /* Init study hall */
    init_hall(study_hall, day_students);

    /* Init waiting room */ 
    init_queue(waiting_room, day_students);


    /* I create the threads */
    for (i = 0; i < day_students; i++) {
        students[i]->id = i + 1;
        students[i]->study_time = 5 + (rand() % 10) + 1;
        pthread_create(&students[i]->thread, NULL, student_thread, students[i]);
    }    

    for (i = 0; i < day_students; i++) {
        /* Here i join all the threads */
        pthread_join(students[i]->thread, NULL);
    }

    /* I clean up the memory */
    cleanup(day_students);

    copyrights();
    return 0;
}


/* ---------- Function implementations ---------- */

void *student_thread(void *arg) {
    student_T *student = (student_T*)arg;
    student_T *temp;

    pthread_mutex_lock(&study_hall_mutex);
    is_full();
    
    if (entry) {

        study_hall->studentsIn[study_hall->status] = student;
        student->position = study_hall->status;
        study_hall->status++;

        pthread_mutex_lock(&printing_mutex);
        printf("\nStudent with id: %d was born.\n", student->id);
        printf("Student %d entered the study hall.\n", student->id);
        pthread_mutex_unlock(&printing_mutex);
        print_grids();
        pthread_mutex_unlock(&study_hall_mutex);
        
    } else {
        pthread_mutex_lock(&printing_mutex);
        printf("\nStudent with id: %d was born.\n", student->id);
        printf("Student %d entered the waiting room.\n", student->id);
        pthread_mutex_unlock(&printing_mutex);
        
        enqueue(waiting_room, student);
        
        pthread_mutex_unlock(&study_hall_mutex);

        /* I wait for the study hall to be empty again */
        
        temp = dequeue(waiting_room);
        pthread_mutex_lock(&study_hall_mutex);
        is_full();
        study_hall->studentsIn[study_hall->status] = temp;
        student->position = study_hall->status;
        study_hall->status++;
        empty = empty_hall();
        is_full();
        pthread_mutex_lock(&printing_mutex);
        printf("\nStudent %d entered the study hall.\n", student->id);
        pthread_mutex_unlock(&printing_mutex);

        
        pthread_mutex_unlock(&study_hall_mutex);
        print_grids();

    }

    sleep(student->study_time);
    pthread_mutex_lock(&printing_mutex);
    printf("\nStudent %d exited study hall after studying for %0.2f secs.\n", student->id, student->study_time);
    pthread_mutex_unlock(&printing_mutex);
    pthread_mutex_lock(&study_hall_mutex);
    study_hall->studentsIn[student->position]->id = -1;
    study_hall->studentsIn[student->position]->study_time = -1;
    is_full();
    pthread_mutex_unlock(&study_hall_mutex);
    print_grids();
    empty = empty_hall();

    /* I have to return void * */
    return NULL;
}

void init_students_array() {
    int i;
    students = (student_T**)malloc(sizeof(student_T**) * day_students);
    if (students == NULL) {
        fprintf(stderr, "Error allocating memory for students");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < day_students; i++) {
        students[i] = (student_T*)malloc(sizeof(student_T*));
        
        if (students[i] == NULL) {
            fprintf(stderr, "Error allocating memory for students");
            exit(EXIT_FAILURE);
        }

        students[i]->id = -1;
        students[i]->study_time = -1;
        students[i]->position = -1;

        
    }
}


void init_hall() {
    int i;
    entry = 1;
    study_hall = (study_hall_T*)malloc(sizeof(study_hall_T));
    if (study_hall == NULL) {
        fprintf(stderr, "Error allocating memory for study hall\n");
        exit(EXIT_FAILURE);
    }
    study_hall->status = 0;
    study_hall->studentsIn = (student_T**)malloc(sizeof(student_T*) * MAX_STUDY_HALL);

    if (study_hall->studentsIn == NULL) {
        fprintf(stderr, "Error allocating memory for students in study hall\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < MAX_STUDY_HALL; i++) {
        study_hall->studentsIn[i] = (student_T*)malloc(sizeof(student_T));
        if (study_hall->studentsIn[i] == NULL) {
            fprintf(stderr, "Error allocating memory for students in study hall\n");
            exit(EXIT_FAILURE);
        }
        study_hall->studentsIn[i]->id = -1;
        study_hall->studentsIn[i]->study_time = -1;
        study_hall->studentsIn[i]->position = -1;
    } 
}

void init_queue() {
    int i;

    /* Queue - waiting room initiation */
    waiting_room = (waiting_queue_T*)malloc(sizeof(waiting_queue_T));
    if (waiting_room == NULL) {
        fprintf(stderr, "Error allocating memory for waiting room\n");
        exit(EXIT_FAILURE);
    }
     
    waiting_room->size = MAX_WAITING_ROOM;
    waiting_room->front = 0;
    waiting_room->rear = -1;
    pthread_mutex_init(&waiting_room->queue_mutex, NULL);
    
    /* Students queue-array initiation */
    waiting_room->students = (student_T**)malloc(sizeof(student_T**) * (MAX_WAITING_ROOM));
    for (i = 0; i < waiting_room->size; i++) {
        /* I initiate the waiting students with false values */
        waiting_room->students[i] = (student_T*)malloc(sizeof(student_T*));
        waiting_room->students[i]->id = -1;
        waiting_room->students[i]->study_time = -1;
        waiting_room->students[i]->position = -1;
    }

    if (waiting_room->students == NULL) {
        fprintf(stderr, "Error allocating memory for students");
        exit(EXIT_FAILURE);
    }  
}

void enqueue(waiting_queue_T *queue, student_T *student) {
    pthread_mutex_lock(&queue->queue_mutex);

    /* Checking if I can place a student in */
    if (queue->rear == queue->size - 1) {
        printf("Queue is full\n");
        return;
    }
    

    queue->rear++;
    queue->students[queue->rear] = student;
    print_grids();
    pthread_mutex_unlock(&queue->queue_mutex);
}

student_T *dequeue(waiting_queue_T *queue) {
    student_T *student;
    
    while (!entry || !empty) {
            sleep(1);
    }

    pthread_mutex_lock(&queue->queue_mutex);
    if (queue->front == queue->rear + 1) {
        printf("Queue is empty\n");
        return NULL;
    }


    student = queue->students[queue->front];
    queue->front++;
    pthread_mutex_unlock(&queue->queue_mutex);
    return student;
}



void print_grids() {
    int i;
    pthread_mutex_lock(&printing_mutex);

    printf("\nStudy hall:|");
    for (i = 0; i < MAX_STUDY_HALL; i++) {
        if (study_hall->studentsIn[i]->id <= 0) {
            printf(" |");
        } else {
            printf("%d|", study_hall->studentsIn[i]->id);
        }
    }

    printf("\nWaiting room:|");
    for (i = waiting_room->front; i <= waiting_room->rear; i++) {
        if (waiting_room->students[i]->id < 0) {
            printf(" |");
        } else {
            printf("%d|", waiting_room->students[i]->id);
        }
    }
    printf("\n\n");
    pthread_mutex_unlock(&printing_mutex);
}

/* Function to check if the waiting room is empty */
int empty_waiting_room() {
    return waiting_room->front > waiting_room->rear;
}

/* Function to check if the study hall is empty */
int empty_hall() {
    int i;
    for (i = 0; i < MAX_STUDY_HALL; i++) {
        if (study_hall->studentsIn[i]->id != -1) {
            return 0;
        }
    }

    entry = 1;
    study_hall->status = 0;
    return 1;
}

void is_full() {
    int i, ctr = 0;
    for (i = 0; i < MAX_STUDY_HALL; i++) {
        if (study_hall->studentsIn[i]->id != -1) {
            ctr++;
        }
    }

    if (ctr == MAX_STUDY_HALL) {
        entry = 0;
    } else if (ctr == 0) {
        entry = 1;
    }
}

void cleanup(int day_students) {
    int i;
    free(waiting_room->students);
    free(waiting_room);

    for (i = 0; i < day_students; i++) {
        free(students[i]);
    }
    free(students);
    free(study_hall);
    
    pthread_mutex_destroy(&printing_mutex);
    pthread_mutex_destroy(&study_hall_mutex);
    pthread_mutex_destroy(&waiting_room->queue_mutex);
    pthread_cond_destroy(&cond);
}

void copyrights() {
    printf("© 2020 - 2023 https://github.com/PercySS - All Rights Reserved.\n");
    printf("Last Updated : 01/20/2023\n");
    exit(0);
}