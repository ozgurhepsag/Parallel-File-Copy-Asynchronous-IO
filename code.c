#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aio.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

// IMPORTANT TO KNOW: File paths for destination and source file must be given like "/home/ozgur/Desktop/". 
// Please mind this. The path must be end with "/"

#define FILE_SIZE 30001 // Size with byte
#define MAX_FILE_SIZE 1024 * 100 // Max file size is 100 MB
sem_t mutex; // Mutex is using for printing thread percentage

struct arguments_struct // Keeps the informations about threads
{
    int thread_no;
    long offset;
    long buffer_size;
    int source_file;
    int destination_file;
};

void *ascopy(void *thread_arguments)
{
    struct aiocb aio;
    struct arguments_struct *args = (struct arguments_struct *)thread_arguments; // Get information for each thread from this struct in thread function.

    sem_wait(&mutex);
    printf("\n%d. Thread has created with Offset: %d and Buffer Size: %d \n", args->thread_no, args->offset, args->buffer_size);
    sem_post(&mutex);

    char *buffer;
    int err, ret;
    int source_fd = args->source_file;
    int dest_fd = args->destination_file;

    buffer = (char *)malloc(args->buffer_size * sizeof(char)); // Buffer is ready to use in read and write operations.
    memset(&aio, 0, sizeof(struct aiocb)); // memset() is used to fill a block of memory with a particular value.
    memset(buffer, 0, sizeof(buffer));

    // Variables in aio struct filled with essential values for each thread.
    aio.aio_fildes = source_fd; // Source file descriptor is assigned for reading operation.
    aio.aio_buf = buffer;
    aio.aio_nbytes = args->buffer_size; 
    aio.aio_offset = args->offset; // offset is different for each thread

    sem_wait(&mutex);
    printf("\n%d. Thread is ready to start read the source file.          PROGRESS (%%0): [                ]\n", args->thread_no);
    sem_post(&mutex);

    aio_read(&aio); //read operation

    while (aio_error(&aio) == EINPROGRESS) 
        ;

    err = aio_error(&aio);
    ret = aio_return(&aio);

    if (err != 0)
    {
        printf("\nError at aio_error(): %s\n", strerror(err));
        close(source_fd);
        exit(2);
    }

    if (ret != args->buffer_size)
    {
        printf("Error at aio_return()\n");
        close(source_fd);
        exit(2);
    }

    aio.aio_fildes = dest_fd; // Destination file descriptor is assigned for write operation.

    aio_write(&aio); // writing operation

    sem_wait(&mutex);
    printf("\n%d. Thread has finished reading and is ready to write.          PROGRESS (%%50): [########        ]\n", args->thread_no);  
    sem_post(&mutex);

    while (aio_error(&aio) == EINPROGRESS)
        ;

    err = aio_error(&aio);

    if (err != 0)
    {
        printf("Error at aio_error(): %d\n", err);
        close(dest_fd);
        exit(2);
    }

    ret = aio_return(&aio);

    if (ret != args->buffer_size)
    {
        printf("Error at aio_return()\n");
        close(dest_fd);
        exit(2);
    }

    sem_wait(&mutex);
    printf("\n%d. Thread has finished writing.          PROGRESS (%%100): [################]\n", args->thread_no);
    printf("\n%d. Thread has finished its work.\n", args->thread_no);               
    sem_post(&mutex);

    pthread_exit(NULL);
}

void source_file_creator(char *souce_path)
{
    long size = FILE_SIZE;
    long i;

    FILE *file;
    file = fopen(souce_path, "w"); // Firstly, create file
    fclose(file);

    file = fopen(souce_path, "a"); // Then, open the source file with "a" (Append).

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (i = 0; i < size; i++)
    {
        int key = rand() % strlen(charset);
        fprintf(file, "%c", charset[key]); // Write random char until the source file size is desired size.
    }

    fclose(file);
}

int main(int argc, char *argv[])
{
    int source_fd = 0; // Source file descriptor
    int destination_fd = 0; // Destination file descriptor
    int thread_number; // Thread number can be maximum 10.

    if (argc < 4)
    {
        printf("You have entered less arguments, Please try again.\nYou need to enter following arguments in order; code.out, source path, destination path and thread number.\n");
        return 0;
    }

    if (argc > 4)
    {
        printf("You have entered too much arguments, Please try again.\nYou need to enter following arguments in order; code.out, source path, destination path and thread number.\n");
        return 0;
    }

    char *source_filename = "source.txt"; // source file name must be source.txt
    char *destination_filename = "destination.txt"; // destination file name must be destionation.txt
    char *source_path; // keeps the path for source file
    char *destination_path; // keeps the path for destination file

    if (strcmp(argv[1], "-") != 0) // Control the path coming from the argument[1] for source file path.
    {
        source_path = (char *)malloc((strlen(argv[1]) + strlen(source_filename)) * sizeof(char)); // If the path is not "-", then malloc for absolute path.
        strcpy(source_path, argv[1]);
        strcat(source_path, source_filename);
    }
    else
    {
        source_path = (char *)malloc(strlen(source_filename) * sizeof(char));
        strcpy(source_path, source_filename);
    }

    if (strcmp(argv[2], "-") != 0) // Control the path coming from the argument[2] for destination file path.
    {
        destination_path = (char *)malloc((strlen(argv[2]) + strlen(destination_filename)) * sizeof(char));
        strcpy(destination_path, argv[2]);
        strcat(destination_path, destination_filename);
    }
    else
    {
        destination_path = (char *)malloc(strlen(destination_filename) * sizeof(char));
        strcpy(destination_path, destination_filename);
    }

    thread_number = atoi(argv[3]); // Thread number is taken from argument[3].

    if (thread_number < 1 || thread_number > 10) // Thread number cannot be less than 1 and more than 10.
    {
        printf("You have to enter thread number minimum 1 and maximum 10, please try again.\n");
        return 0;
    }

    if (FILE_SIZE < MAX_FILE_SIZE && FILE_SIZE > 0) // File size can be maximum 100 MB
    {
        source_file_creator(source_path);
    }
    else
    {
        printf("File size may be 100 MB at most\n");
        return 0;
    }

    // Welcome screen
    printf("\n       WELCOME\n");
    printf("---------------------\n");
    printf("\nThread number: %d\n", thread_number);
    printf("File size of the source file: %d\n", FILE_SIZE);

    char cwd_buffer1[300]; // For source file path in working directory. These are just for using in the User Interface, so user can find easily where the file is.
    char cwd_buffer2[300]; // For destination file path in working directory.

    if (strcmp(source_path, source_filename) == 0) // This is for displaying destination and source file path for user.
    {
        getcwd(cwd_buffer1, sizeof(cwd_buffer1));
        printf("Souce file path: %s/%s\n", cwd_buffer1, source_filename);
    }
    else
    {
        printf("Souce file path: %s\n", source_path);
    }

    if (strcmp(destination_path, destination_filename) == 0)
    {
        getcwd(cwd_buffer2, sizeof(cwd_buffer2));
        printf("Destination file path: %s/%s\n", cwd_buffer1, destination_filename);
    }
    else
    {
        printf("Destination file path: %s\n", destination_path);
    }

    // Calculating the correct buffer size for threads
    // If there is remainder, then it will be added to first thread's buffer size
    long thread_portion, first_thread_portion;
    long remainder = FILE_SIZE % thread_number;
    thread_portion = (FILE_SIZE - remainder) / thread_number;
    first_thread_portion = thread_portion + remainder;

    pthread_t threads[thread_number]; // thread array
    struct arguments_struct args[thread_number]; // struct for each thread
    sem_init(&mutex, 0, 1); // Inıtialize the mutex from 1.

    source_fd = open(source_path, O_RDONLY); // open source file

    if (source_fd == -1)
    {
        printf("Error while opening source file! The path must be end with /");
        return 0;
    }

    destination_fd = open(destination_path, O_WRONLY | O_CREAT | O_TRUNC, 0777); // open destination file

    if (destination_fd == -1)
    {
        printf("Error while opening destination file! The path must be end with /");
        return 0;
    }

    int i = 0;

    for (i = 0; i < thread_number; i++) // Assigning the variables in the struct with the correct values for each thread.
    {
        args[i].source_file = source_fd;
        args[i].destination_file = destination_fd;
        args[i].thread_no = i + 1;

        if (i == 0)
        {
            args[i].offset = 0; // First thread's offset must be 0
            args[i].buffer_size = first_thread_portion; // First thread's buffers size can be different from others
        }
        else
        {
            args[i].offset = args[i - 1].buffer_size + args[i - 1].offset; // Other thread's offset is calculatin adding the previous thread's buffer to previous thread's offset
            args[i].buffer_size = thread_portion; // Other thread's buffer size is same.
        }
    }

    // Create threads
    for (i = 0; i < thread_number; i++)
    {
        if (pthread_create(&threads[i], NULL, ascopy, (void *)&(args[i])) < 0) // Creating the threads
        {
            printf("Could not create thread\n");
        }
    }

    for (i = 0; i < thread_number; i++)
    {
        pthread_join(threads[i], NULL);
    }

    close(source_fd);
    close(destination_fd);

    return 0;
}