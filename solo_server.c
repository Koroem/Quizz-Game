#include <stdio.h>
#include <sqlite3.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>

#define PORT 2555

extern int errno;

/* threads */
typedef struct thData
{
    int idThread; // Thread id
    int cl;       // Descriptor returned by accept
} thData;
static void *treat(void *); // Function executed for each accepted client
int current_thread = 0;
pthread_mutex_t mutex; // To prvent threads from modifying current_thread at the same time
/* end */

/* database */
sqlite3 *db;   // Pointer to the database
char *err_msg; // For error message
int rc;        // For error handling database operations
/* end */

/* server */
int sd;

/* converts sockaddr_in object to a IP ADDRESS:PORT NUMBER formatted string */
char *conv_addr(struct sockaddr_in address)
{
    static char str[25];
    char port[7];
    bzero(port, 7);
    strcpy(str, inet_ntoa(address.sin_addr));      // IP address
    sprintf(port, ":%d", ntohs(address.sin_port)); // Port
    strcat(str, port);                             // IP ADDRESS: PORT NUMBER
    return (str);
}
/* end */

/* game */

void solo_game();
/* for swapping answers order */
void swap2(char *str1, char *str2)
{
    char *temp = (char *)malloc((strlen(str1) + 1) * sizeof(char));
    strcpy(temp, str1);
    strcpy(str1, str2);
    strcpy(str2, temp);
    free(temp);
}

/* generate random answers order */

void shuffle(int *array, size_t n)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int usec = tv.tv_usec;
    srand48(usec);

    if (n > 1)
    {
        size_t i;
        for (i = n - 1; i > 0; i--)
        {
            size_t j = (unsigned int)(drand48() * (i + 1));
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}
/* end */

int main()
{

    struct sockaddr_in server;
    struct sockaddr_in from;

    pthread_t th[100000]; // Array of thread identificators
    int optval = 1;

    rc = sqlite3_open("intrebari.db", &db); //  Opening intrebari database

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket.\n");
        return 0;
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("Error at bind.\n");
        return 0;
    }

    if (listen(sd, 1000) == -1)
    {
        perror("Error at listen.\n");
        return 0;
    }
    pthread_mutex_init(&mutex, NULL);
    printf("(Solo-Server) Waiting for games to start...\n\n");
    while (1)
    {
        int client;
        int length = sizeof(from);
        thData *td;

        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            printf("Error establishing connection with a client\n");
            continue;
        }
        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = current_thread++;
        td->cl = client;

        pthread_create(&th[current_thread], NULL, &treat, td);
    }
}
static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("Client %d has connected to the server. Allocating thread %d\n", tdL.cl, tdL.idThread);
    solo_game((struct thData *)arg);

    /* To prevent race condition where two threads would
       decrement the shared variable at the same time */

    pthread_mutex_lock(&mutex);
    current_thread--;
    pthread_mutex_unlock(&mutex);

    pthread_detach(pthread_self());
    close(tdL.cl);
    return (NULL);
};
void solo_game(void *arg)
{
    int nr, i, username_size;
    struct thData tdL;
    tdL = *((struct thData *)arg);

    char c;
    char username[32];

    if (read(tdL.cl, &c, sizeof(char)) <= 0)
    {
        printf("[Thread %d] Error receiving username category \n", tdL.idThread);
        return;
    }
    if (read(tdL.cl, &username_size, sizeof(int)) <= 0)
    {
        printf("[Thread %d] Error receiving username length \n", tdL.idThread);
        return;
    }
    if (read(tdL.cl, &username, username_size) <= 0)
    {
        printf("[Thread %d] Error receiving username \n", tdL.idThread);
        return;
    }

    printf("The game for user %d has started\n\n", tdL.cl);
    char user_answer;
    int score = 0;
    char category[64];
    char query[32846];
    int nr_questions = 10;

    switch (c)
    {
    case 'a':
        strcpy(category, "General Knowledge");
        break;
    case 'b':
        strcpy(category, "Books");
        break;
    case 'c':
        strcpy(category, "Film");
        break;
    case 'd':
        strcpy(category, "Music");
        break;
    case 'e':
        strcpy(category, "Television");
        break;
    case 'f':
        strcpy(category, "Video Games");
        break;
    case 'g':
        strcpy(category, "Board Games");
        break;
    case 'h':
        strcpy(category, "Science & Nature");
        break;
    case 'i':
        strcpy(category, "Science: Computers");
        break;
    case 'j':
        strcpy(category, "Science: Mathematics");
        break;
    case 'k':
        strcpy(category, "Mythology");
        break;
    case 'l':
        strcpy(category, "Sports");
        break;
    case 'm':
        strcpy(category, "Geography");
        break;
    case 'n':
        strcpy(category, "History");
        break;
    case 'o':
        strcpy(category, "Politics");
        break;
    case 'p':
        strcpy(category, "Celebrities");
        break;
    case 'q':
        strcpy(category, "Animals");
        break;
    case 'r':
        strcpy(category, "Vehicles");
        break;
    case 's':
        strcpy(category, "Comics");
        break;
    case 't':
        strcpy(category, "Japanesse Anime & Manga");
        break;
    case 'u':
        strcpy(category, "Cartoon & Animations");
        break;
    }

    sprintf(query, "SELECT *FROM intrebari where category='%s' ORDER BY RANDOM() LIMIT %d;", category, nr_questions);

    char type[9];
    int multiple_order[4] = {0, 1, 2, 3};
    int boolean_order[2] = {0, 1};
    int correct_answer_index;
    char question[512];
    char answer[512];
    char a2[512];
    char a3[512];
    char a4[512];
    char quizz_data[65536];

    sqlite3_stmt *result;
    const char *t;

    int sync;

    if (sqlite3_prepare_v2(db, query, 128, &result, &t) != SQLITE_OK)
    {
        printf("Error generating questions%s\n", sqlite3_errmsg(db));
        return;
    }
    while (sqlite3_step(result) == SQLITE_ROW)
    {

        memset(type, 0, strlen(type));
        memset(question, 0, strlen(question));
        memset(answer, 0, strlen(answer));
        memset(a2, 0, strlen(a2));
        memset(a3, 0, strlen(a3));
        memset(a4, 0, strlen(a4));

        strcpy(type, sqlite3_column_text(result, 1));
        strcpy(question, sqlite3_column_text(result, 2));

        if (strcmp(type, "boolean") == 0)
        {
            shuffle(boolean_order, 2);
            switch (boolean_order[0])
            {
            case 0:
            {
                strcpy(answer, sqlite3_column_text(result, 3));
                strcpy(a2, sqlite3_column_text(result, 4));
                break;
            }
            case 1:
            {
                strcpy(answer, sqlite3_column_text(result, 4));
                strcpy(a2, sqlite3_column_text(result, 3));
            }
            }
        }
        if (strcmp(type, "multiple") == 0)
        {
            shuffle(multiple_order, 4);
            switch (multiple_order[0])
            {
            case 0:
            {
                strcpy(answer, sqlite3_column_text(result, 3));
                break;
            }
            case 1:
            {
                strcpy(answer, sqlite3_column_text(result, 4));
                break;
            }
            case 2:
            {
                strcpy(answer, sqlite3_column_text(result, 5));
                break;
            }
            case 3:
            {
                strcpy(answer, sqlite3_column_text(result, 6));
            }
            }
            switch (multiple_order[1])
            {

            case 0:
            {
                strcpy(a2, sqlite3_column_text(result, 3));
                break;
            }
            case 1:
            {
                strcpy(a2, sqlite3_column_text(result, 4));
                break;
            }
            case 2:
            {
                strcpy(a2, sqlite3_column_text(result, 5));
                break;
            }
            case 3:
            {
                strcpy(a2, sqlite3_column_text(result, 6));
            }
            }
            switch (multiple_order[2])
            {

            case 0:
            {
                strcpy(a3, sqlite3_column_text(result, 3));
                break;
            }
            case 1:
            {
                strcpy(a3, sqlite3_column_text(result, 4));
                break;
            }
            case 2:
            {
                strcpy(a3, sqlite3_column_text(result, 5));
                break;
            }
            case 3:
            {
                strcpy(a3, sqlite3_column_text(result, 6));
            }
            }
            switch (multiple_order[3])
            {

            case 0:
            {
                strcpy(a4, sqlite3_column_text(result, 3));
                break;
            }
            case 1:
            {
                strcpy(a4, sqlite3_column_text(result, 4));
                break;
            }
            case 2:
            {
                strcpy(a4, sqlite3_column_text(result, 5));
                break;
            }
            case 3:
            {
                strcpy(a4, sqlite3_column_text(result, 6));
            }
            }
        }

        int buffer_size = (int)strlen(type);

        if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // Sending Type length
        {
            close(tdL.cl);
            printf("User %s has left game %d\n", username, tdL.idThread);
            break;
        }
        if (write(tdL.cl, &type, buffer_size) <= 0) // Sending type
        {
            close(tdL.cl);
            printf("User %s has left game %d\n", username, tdL.idThread);
            break;
        }

        buffer_size = (int)strlen(question);

        if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // Sending question length
        {
            close(tdL.cl);
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read1() de la client.\n");
            break;
        }
        if (write(tdL.cl, &question, buffer_size) <= 0) // Sending question
        {
            close(tdL.cl);
            printf("User %s has left game %d\n", username, tdL.idThread);
            break;
        }
        if (strcmp(type, "boolean") == 0)
        {

            buffer_size = (int)strlen(answer);

            if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // Sending a1 length
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            if (write(tdL.cl, answer, buffer_size) <= 0) // Sending a1
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            buffer_size = (int)strlen(a2);

            if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // Sending a2 length
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            if (write(tdL.cl, &a2, buffer_size) <= 0) // Sending a2
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }
        }
        else
        {

            buffer_size = (int)strlen(answer);

            if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // Sending a1 length
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            if (write(tdL.cl, &answer, buffer_size) <= 0) // Sending a1
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            buffer_size = (int)strlen(a2);

            if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // Sending a2 length
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            if (write(tdL.cl, &a2, buffer_size) <= 0) // Sending a2
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            buffer_size = (int)strlen(a3);

            if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // Sending a3 length
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            if (write(tdL.cl, &a3, buffer_size) <= 0) // Sending a3
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            buffer_size = (int)strlen(a4);

            if (write(tdL.cl, &buffer_size, sizeof(int)) <= 0) // // Sending a4 length
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }

            if (write(tdL.cl, &a4, buffer_size) <= 0) // Sending a4
            {
                close(tdL.cl);
                printf("User %s has left game %d\n", username, tdL.idThread);
                break;
            }
        }
        sleep(15);
        if (read(tdL.cl, &user_answer, sizeof(char)) <= 0) // Receiving user answer
        {
            printf("User %s has left game %d\n", username, tdL.idThread);
            break;
        }

        if (strcmp(type, "boolean") == 0)
            switch (user_answer)
            {
            case 'a':
            {
                if (boolean_order[0] == 0)
                    score++;
                break;
            }
            case 'b':
            {
                if (boolean_order[1] == 0)
                    score++;
                break;
            }
            }
        if (strcmp(type, "multiple") == 0)
            switch (user_answer)
            {
            case 'a':
            {
                if (multiple_order[0] == 0)
                    score++;
                break;
            }
            case 'b':
            {
                if (multiple_order[1] == 0)
                    score++;
                break;
            }
            case 'c':
            {
                if (multiple_order[2] == 0)
                    score++;
                break;
            }
            case 'd':
            {
                if (multiple_order[3] == 0)
                    score++;
                break;
            }
            }
    }
    if (write(tdL.cl, &score, sizeof(int)) <= 0) // Sending the score obtained
    {
        printf("User %s has left game %d\n", username, tdL.idThread);
    }
    close(tdL.cl);
    sqlite3_close(db);
    printf("The game %d has ended\n", tdL.idThread);

    /* We connect to the main server to add the game to history because the database is already opened in the main
server and we can`t perform write operations here*/

    int sport = 2116;
    int dd;
    char d = '+';
    struct sockaddr_in server_management;

    if ((dd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error establishing connection to the main server.\n");
        return;
    }

    server_management.sin_family = AF_INET;
    server_management.sin_addr.s_addr = inet_addr("192.168.174.128");
    server_management.sin_port = htons(sport);

    char game_type[5] = "Solo";

    int size;

    if (connect(dd, (struct sockaddr *)&server_management, sizeof(struct sockaddr)) == -1)
    {
        perror("Error connecting to the main server\n");
        sleep(1);
        return;
    }
    if (write(dd, &d, sizeof(char)) <= 0) // Sending command update history to the server
    {
        perror("Error sending command update history.\n");
        return;
    }

    size = strlen(game_type);

    if (write(dd, &size, sizeof(int)) <= 0) // Sending type of game length to the main server
    {
        perror("Error sending game type length \n");
        return;
    }

    if (write(dd, &game_type, size) <= 0) // Sending type of game to the main server
    {
        perror("Error sending game type\n");
        return;
    }

    size = strlen(username);

    if (write(dd, &size, sizeof(int)) <= 0) // Sending the length of the username to the main server
    {
        perror("[client]Eroare la write() spre server_management.\n");
        return;
    }
    if (write(dd, &username, size) <= 0) // Sending the username to the main server
    {
        perror("[client]Eroare la write() spre server_management.\n");
        return;
    }
    if (write(dd, &score, sizeof(int)) <= 0) // Sending the score to the main server
    {
        perror("[client]Eroare la write() spre server_management.\n");
        return;
    }
    /* To prevent closing the socket before all data has been read by the main server */
    int data_received_confirmation;
    if (read(dd, &data_received_confirmation, sizeof(int)) <= 0)
    {
        printf("Error receiving confirmation from the main server\n");
        close(dd);
        return;
    }
    printf("The game %d has been added to history\n\n", tdL.idThread);
    close(dd);
}