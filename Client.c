#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h>
#include <ctype.h>
#include <arpa/inet.h>

#define port 2116
#define solo_game_port 2555
#define duo_game_port 2666
#define ffa_game_port 2777

void menu();
void login();
void logout();
void howtoplay();
void createAccount();
void game();
void history();
void tutorial();
void duo();
void ffa();
void loggedMenu();
void leave();
void singleplayer();
void gametypes();
void questiontypes();
void questioncategories();

char logged_user[32];      // The username of the logged user
int sd;                    // Socket descriptor
struct sockaddr_in server; // Internet socket address structure

void ffa()
{
    system("clear");      // Clear screen
    tcflush(0, TCIFLUSH); // Empty STDIN buffer

    int gd, c, username_length = strlen(logged_user);

    struct sockaddr_in game_server;

    if ((gd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error establishing connection to the game server.\n");
        sleep(1);
        loggedMenu("%%");
    }

    game_server.sin_family = AF_INET;
    game_server.sin_addr.s_addr = inet_addr("192.168.174.128");
    game_server.sin_port = htons(ffa_game_port);

    char decision;
    char options[404];

    sprintf(options,
            "A for General Knowledge\nB for Books\nC for Film\nD for Music\nE for Television\nF for Video Games\nG for Board Games\nH for Science & Nature\nI for Science: Computers\nJ for Science: Mathematics\n"
            "K for Mythology\nL for Sports\nM for Geography\nN for History\nO for Politics\nP for Celebrities\nQ for Animals\nR for Vehicles\nS for Comics\nT for Japanesse Anime & Manga\nU for Cartoon & Animations\n\n"
            "V to go back\nX to quit\n");

    const char *correct = "abcdefghijklmnopqrstuvxABCDEFGHIJKLMNOPQRSTUVX";

    printf("%s\n", options);
    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        decision = tolower(decision);

    if (strchr(correct, (int)decision) == NULL) // Check if the command is not accepted
    {
        printf("\nUnrecognized\n");
        sleep(1);
        singleplayer(0);
    }
    switch (decision)
    {
    case 'v':
        close(gd);
        loggedMenu("%%");
        break;
    case 'x':
        close(gd);
        leave();
        exit(0);
        break;
    }
    if (connect(gd, (struct sockaddr *)&game_server, sizeof(struct sockaddr)) == -1) // connecting to the Ffa server
    {
        printf("Server is currently under maintenance. Try again later.\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &decision, sizeof(char)) <= 0) // sending the category of questions to the server
    {
        perror("Error sending category\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &username_length, sizeof(int)) <= 0) // sending the username's length to the server
    {
        perror("Error sending username's length\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &logged_user, username_length) <= 0) // sending the username to the server
    {
        perror("Error sending username\n");
        sleep(1);
        loggedMenu("%%");
    }
    system("clear");

    fd_set input_set;       // File descriptors set
    struct timeval timeout; // To set the timeout duration

    FD_ZERO(&input_set);              // Macro to clear the file descriptors set
    FD_SET(STDIN_FILENO, &input_set); // Adding STDIN to input_set

    /* Setting the timeout time for select*/

    timeout.tv_sec = 1;  // 1 second
    timeout.tv_usec = 0; // 0 milliseconds

    char type[9] = {'\0'};        // Type of question
    char question[512] = {'\0'};  // Question
    char answer1[512] = {'\0'};   // Answer1
    char answer2[512] = {'\0'};   // Answer2
    char answer3[512] = {'\0'};   // Answer3
    char answer4[512] = {'\0'};   // Answer4
    char buffer_size[8] = {'\0'}; // Used to store the length of type,question and answers
    char a[2] = {'\0'};           // Answer of the user
    char l = 'x';                 // Sent to the server if the user has not provided a valid answer

    int num, count, read_bytes, ready_for_reading, final_score = 0, start = 0;

    printf("Waiting for other users to join\n");

    while (start != 1) // Game starts when the client receives the ping 1 from the server
    {
        if (read(gd, &start, sizeof(int)) <= 0) // Receive ping from the server
        {
            printf("Error in queue\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (start == 1) // When the game starts we exit the loop without writing to the socket
            break;
        if (write(gd, &start, sizeof(int)) <= 0) // Send ping back to the server
        {
            printf("Error in queue\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
    }
    /* The game has started */
    for (int que = 1; que <= 10; que++) // For each question
    {

        system("clear");      // Clear screen
        tcflush(0, TCIFLUSH); // Clear STDIN buffer

        count = 15;            // Used to store the time left to submit an answer
        read_bytes = 0;        // Used to check if the user provided an answer
        ready_for_reading = 0; // Used for timeout read with select

        memset(a, 0, strlen(a));
        memset(type, 0, strlen(type));
        memset(question, 0, strlen(question));
        memset(answer1, 0, strlen(answer1));
        memset(answer2, 0, strlen(answer2));
        memset(answer3, 0, strlen(answer3));
        memset(answer4, 0, strlen(answer4));

        if (read(gd, &num, sizeof(int)) <= 0) // Receiving the length of type
        {
            perror("Error reading type length.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &type, num) <= 0) // Receiving the type of the question
        {
            perror("Error reading question type\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &num, sizeof(int)) <= 0) // Receiving the question`s length
        {
            perror("Error reading question length.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &question, num) <= 0) // Receiving question
        {
            perror("Error reading question.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (strcmp(type, "boolean") == 0) // Boolean type question => only 2 answers
        {
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer1`s length
            {
                perror("Error reading answer1 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }

            if (read(gd, &answer1, num) <= 0) // Receiving answer1
            {
                perror("Error reading answer1 .\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer2`s length
            {
                perror("Error reading answer2 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer2, num) <= 0) // Receiving answer2
            {
                perror("Error reading answer2.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            system("clear"); // Clear screen
            printf("Question %d: %s\n", que, question);
            printf("----------------------------------------------------------------------------------------------------------------------\n");
            printf("a: %s\n", answer1);
            printf("b: %s\n", answer2);

            printf("Please provide an answer\n\n");
        }
        else // Multiple type question => 4 possible answers
        {
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer1`s length
            {
                perror("Error reading answer1 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }

            if (read(gd, &answer1, num) <= 0) // Receiving answer1
            {
                perror("Error reading answer1.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer2`s length
            {
                perror("Error reading answer2 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer2, num) <= 0) // Receiving answer2
            {
                perror("Error reading answer2.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer3`s length
            {
                perror("Error reading answer3 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer3, num) <= 0) // Receiving answer3
            {
                perror("Error reading answer3.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer4`s length
            {
                perror("Error reading answer4 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer4, num) <= 0) // Receiving answer4
            {
                perror("Error reading answer4.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }

            system("clear"); // Clear screen
            printf("Question %d: %s\n", que, question);
            printf("----------------------------------------------------------------------------------------------------------------------\n");
            printf("a: %s\n", answer1);
            printf("b: %s\n", answer2);
            printf("c: %s\n", answer3);
            printf("d: %s\n\n", answer4);

            printf("Please provide an answer\n\n");
        }
        /* While answer is not accepted or the avaliable time has passed */

        while (((a[0] != 'a' && a[0] != 'b' && strcmp(type, "boolean") == 0) || (a[0] != 'a' && a[0] != 'b' && a[0] != 'c' && a[0] != 'd' && strcmp(type, "multiple") == 0)) && count > 0)
        {
            /* Update the timeout interval for each iteration */

            timeout.tv_sec = 1;  // Seconds for each read attempt
            timeout.tv_usec = 0; // Milliseconds for each read attempt

            FD_ZERO(&input_set);              // Macro to clear the file descriptors set
            FD_SET(STDIN_FILENO, &input_set); // Adding STDIN to input_set

            fflush(stdout); // Empy STDOUT buffer

            count == 1 ? printf("(%d second left)", count) : printf("(%d seconds left)", count);
            printf("\n");

            ready_for_reading = select(1, &input_set, NULL, NULL, &timeout); // 1 sec timeout read

            if (ready_for_reading == -1)
            {
                printf("Error reading user input\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }

            if (ready_for_reading) // 1 second has passed and we check if the user wrote something in the STDIN buffer
            {
                read_bytes = read(0, a, sizeof(a));
                while (strlen(a) > 0 && a[strlen(a) - 1] == '\n') // Removes \n from a
                    a[strlen(a) - 1] = '\0';
                a[0] = tolower(a[0]);
                if (read_bytes != 0) // We check if the user provided an answer
                {
                    tcflush(0, TCIFLUSH);

                    if ((a[0] != 'a' && a[0] != 'b' && (strcmp(type, "boolean") == 0)) && a[0] != '\n') // Answer not accepted
                        printf("\nAccepted answers are: a,b \n");
                    else if ((a[0] != 'a' && a[0] != 'b' && a[0] != 'c' && a[0] != 'd' && (strcmp(type, "multiple") == 0)) && a[0] != '\n') // Answer not accepted
                        printf("\nAccepted answers are: a,b,c,d \n");
                }
            }
            count--;
            /* Answer accepted */
            if (((a[0] == 'a' || a[0] == 'b') && strcmp(type, "boolean") == 0) || ((a[0] == 'a' || a[0] == 'b' || a[0] == 'c' || a[0] == 'd') && strcmp(type, "multiple") == 0))
            {
                printf("Answer submited: %c\n\n", a[0]);
                break;
            }
        }
        while (count > 0) // If there is any time left after the user provided the answer
        {
            count == 1 ? printf("(%d second left)\n", count) : printf("(%d seconds left)\n", count);
            sleep(1);
            count--;
        }

        if (strcmp(type, "boolean") == 0) // Boolean type question
        {

            if (a[0] == 'a' || a[0] == 'b')
            {

                if (write(gd, &a[0], sizeof(char)) <= 0) // User has submited an accepted answer
                {
                    perror("Error sending answer.\n");
                    close(gd);
                    sleep(1);
                    loggedMenu("%%");
                }
            }
            else if (write(gd, &l, sizeof(char)) <= 0) // User didn`t submit an accepted answer
            {
                perror("Error sending null answer.\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }
        }
        else if (strcmp(type, "multiple") == 0) // Multiple type question
        {

            if (a[0] == 'a' || a[0] == 'b' || a[0] == 'c' || a[0] == 'd')
            {

                if (write(gd, &a[0], sizeof(char)) <= 0) // User has submited an accepted answer
                {
                    perror("Error sending answer.\n");
                    close(gd);
                    sleep(1);
                    loggedMenu("%%");
                }
            }
            else

                if (write(gd, &l, sizeof(char)) <= 0) // User didn`t submit an accepted answer
            {
                perror("Error sending null answer.\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }
        }
    }
    /* game has finished */

    int nr_of_winners; // Used to store the number of winners (users that obtained the maximum score for that game)
    char winner[32];   // Used to store the username of the winners

    if (read(gd, &final_score, sizeof(int)) <= 0) // Receiving the score obtained
    {
        perror("Error reading score.\n");
        sleep(1);
        close(gd);
        loggedMenu("%%");
    }
    printf("\nCongratulations %s! You got %d questions right\n\n", logged_user, final_score);

    if (read(gd, &nr_of_winners, sizeof(int)) <= 0) // Receiving the number of winners
    {
        perror("Error reading number of winners.\n");
        sleep(1);
        close(gd);
        loggedMenu("%%");
    }
    printf("Winners:\n\n");
    int user_length;
    for (int contor = 0; contor < nr_of_winners; contor++) // For each winner
    {

        memset(winner, 0, strlen(winner));
        user_length = 0;
        if (read(gd, &final_score, sizeof(int)) <= 0) // Receiving the winner`s score
        {
            perror("Error reading winner score.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (read(gd, &user_length, sizeof(int)) <= 0) // Receiving the winner`s username length
        {
            perror("Error reading winner`s length \n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (read(gd, &winner, user_length) <= 0) // Receiving the winner`s username
        {
            perror("Error reading winner score.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        printf("%s : %d points\n", winner, final_score);
    }

    sleep(10);
    close(gd);
    loggedMenu("%%");
}
void singleplayer(int flag)
{

    system("clear"); // Clear screen

    tcflush(0, TCIFLUSH); // Clear STDIN buffer

    int gd, c, username_size;
    struct sockaddr_in game_server;

    char decision;
    char options[404];
    sprintf(options,
            "A for General Knowledge\nB for Books\nC for Film\nD for Music\nE for Television\nF for Video Games\nG for Board Games\nH for Science & Nature\nI for Science: Computers\nj for Science: Mathematics\n"
            "K for Mythology\nL for Sports\nM for Geography\nN for History\nO for Politics\nP for Celebrities\nQ for Animals\nR for Vehicles\nS for Comics\nT for Japanesse Anime & Manga\nU for Cartoon & Animations\n\n"
            "V to go back\nX to quit\n");
    const char *correct = "abcdefghijklmnopqrstuvxABCDEFGHIJKLMNOPQRSTUVX";
    printf("%s\n", options);
    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF)
        decision = tolower(decision);
    if (strchr(correct, (int)decision) == NULL)
    {
        printf("\nUnrecognized\n");
        sleep(1);
        singleplayer(0);
    }
    switch (decision)
    {
    case 'v':
        close(gd);
        loggedMenu("%%");
        break;
    case 'x':
        close(gd);
        leave();
        exit(0);
        break;
    }
    if ((gd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error connecting to the game server.\n");
        sleep(1);
        loggedMenu("%%");
    }
    game_server.sin_family = AF_INET;
    game_server.sin_addr.s_addr = inet_addr("192.168.174.128");
    game_server.sin_port = htons(solo_game_port);

    username_size = strlen(logged_user);
    if (connect(gd, (struct sockaddr *)&game_server, sizeof(struct sockaddr)) == -1)
    {
        perror("Failed to connect to gameserver solo.\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &decision, sizeof(decision)) <= 0) // Sending the category
    {
        perror("Error starting game\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &username_size, sizeof(int)) <= 0) // Sending the length of the username
    {
        perror("Error starting game\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &logged_user, username_size) <= 0) // Sending the username
    {
        perror("Error starting game\n");
        sleep(1);
        loggedMenu("%%");
    }

    system("clear"); // Clear screen

    fd_set input_set;       // File descriptors set
    struct timeval timeout; // To set the timeout duration

    FD_ZERO(&input_set);              // Macro to clear the file descriptors set
    FD_SET(STDIN_FILENO, &input_set); // Adding STDIN to input_set

    /* Setting the timeout time for select*/
    timeout.tv_sec = 1;  // 1 second
    timeout.tv_usec = 0; // 0 milliseconds

    char type[9] = {'\0'};        // Type of question
    char question[512] = {'\0'};  // Question
    char answer1[512] = {'\0'};   // Answer1
    char answer2[512] = {'\0'};   // Answer2
    char answer3[512] = {'\0'};   // Answer3
    char answer4[512] = {'\0'};   // Answer4
    char buffer_size[8] = {'\0'}; // Used to store the length of type,question and answers
    char a[2] = {'\0'};           // Answer of the user
    char l = 'x';                 // Sent to the server if the user has not provided a valid answer

    int num, count, read_bytes, ready_for_reading, final_score = 0;

    /* The game has started */

    for (int que = 1; que <= 10; que++) // // For each question
    {

        system("clear");      // Clear screen
        tcflush(0, TCIFLUSH); // Clear STDIN buffer

        count = 15;            // Used to store the time left to submit an answer
        read_bytes = 0;        // Used to check if the user provided an answer
        ready_for_reading = 0; // Used for timeout read with select

        memset(a, 0, strlen(a));
        memset(type, 0, strlen(type));
        memset(question, 0, strlen(question));
        memset(answer1, 0, strlen(answer1));
        memset(answer2, 0, strlen(answer2));
        memset(answer3, 0, strlen(answer3));
        memset(answer4, 0, strlen(answer4));

        if (read(gd, &num, sizeof(int)) <= 0) // Receiving the length of type
        {
            perror("Error reading type length.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &type, num) <= 0) // // Receiving the type of the question
        {
            perror("Error reading question type\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &num, sizeof(int)) <= 0) // Receiving the question`s length
        {
            perror("Error reading question length.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &question, num) <= 0) // Receiving question
        {
            perror("Error reading question.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (strcmp(type, "boolean") == 0) // Boolean type question => only 2 answers
        {
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer1`s length
            {
                perror("Error reading answer1 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }

            if (read(gd, &answer1, num) <= 0) // Receiving answer1
            {
                perror("Error reading answer1 .\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer2`s length
            {
                perror("Error reading answer2 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer2, num) <= 0) // Receiving answer2
            {
                perror("Error reading answer2.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            system("clear"); // clear screen
            printf("Question %d: %s\n", que, question);
            printf("----------------------------------------------------------------------------------------------------------------------\n");
            printf("a: %s\n", answer1);
            printf("b: %s\n", answer2);
            printf("Please provide an answer\n\n");
        }
        else // Multiple type question => 4 possible answers
        {
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer1`s length
            {
                perror("Error reading answer1 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }

            if (read(gd, &answer1, num) <= 0) // Receiving answer1
            {
                perror("Error reading answer1.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer2`s length
            {
                perror("Error reading answer2 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer2, num) <= 0) // Receiving answer2
            {
                perror("Error reading answer2.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer3`s length
            {
                perror("Error reading answer3 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer3, num) <= 0) // Receiving answer3
            {
                perror("Error reading answer3.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer4`s length
            {
                perror("Error reading answer4 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer4, num) <= 0) // Receiving answer4
            {
                perror("Error reading answer4.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            system("clear"); // Clear screen
            printf("Question %d: %s\n", que, question);
            printf("----------------------------------------------------------------------------------------------------------------------\n");
            printf("a: %s\n", answer1);
            printf("b: %s\n", answer2);
            printf("c: %s\n", answer3);
            printf("d: %s\n\n", answer4);
        }
        printf("Please provide an answer\n\n");
        /* While answer is not accepted or the avaliable time has passed */
        while (((a[0] != 'a' && a[0] != 'b' && strcmp(type, "boolean") == 0) || (a[0] != 'a' && a[0] != 'b' && a[0] != 'c' && a[0] != 'd' && strcmp(type, "multiple") == 0)) && count > 0)
        {
            /* Update the timeout interval for each iteration */

            timeout.tv_sec = 1;  // Seconds for each read attempt
            timeout.tv_usec = 0; // Milliseconds for each read attempt

            FD_ZERO(&input_set);              // Macro to clear the file descriptors set
            FD_SET(STDIN_FILENO, &input_set); // Adding STDIN to input_set

            fflush(stdout); // Empy STDOUT buffer

            count == 1 ? printf("(%d second left)\n", count) : printf("(%d seconds left)\n", count);

            ready_for_reading = select(1, &input_set, NULL, NULL, &timeout); // 1 sec timeout read

            if (ready_for_reading == -1)
            {
                printf("Error reading user input\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }

            if (ready_for_reading) // 1 second has passed and we check if the user wrote something in the STDIN buffer
            {
                read_bytes = read(0, a, sizeof(a));
                while (strlen(a) > 0 && a[strlen(a) - 1] == '\n') // Removes \n from a
                    a[strlen(a) - 1] = '\0';
                if (read_bytes != 0) // We check if the user provided an answer
                {
                    tcflush(0, TCIFLUSH);

                    if ((a[0] != 'a' && a[0] != 'b' && (strcmp(type, "boolean") == 0)) && a[0] != '\n') // Answer not accepted
                        printf("\nAccepted answers are: a,b \n");
                    else if ((a[0] != 'a' && a[0] != 'b' && a[0] != 'c' && a[0] != 'd' && (strcmp(type, "multiple") == 0)) && a[0] != '\n') // Answer not accepted
                        printf("\nAccepted answers are: a,b,c,d \n");
                }
            }
            count--;
            /* Answer accepted */
            if (((a[0] == 'a' || a[0] == 'b') && strcmp(type, "boolean") == 0) || ((a[0] == 'a' || a[0] == 'b' || a[0] == 'c' || a[0] == 'd') && strcmp(type, "multiple") == 0))
            {
                printf("\nAnswer submited: %c\n\n", a[0]);
                break;
            }
        }
        while (count > 0) // If there is any time left after the user provided the answer
        {
            count == 1 ? printf("(%d second left)\n", count) : printf("(%d seconds left)\n", count);
            sleep(1);
            count--;
        }

        if (strcmp(type, "boolean") == 0) // Boolean type question
        {
            if (a[0] == 'a' || a[0] == 'b')
            {
                if (write(gd, &a[0], sizeof(char)) <= 0) // User has submited an accepted answer
                {
                    perror("Error sending answer.\n");
                    close(gd);
                    sleep(1);
                    loggedMenu("%%");
                }
            }
            else if (write(gd, &l, sizeof(char)) <= 0) // User didn`t submit an accepted answer
            {
                perror("Error sending null answer.\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }
        }
        else if (strcmp(type, "multiple") == 0) // Multiple type question
        {
            if (a[0] == 'a' || a[0] == 'b' || a[0] == 'c' || a[0] == 'd')
            {
                if (write(gd, &a[0], sizeof(char)) <= 0) // User has submited an accepted answer
                {
                    perror("Error sending answer.\n");
                    close(gd);
                    sleep(1);
                    loggedMenu("%%");
                }
            }
            else if (write(gd, &l, sizeof(char)) <= 0) // User didn`t submit an accepted answer
            {
                perror("Error sending null answer.\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }
        }
    }

    /* game has finished */

    if (read(gd, &final_score, sizeof(int)) <= 0) // Receiving the score obtained
    {
        perror("Error reading score.\n");
        close(gd);
        sleep(1);
        loggedMenu("%%");
    }
    printf("\nCongratulations %s! You got %d questions right\n", logged_user, final_score);

    close(gd);
    sleep(10);
    loggedMenu("%%");
}
void duo()
{
    system("clear");      // Clear screen
    tcflush(0, TCIFLUSH); // Empty STDIN buffer

    int gd, c, username_length = strlen(logged_user);

    struct sockaddr_in game_server;

    if ((gd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error establishing connection the duo server.\n");
        sleep(1);
        loggedMenu("%%");
    }
    game_server.sin_family = AF_INET;
    game_server.sin_addr.s_addr = inet_addr("192.168.174.128");
    game_server.sin_port = htons(duo_game_port);

    char decision;
    char options[404];
    sprintf(options,
            "A for General Knowledge\nB for Books\nC for Film\nD for Music\nE for Television\nF for Video Games\nG for Board Games\nH for Science & Nature\nI for Science: Computers\nJ for Science: Mathematics\n"
            "K for Mythology\nL for Sports\nM for Geography\nN for History\nO for Politics\nP for Celebrities\nQ for Animals\nR for Vehicles\nS for Comics\nT for Japanesse Anime & Manga\nU for Cartoon & Animations\n\n"
            "V to go back\nX to quit\n");

    const char *correct = "abcdefghijklmnopqrstuvxABCDEFGHIJKLMNOPQRSTUVX";

    printf("%s\n", options);
    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        decision = tolower(decision);

    if (strchr(correct, (int)decision) == NULL) // Check if the command is not accepted
    {
        printf("\nUnrecognized\n");
        sleep(1);
        singleplayer(0);
    }
    switch (decision)
    {
    case 'v':
        close(gd);
        loggedMenu("%%");
        break;
    case 'x':
        close(gd);
        leave();
        exit(0);
        break;
    }
    if (connect(gd, (struct sockaddr *)&game_server, sizeof(struct sockaddr)) == -1) // connecting to the Duo server
    {
        printf("Server is currently under maintenance. Try again later.\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &decision, sizeof(decision)) <= 0) // Sending the category picked by the user to the server
    {
        perror("Error sending category\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &username_length, sizeof(int)) <= 0) // Sending the username's length to the server
    {
        perror("Error sending username's length\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (write(gd, &logged_user, username_length) <= 0) // Sending the username to the server
    {
        perror("Error sending username\n");
        sleep(1);
        loggedMenu("%%");
    }
    system("clear");

    fd_set input_set;       // File descriptors set
    struct timeval timeout; // To set the timeout duration

    FD_ZERO(&input_set);              // Macro to clear the file descriptors set
    FD_SET(STDIN_FILENO, &input_set); // Adding STDIN to input_set

    /* Setting the timeout time for select */

    char type[9] = {'\0'};        // Type of question
    char question[512] = {'\0'};  // Question
    char answer1[512] = {'\0'};   // Answer1
    char answer2[512] = {'\0'};   // Answer2
    char answer3[512] = {'\0'};   // Answer3
    char answer4[512] = {'\0'};   // Answer4
    char buffer_size[8] = {'\0'}; // Used to store the length of type,question and answers
    char a[2] = {'\0'};           // Answer of the user
    char l = 'x';                 // Sent to the server if the user has not provided a valid answer;

    int num, count, read_bytes, ready_for_reading, final_score = 0, start = 0;

    printf("Waiting for other users to join\n");

    while (start != 1) // Game starts when the client receives the ping 1 from the server
    {
        if (read(gd, &start, sizeof(int)) <= 0) // Receive ping from the server
        {
            printf("Error in queue\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (start == 1)
            break;
        if (write(gd, &start, sizeof(int)) <= 0) // Send ping back to the server
        {
            printf("Error in queue\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
    }
    /* The game has started */
    for (int que = 1; que <= 10; que++) // For each question
    {

        system("clear");      // Clear screen
        tcflush(0, TCIFLUSH); // Clear STDIN buffer

        count = 15;            // Used to store the time left to submit an answer
        read_bytes = 0;        // Used to check if the user provided an answer
        ready_for_reading = 0; // Used for timeout read with select

        memset(a, 0, strlen(a));
        memset(type, 0, strlen(type));
        memset(question, 0, strlen(question));
        memset(answer1, 0, strlen(answer1));
        memset(answer2, 0, strlen(answer2));
        memset(answer3, 0, strlen(answer3));
        memset(answer4, 0, strlen(answer4));

        if (read(gd, &num, sizeof(int)) <= 0) // Receiving the length of type
        {
            perror("Error reading type length.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &type, num) <= 0) // Receiving the type of the question
        {
            perror("Error reading question type\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &num, sizeof(int)) <= 0) // Receiving question's length
        {
            perror("Error reading question length.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }

        if (read(gd, &question, num) <= 0) // Receiving question
        {
            perror("Error reading question.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (strcmp(type, "boolean") == 0) // Boolean type question => only 2 answers
        {
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer1's length
            {
                perror("Error reading answer1 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }

            if (read(gd, &answer1, num) <= 0) // Receiving answer1
            {
                perror("Error reading answer1 .\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer2's length
            {
                perror("Error reading answer2 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer2, num) <= 0) // Receiving answer2
            {
                perror("Error reading answer2.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            system("clear");
            printf("Question %d: %s\n", que, question);
            printf("----------------------------------------------------------------------------------------------------------------------\n");
            printf("a: %s\n", answer1);
            printf("b: %s\n", answer2);
            printf("Please provide an answer\n\n");
        }
        else // Multiple type question => 4 possible answers
        {
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer1's length
            {
                perror("Error reading answer1 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }

            if (read(gd, &answer1, num) <= 0) // Receiving answer1
            {
                perror("Error reading answer1.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer2's length
            {
                perror("Error reading answer2 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer2, num) <= 0) // Receiving answer2
            {
                perror("Error reading answer2.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer3's length
            {
                perror("Error reading answer3 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer3, num) <= 0) // Receiving answer3
            {
                perror("Error reading answer3.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &num, sizeof(int)) <= 0) // Receiving answer4's length
            {
                perror("Error reading answer4 length.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            if (read(gd, &answer4, num) <= 0) // Receiving answer4
            {
                perror("Error reading answer4.\n");
                sleep(1);
                close(gd);
                loggedMenu("%%");
            }
            system("clear"); // Clear screen
            printf("Question %d: %s\n", que, question);
            printf("----------------------------------------------------------------------------------------------------------------------\n");
            printf("a: %s\n", answer1);
            printf("b: %s\n", answer2);
            printf("c: %s\n", answer3);
            printf("d: %s\n\n", answer4);
        }
        printf("Please provide an answer\n\n");

        /* While answer is not accepted or the avaliable time has passed */

        while (((a[0] != 'a' && a[0] != 'b' && strcmp(type, "boolean") == 0) || (a[0] != 'a' && a[0] != 'b' && a[0] != 'c' && a[0] != 'd' && strcmp(type, "multiple") == 0)) && count > 0) // invalid answer
        {
            /* Update the timeout interval for each iteration */

            timeout.tv_sec = 1;  // Seconds for each read attempt
            timeout.tv_usec = 0; // Milliseconds for each read attempt

            FD_ZERO(&input_set);              // Macro to clear the file descriptors set
            FD_SET(STDIN_FILENO, &input_set); // Adding STDIN to input_set

            fflush(stdout); // Empty STDOUT buffer

            count == 1 ? printf("(%d second left)", count) : printf("(%d seconds left)", count);
            printf("\n");

            ready_for_reading = select(1, &input_set, NULL, NULL, &timeout); // 1 sec timeout read
            if (ready_for_reading == -1)
            {
                printf("Error reading user input\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }

            if (ready_for_reading) // // 1 second has passed and we check if the user wrote something in the STDIN buffer
            {
                read_bytes = read(0, a, sizeof(a));
                while (strlen(a) > 0 && a[strlen(a) - 1] == '\n') // Removes \n from a
                    a[strlen(a) - 1] = '\0';
                a[0] = tolower(a[0]);
                if (read_bytes != 0) // We check if the user provided an answer
                {
                    tcflush(0, TCIFLUSH); // empty STDIN buffer

                    if ((a[0] != 'a' && a[0] != 'b' && (strcmp(type, "boolean") == 0)) && a[0] != '\n') // Answer not accepted
                        printf("\nAccepted answers are: a,b \n");
                    else if ((a[0] != 'a' && a[0] != 'b' && a[0] != 'c' && a[0] != 'd' && (strcmp(type, "multiple") == 0)) && a[0] != '\n') // Answer not accepted
                        printf("\nAccepted answers are: a,b,c,d \n");
                }
            }
            count--;
            /* Answer accepted */
            if (((a[0] == 'a' || a[0] == 'b') && strcmp(type, "boolean") == 0) || ((a[0] == 'a' || a[0] == 'b' || a[0] == 'c' || a[0] == 'd') && strcmp(type, "multiple") == 0)) // check for accepted answer
            {
                printf("\nAnswer submited: %c\n\n", a[0]);
                break;
            }
        }
        while (count > 0) // If there is any time left after the user provided the answer
        {
            count == 1 ? printf("(%d second left)\n", count) : printf("(%d seconds left)\n", count);
            sleep(1);
            count--;
        }

        if (strcmp(type, "boolean") == 0) // Boolean type question
        {
            if (a[0] == 'a' || a[0] == 'b') // accepted answers for type boolean
            {
                if (write(gd, &a[0], sizeof(char)) <= 0) // User has submited an accepted answer
                {
                    perror("Error sending answer.\n");
                    close(gd);
                    sleep(1);
                    loggedMenu("%%");
                }
            }
            else if (write(gd, &l, sizeof(char)) <= 0) // User didn`t submit an accepted answer
            {
                perror("Error sending null answer.\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }
        }
        else if (strcmp(type, "multiple") == 0) // Multiple type question
        {
            if (a[0] == 'a' || a[0] == 'b' || a[0] == 'c' || a[0] == 'd')
            {
                if (write(gd, &a[0], sizeof(char)) <= 0) // User has submited an accepted answer
                {
                    perror("Error sending answer.\n");
                    close(gd);
                    sleep(1);
                    loggedMenu("%%");
                }
            }
            else if (write(gd, &l, sizeof(char)) <= 0) // User didn`t submit an accepted answer
            {
                perror("Error sending null answer.\n");
                close(gd);
                sleep(1);
                loggedMenu("%%");
            }
        }
    }
    /* game has finished */
    int nr_of_winners; // Used to store the number of winners (users that obtained the maximum score for that game)
    char winner[32];   // Used to store the username of the winners

    if (read(gd, &final_score, sizeof(int)) <= 0) // Receiving the score obtained
    {
        perror("Error reading score.\n");
        sleep(1);
        close(gd);
        loggedMenu("%%");
    }
    printf("\nCongratulations %s! You got %d questions right\n\n", logged_user, final_score);

    if (read(gd, &nr_of_winners, sizeof(int)) <= 0) // Receiving the number of winners
    {
        perror("Error reading number of winners.\n");
        sleep(1);
        close(gd);
        loggedMenu("%%");
    }
    printf("Winners:\n\n");
    int user_length, user_score;

    for (int contor = 0; contor < nr_of_winners; contor++) // For each winner
    {
        memset(winner, 0, strlen(winner));
        user_length = 0;
        if (read(gd, &user_score, sizeof(int)) <= 0) // Receiving the winner`s score
        {
            perror("Error reading winner score.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (read(gd, &user_length, sizeof(int)) <= 0) // Receiving the winner`s username length
        {
            perror("Error reading winner`s length \n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        if (read(gd, &winner, user_length) <= 0) // Receiving the winner`s username
        {
            perror("Error reading winner score.\n");
            sleep(1);
            close(gd);
            loggedMenu("%%");
        }
        printf("%s : %d points\n", winner, final_score);
    }

    sleep(10);
    close(gd);
    loggedMenu("%%");
}
void game()
{
    system("clear");      // Clear screen
    tcflush(0, TCIFLUSH); // Empty STDIN buffer
    int c;
    char decision = {'\0'};
    char *options = "S for Single player\nD for 1v1\nF for FFA\nB to go back\nQ to Quit\n";
    printf("%s\n", options);

    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        decision = tolower(decision);
    switch (decision)
    {
    case 's':
        singleplayer(0);
        break;
    case 'd':
        duo();
        break;
    case 'f':
        ffa();
        break;
    case 'b':
        loggedMenu("%%");
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n", "Unrecognized");
        sleep(1);
        game();
    }
}
void history()
{
    system("clear");      // Clear screen
    tcflush(0, TCIFLUSH); // Empty STDIN buffer

    char decision = 'h';
    char *options = "B to go back\nH to check other player's history\n";
    char history_user[1024];
    char username[32];
    char response[64];
    int c, user_size;
    int nr_of_games;

    printf("%s", "Username:");
    if (write(sd, &decision, sizeof(decision)) <= 0) // Sending command history to the server
    {
        perror("Error sending command history to server\n");
        sleep(1);
        loggedMenu("%%");
    }

    scanf("%s", username);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer

        if (strlen(username) > 0 && username[strlen(username)] == '\n') // Removes \n from username
            username[strlen(username) - 1] = '\0';
    user_size = strlen(username);

    if (write(sd, &user_size, sizeof(user_size)) <= 0) // Sending prefixated username length to the server
    {
        perror("Error sending username to the server.\n");
        sleep(1);
        loggedMenu("%%");
    }

    if (write(sd, &username, user_size) <= 0) // Sending username to the server
    {
        perror("Error sending username to the server.\n");
        sleep(1);
        loggedMenu("%%");
    }
    if (read(sd, &nr_of_games, sizeof(nr_of_games)) <= 0) // Receiving the number of games of the user
    {
        perror("Error getting history data\n");
        sleep(1);
        loggedMenu("%%");
    }

    if (nr_of_games == -1 || nr_of_games == 0)
    {
        nr_of_games == -1 ? printf("User not found\n\n") : printf("User has not played any games yet\n\n");
        printf("%s\n", options);
        scanf("%c", &decision);
        while ((c = getchar()) != '\n' && c != EOF)
            decision = tolower(decision);

        switch (decision)
        {
        case 'b':
            loggedMenu("%%");
            break;
        case 'h':
            history();
            break;
        default:
            printf("%s\n", "Unrecognized");
            sleep(1);
            loggedMenu("%%");
        }
    }

    printf("\nNr of games:%d \n\n", nr_of_games);

    int gameid, score, nr_of_users;
    char winner[32];                                            // username of the winner
    char gametype[5];                                           // type of the game
    char username_game[32];                                     // username
    char spaces[39] = "                                      "; // 38 spaces

    for (int i = 0; i < nr_of_games; i++)
    {
        memset(gametype, 0, strlen(gametype));
        memset(winner, 0, strlen(winner));
        if (read(sd, &gameid, sizeof(int)) <= 0) // Receiving the id of the game
        {
            perror("Error reading gameid.\n");
            sleep(1);
            loggedMenu("%%");
        }

        if (read(sd, &gametype, sizeof(gametype)) <= 0) // Receiving the type of the game
        {
            perror("Error reading game type.\n");
            sleep(1);
            loggedMenu("%%");
        }
        if (read(sd, &winner, sizeof(winner)) <= 0) // Receiving the username of the winner
        {
            perror("Error reading game winner.\n");
            sleep(1);
            loggedMenu("%%");
        }

        printf("Match id:%d \nGame type:%s\n", gameid, gametype);
        if (strlen(gametype) < 4) // No winners in a solo game
            printf("Winner:%s\n\n", winner);
        else
            printf("\n");
        printf("Usernames                             Scores\n");
        printf("-------------------------------------------------------\n"); // 65 -

        if (read(sd, &nr_of_users, sizeof(int)) <= 0) // Receiving the nr of users in the game
        {
            perror("Error reading the number of players.\n");
            sleep(1);
            loggedMenu("%%");
        }

        for (int k = 0; k < nr_of_users; k++) // Receiving the username and score for each user
        {
            score = 0;
            memset(username_game, 0, strlen(username_game)); // Receiving the username of the user

            if (read(sd, &username_game, sizeof(username_game)) <= 0)
            {
                perror("Error reading players.\n");
                sleep(1);
                loggedMenu("%%");
            }

            if (read(sd, &score, sizeof(int)) <= 0) // Receiving the score of the user
            {
                perror("Error reading score.\n");
                sleep(1);
                loggedMenu("%%");
            }
            printf("%s%s%d\n", username_game, spaces + strlen(username_game), score);
        }
        printf("-------------------------------------------------------\n\n"); // 65
    }
    printf("%s\n", options);
    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        decision = tolower(decision);
    switch (decision)
    {
    case 'b':
        loggedMenu("%%");
        break;
    case 'h':
        history();
        break;
    default:
        printf("%s\n", "Unrecognized");
        sleep(1);
        loggedMenu("%%");
    }
}
void howtoplay()
{
    system("clear");
    tcflush(0, TCIFLUSH); // Empty STDIN buffer
    char *options = "- Each game has 10 questions and lasts 150 seconds \n- Each player will receive the same question and answers\n- The player with the most correct answers will win the game\n- Failing to provide an answer or answering incorrectly will result in 0 points\n- Once an answer is submitted, it cannot be changed\n\nB to go back\nQ to quit";
    char decision;
    int c;
    printf("%s\n", options);
    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        decision = tolower(decision);
    switch (decision)
    {
    case 'b':
        loggedMenu("%%");
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n", "Unrecognized");
        sleep(1);
        howtoplay();
    }
}
void gametypes()
{
    system("clear");
    tcflush(0, TCIFLUSH); // Empty STDIN buffer
    char decision;
    const char *options = {"Solo game(Practice mode) - 1 player\nDuo game - 2 players\nFfa game - 10 players \n\nB to go back\nQ to Quit\n"};
    int c;
    printf("%s\n", options);
    scanf("%c", &decision);

    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        decision = tolower(decision);
    switch (decision)
    {
    case 'b':
        tutorial();
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n", "Unrecognized");
        sleep(1);
        tutorial();
    }
}

void questioncategories()
{
    system("clear");
    tcflush(0, TCIFLUSH); // Empty STDIN buffer
    char decision;
    int c;
    char options[404];
    sprintf(options,
            "General Knowledge\nBooks\nFilm\nMusic\nTelevision\nVideo Games\nBoard Games\nScience & Nature\nScience: Computers\n"
            "Science: Mathematics\nMythology\nSports\nGeography\nHistory\nPolitics\nCelebrities\nAnimals\nVehicles\nComics\n"
            "Japanesse Anime & Manga\nCartoon & Animations\n\nB to go back\nQ to quit\n");

    printf("%s\n", options);
    scanf("%c", &decision);

    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer

        decision = tolower(decision);
    switch (decision)
    {
    case 'b':
        tutorial();
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n", "Unrecognized");
        sleep(1);
        tutorial();
    }
}

void tutorial()
{
    system("clear");
    tcflush(0, TCIFLUSH); // Empty STDIN buffer
    char decision;
    char *options = "H for how to play\nG for game types\nC for questions categories\nB to go back\nQ to Quit\n";
    int c;

    printf("%s\n", options);
    scanf("%c", &decision);

    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer

        decision = tolower(decision);
    switch (decision)
    {
    case 'h':
        howtoplay();
        break;
    case 'g':
        gametypes();
        break;
    case 'c':
        questioncategories();
        break;
    case 'b':
        loggedMenu("%%");
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n", "Unrecognized");
        sleep(1);
        tutorial();
    }
}
void logout()
{
    char decision = 'o';
    if (write(sd, &decision, sizeof(char)) <= 0) // Sending logout request to the server
    {
        perror("Error sending logout request to server.\n");
        sleep(1);
        loggedMenu("%%");
    }
    menu();
}
void loggedMenu(char *username)
{
    system("clear");
    tcflush(0, TCIFLUSH); // Empty STDIN buffer

    const char *welcome = "%%";
    const char *options = "J to join a Game\nH for history\nT for tutorial|info\nL to logout\nQ to Quit\n";
    char decision = {'\0'};
    int c;
    if (strcmp(username, welcome) != 0)
    {
        printf("Welcome %s\n\n", username);
    }

    printf("%s\n", options);

    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        decision = tolower(decision);

    switch (decision)
    {
    case 'j':
        game();
        break;
    case 'h':
        history();
        break;
    case 't':
        tutorial();
        break;
    case 'l':
        logout();
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n", "Unrecognized");
        sleep(1);
        loggedMenu("%%");
    }
}
void login()
{
    system("clear");
    tcflush(0, TCIFLUSH); // Empty STDIN buffer

    char decision = 'l';

    if (write(sd, &decision, sizeof(char)) <= 0)
    {
        perror("Error sending loign request to server\n");
        sleep(1);
        menu();
    }
    char username[32]; // username
    char password[32]; // password
    char userdata[64]; // username%password
    char ch = '%';     // separator for encoding
    char response;
    int c, user_size;
    static struct termios oldt, newt;

    printf("%s", "Username:");

    scanf("%s", username);
    while ((c = getchar()) != '\n' && c != EOF)
        ;

    if (strlen(username) > 0 && username[strlen(username) - 1] == '\n') // Removes \n from username
        username[strlen(username) - 1] = '\0';

    printf("%s", "Password:");
    /* changing STDIN display to hide the password while typing it*/
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    scanf("%s", password);
    while ((c = getchar()) != '\n' && c != EOF)
        ;
    /* changing STDIN back to normal display */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    if (strlen(password) > 0 && password[strlen(password)] == '\n') // Removes \n from password
        password[strlen(password) - 1] = '\0';
    printf("\n");

    user_size = strlen(username) + strlen(password) + 2;

    sprintf(userdata, "%s%c%s", username, ch, password);

    if (write(sd, &user_size, sizeof(int)) <= 0) // Sending the encoded string length to the server
    {
        perror("Error sending credentials to the server.\n");
        sleep(1);
        menu();
    }

    if (write(sd, &userdata, user_size) <= 0) // Sending the encoded string to the server
    {
        perror("Error sending credentials to the server.\n");
        sleep(1);
        menu();
    }

    if (read(sd, &response, sizeof(char)) <= 0) // Receving response from the server
    {
        perror("Failed to get a response from the server\n");
        sleep(1);
        menu();
    }
    switch (response)
    {
    case 'l':
        strcpy(logged_user, username);
        loggedMenu(username);
        break;
    case 'x':
        printf("Already logged in\n");
        sleep(1);
        menu();
        break;
    default:
        printf("Wrong username or password\n");
        sleep(1);
        menu();
    }
}
void createAccount()
{
    system("clear");
    tcflush(0, TCIFLUSH); // Empty STDIN buffer

    const char *options = "C to create account\nB to go back \nQ to quit \n \n";
    char decision = {'\0'};
    int c;

    printf("%s\n", options);

    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF) // Empty STDIN buffer
        ;
    decision = tolower(decision);

    switch (decision)
    {
    case 'c':
        break;
    case 'b':
        menu();
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n\n", "Unrecognized");
        sleep(1);
        menu();
    }

    int user_size;
    char username[32];  // username
    char password[32];  // password
    char password2[32]; // password confirmation
    char userdata[64];  // "username%password" string
    char ch = '%';      // separator for encoding
    char result;
    static struct termios oldt, newt;

    const char *allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; // Allowed characters for username and password

    const char *username_request = "Enter username between 6 and 31 characters:";
    const char *password_request = "Enter password between 6 and 31 characters:";
    const char *password_confirmation = "Retype password:";

    printf("%s", username_request);
    scanf("%s", username);
    while ((c = getchar()) != '\n' && c != EOF) // empty STDIN buffer
        ;

    while (strlen(username) > 0 && username[strlen(username) - 1] == '\n') // Removes \n from username
        username[strlen(username) - 1] = '\0';

    if (strlen(username) < 6 || strlen(username) > 31) // Checking if the length restrictions are met
    {
        strlen(username) < 6 ? printf("Username %s is too short\n", username) : printf("Username %s is too long\n", username);
        sleep(1);
        createAccount();
    }

    for (int index = 0; index < strlen(username); index++)
        if (strchr(allowed, username[index]) == NULL) // Checking if the characters restrictions are met
        {
            printf("Username can`t contain the character %c index %d\n", username[index], index);
            sleep(1);
            createAccount();
        }

    printf("%s", password_request);

    /* changing STDIN display to hide the password while typing it*/

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    scanf("%s", password);
    while ((c = getchar()) != '\n' && c != EOF) // Empty stdin buffer
        ;
    /* changing STDIN back to normal display */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    printf("\n");

    while (strlen(password) > 0 && password[strlen(password) - 1] == '\n') // Removes \n from password
        password[strlen(password) - 1] = '\0';

    if (strlen(password) < 6 || strlen(password) > 31) // Checking if the length restrictions are met
    {
        strlen(password) < 6 ? printf("Password %s is too short\n", password) : printf("Password %s is too long\n", password);
        sleep(1);
        createAccount();
    }

    for (int index = 0; index < strlen(password); index++)
        if (strchr(allowed, password[index]) == NULL) // Checking if the characters restrictions are met
        {
            printf("Password can`t contain the character %c index %d\n", password[index], index);
            sleep(1);
            createAccount();
        }

    printf("%s", password_confirmation);
    /* changing STDIN display to hide the password while typing it*/
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    scanf("%s", password2);
    while ((c = getchar()) != '\n' && c != EOF) // Empty stdin buffer
        ;
    /* changing STDIN back to normal display */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    printf("\n");

    while (strlen(password) > 0 && password2[strlen(password2) - 1] == '\n') // Removes \n from password2
        password2[strlen(password2) - 1] = '\0';

    if (strcmp(password, password2) != 0) // Checking if passwords match
    {
        printf("Passwords don't match\n");
        sleep(1);
        createAccount();
    }

    user_size = strlen(username) + strlen(password) + 2;

    sprintf(userdata, "%s%c%s", username, ch, password);
    if (write(sd, &decision, sizeof(decision)) <= 0) // Sending account creation request to the server
    {
        perror("Error sending the command create account to server\n");
        sleep(1);
        menu();
    }

    if (write(sd, &user_size, sizeof(int)) <= 0) // Sending the prefixated encoded string length
    {
        perror("Error sending credentials to the server.\n");
        sleep(1);
        menu();
    }
    if (write(sd, &userdata, user_size) <= 0) // Sending the encoded string to the server
    {
        perror("Error sending credentials to the server.\n");
        sleep(1);
        menu();
    }
    if (read(sd, &result, sizeof(char)) <= 0) // Receiving the result of the account creation request
    {
        perror("Failed to receive an answer from the server)\n");
        sleep(1);
        menu();
    }
    switch (result)
    {
    case 'l':
        printf("Account created successfully\n");
        sleep(1);
        break;
    case 'x':
        printf("Username already taken\n");
        sleep(1);
        break;
    default:
        printf("Error creating account\n");
        sleep(1);
        break;
    }
    menu();
}
void leave()
{
    char decision = 'q';

    if (write(sd, &decision, 1) <= 0) // Sending logout request to the server
    {
        printf("Error sending quit command to server\n");
        sleep(1);
        menu();
    }
    return;
}
void menu()
{
    system("clear");
    tcflush(0, TCIFLUSH);
    char decision = {'\0'}; // Stores the user's command
    int c;

    const char *options = "L to login \nC to create account \nQ to quit \n \n";
    printf("%s\n", options);

    scanf("%c", &decision);
    while ((c = getchar()) != '\n' && c != EOF)
        ; // empty the stdin buffer

    decision = tolower(decision);

    switch (decision)
    {
    case 'l':
        login();
        break;
    case 'c':
        createAccount(0);
        break;
    case 'q':
        leave();
        exit(0);
        break;
    default:
        printf("%s\n\n", "Unrecognized");
        sleep(1);
        menu();
    }
}
int main()
{
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Error establishing connection with the server.\n");
        sleep(1);
        exit(0);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("192.168.174.128");
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        printf("Server is currently under maintenance. Try again later.\n");
        sleep(1);
        exit(0);
    }
    menu();
    return 0;
}
