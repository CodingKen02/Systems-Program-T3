/*
----------------------------------------------------------
Program: skelShm.c
Date: November 14, 2023
Student Name & NetID: Kennedy Keyes kfk38
Description: This multiple concurrent process program uses 
shared memory and semaphores to display "ascii modern art" 
composed of a series of randomly generated blocks of 2 or 
more repeating characters.
----------------------------------------------------------
*/

/* Problem 3 -- List the inlcude files you need for this program. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "semun.h"
#include "binary_sem.h"

#define SEM_KEY  IPC_PRIVATE
#define SHM_KEY  IPC_PRIVATE

/* Problem 4 -- remember to declare a structure that represents the data
stored in the shared memory */
struct SharedData 
{
    int totalBlocks;
    int randomWidth;
    // for each block, store length and character
    int blockData[20][2]; 
};

/* Problem 5 -- create a function to handle the code for the child.
Be certain to pass this function the id values for both the semaphore
and the shared memory segment */
void childProcess(int semId, int shmId) 
{
    struct SharedData *sharedData = (struct SharedData *)shmat(shmId, NULL, 0);
    if (sharedData == (void *)-1) 
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // this generates and stores the total number of blocks to generate
    sharedData->totalBlocks = rand() % 11 + 10; // total blocks between 10 and 20

    // for each block, generate and store length and character
    for (int i = 0; i < sharedData->totalBlocks; i++) 
    {
        sharedData->blockData[i][0] = rand() % 9 + 2; // length between 2 and 10
        sharedData->blockData[i][1] = rand() % 26 + 'a'; // character between 'a' and 'z'
    }

    // release the parent semaphore
    releaseSem(semId, 1);

    // reserve the child semaphore
    reserveSem(semId, 0);

    // detach the shared memory segment
    if (shmdt(sharedData) == -1) 
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // release the parent semaphore
    releaseSem(semId, 1);
}

/* Problem 6 -- create a function to handle the code for the parent.
Be certain to pass this function the id values for both the semaphore
and the shared memory segment */
void parentProcess(int semId, int shmId) 
{
    struct SharedData *sharedData = (struct SharedData *)shmat(shmId, NULL, 0);
    if (sharedData == (void *)-1) 
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // reserve the parent semaphore
    reserveSem(semId, 1);

    // generate a random width for the ASCII art
    sharedData->randomWidth = rand() % 6 + 10; // width between 10 and 15

    // output an image using the data stored in the shared memory segment
    for (int i = 0; i < sharedData->totalBlocks; i++) 
    {
        for (int j = 0; j < sharedData->blockData[i][0]; j++) 
        {
            putchar(sharedData->blockData[i][1]);
        }
        if ((i + 1) % sharedData->randomWidth == 0) 
        {
            putchar('\n');
        }
    }

    // release the child semaphore
    releaseSem(semId, 0);

    // reserve the parent semaphore
    reserveSem(semId, 1);

    // detach the shared memory segment
    if (shmdt(sharedData) == -1) 
    {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
}

/* Problem 7 -- implement function main */
int main(int argc, char *argv[]) 
{
    int semId, shmId;
    struct SharedData *sharedData;

    // create a semaphore set of size 2
    semId = semget(SEM_KEY, 2, IPC_CREAT | IPC_EXCL | 0666);
    if (semId == -1) 
    {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    // initialize the semaphores
    initSemInUse(semId, 0); // semaphore for child
    initSemAvailable(semId, 1); // semaphore for parent

    // create a segment of shared memory
    shmId = shmget(SHM_KEY, sizeof(struct SharedData), IPC_CREAT | IPC_EXCL | 0666);
    if (shmId == -1) 
    {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // attach the shared memory segment
    sharedData = (struct SharedData *)shmat(shmId, NULL, 0);
    if (sharedData == (void *)-1) 
    {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    // initialize other variables and start processes
    pid_t childPid = fork();

    if (childPid == -1) 
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (childPid == 0) 
    {
        childProcess(semId, shmId);
    } else 
    {
        parentProcess(semId, shmId);

        wait(NULL);

        // delete the shared memory
        if (shmdt(sharedData) == -1) 
        {
            perror("shmdt");
            exit(EXIT_FAILURE);
        }

        if (shmctl(shmId, IPC_RMID, NULL) == -1) 
        {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }

        // delete the semaphore
        if (semctl(semId, 0, IPC_RMID, 0) == -1) 
        {
            perror("semctl");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
