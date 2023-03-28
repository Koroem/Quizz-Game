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

#define PORT 2777

extern int errno;

/* server */

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
struct client
{
  char username[32];
  int socket_address;
  int flag;
};
struct game
{
  struct client users[3];
  int nr_of_users;
  int idThread;
  char category[40];
};

/* end */

/* database */
sqlite3 *db;   // Pointer to the database
char *err_msg; // For error message
int rc;        // For error handling database operations
/* end */

/* game */

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
static void *treat();
void game();
/* end */

int main()
{

  signal(SIGPIPE, SIG_IGN);

  /* when the client abruptly terminates and the server attemps to write to a closed socket it receives the singnal
  SIGPIPE and terminates execution, we change the handler of SIGPIPE to SIG_IGN which ignores the signal and does nothing*/

  struct sockaddr_in server;
  struct sockaddr_in from;

  fd_set read_sockets;   // Ready to read descriptors
  fd_set active_sockets; // Active descriptors

  struct timeval tv; // Time structure for timeout of select

  int sd, client; // Socket descriptors
  int optval = 1; // For SO_REUSEADDR
  int fd;         // For iterating over the descriptors set
  int max_socket;
  int len; // sockaddr_in length

  char ping = 'l';
  char response;

  int var, kar, j, ci, cj; // Variables used for iterations
  int k, ping_response;

  pthread_t th[1000000]; // Array of thread identificators

  int nr_of_games = 0; // The game identificator

  rc = sqlite3_open("intrebari.db", &db); // //  Opening intrebari database
  if (rc)
  {
    fprintf(stderr, "Error opening the database: %s\n", sqlite3_errmsg(db));
    return (0);
  }

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Error creating socket.\n");
    return errno;
  }

  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  memset(&server, 0, sizeof(server));
  memset(&from, 0, sizeof(from));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("Error binding\n");
    return errno;
  }

  if (listen(sd, 210) == -1)
  {
    perror("Error listening().\n");
    return errno;
  }

  FD_ZERO(&active_sockets);
  FD_SET(sd, &active_sockets);

  tv.tv_sec = 1;
  tv.tv_usec = 0;

  max_socket = sd;

  printf("(FFA-Server) Waiting for games to start...\n\n");
  fflush(stdout);

  struct game users_data[20];
  for (j = 0; j < 21; j++)
    switch (j)
    {
    case 0:
      strcpy(users_data[j].category, "General Knowledge");
      break;
    case 1:
      strcpy(users_data[j].category, "Books");
      break;
    case 2:
      strcpy(users_data[j].category, "Film");
      break;
    case 3:
      strcpy(users_data[j].category, "Music");
      break;
    case 4:
      strcpy(users_data[j].category, "Television");
      break;
    case 5:
      strcpy(users_data[j].category, "Video Games");
      break;
    case 6:
      strcpy(users_data[j].category, "Board Games");
      break;
    case 7:
      strcpy(users_data[j].category, "Science & Nature");
      break;
    case 8:
      strcpy(users_data[j].category, "Science: Computers");
      break;
    case 9:
      strcpy(users_data[j].category, "Science: Mathematics");
      break;
    case 10:
      strcpy(users_data[j].category, "Mythology");
      break;
    case 11:
      strcpy(users_data[j].category, "Sports");
      break;
    case 12:
      strcpy(users_data[j].category, "Geography");
      break;
    case 13:
      strcpy(users_data[j].category, "History");
      break;
    case 14:
      strcpy(users_data[j].category, "Politics");
      break;
    case 15:
      strcpy(users_data[j].category, "Celebrities");
      break;
    case 16:
      strcpy(users_data[j].category, "Animals");
      break;
    case 17:
      strcpy(users_data[j].category, "Vehicles");
      break;
    case 18:
      strcpy(users_data[j].category, "Comics");
      break;
    case 19:
      strcpy(users_data[j].category, "Japanesse Anime & Manga");
      break;
    case 20:
      strcpy(users_data[j].category, "Cartoon & Animations");
      break;
    }

  char decision;
  int index;
  char category[40];
  char username[32];
  int username_size, startgame = 0;

  while (1)
  {

    bcopy((char *)&active_sockets, (char *)&read_sockets, sizeof(read_sockets));
    if (select(max_socket + 1, &read_sockets, NULL, NULL, &tv) < 0)
    {
      perror("Errot selecting.\n");
      return errno;
    }

    if (FD_ISSET(sd, &read_sockets))
    {
      len = sizeof(from);
      memset(&from, 0, sizeof(from));

      client = accept(sd, (struct sockaddr *)&from, &len);

      if (client < 0)
      {
        perror("Error accepting client.\n");
        continue;
      }
      if (max_socket < client)
        max_socket = client;

      FD_SET(client, &active_sockets);
    }
    for (fd = 0; fd <= max_socket; fd++) /* Iterating over all the descriptors */
    {
      username_size = 0;

      if (fd != sd && FD_ISSET(fd, &read_sockets)) // Executes only 1 time for each socket
      {
        decision = '\0';
        index = 0;
        memset(username, 0, strlen(username));

        
          if (read(fd, &decision, sizeof(char)) <= 0) // Receiving category
          {
            perror("Error receiving decision\n");
            return errno;
          }
          switch (decision)
          {
          case 'a':
          {
            strcpy(category, "General Knowledge");
            index = 0;
            break;
          }
          case 'b':
          {
            strcpy(category, "Books");
            index = 1;
            break;
          }
          case 'c':
          {
            strcpy(category, "Film");
            index = 2;
            break;
          }
          case 'd':
          {
            strcpy(category, "Music");
            index = 3;
            break;
          }
          case 'e':
          {
            strcpy(category, "Television");
            index = 4;
            break;
          }
          case 'f':
          {
            strcpy(category, "Video Games");
            index = 5;
            break;
          }
          case 'g':
          {
            strcpy(category, "Board Games");
            index = 6;
            break;
          }
          case 'h':
          {
            strcpy(category, "Science & Nature");
            index = 7;
            break;
          }
          case 'i':
          {
            strcpy(category, "Science: Computers");
            index = 8;
            break;
          }
          case 'j':
          {
            strcpy(category, "Science: Mathematics");
            index = 9;
            break;
          }
          case 'k':
          {
            strcpy(category, "Mythology");
            index = 10;
            break;
          }
          case 'l':
          {
            strcpy(category, "Sports");
            index = 11;
            break;
          }
          case 'm':
          {
            strcpy(category, "Geography");
            index = 12;
            break;
          }
          case 'n':
          {
            strcpy(category, "History");
            index = 13;
            break;
          }
          case 'o':
          {
            strcpy(category, "Politics");
            index = 14;
            break;
          }
          case 'p':
          {
            strcpy(category, "Celebrities");
            index = 15;
            break;
          }
          case 'q':
          {
            strcpy(category, "Animals");
            index = 16;
            break;
          }
          case 'r':
          {
            strcpy(category, "Vehicles");
            index = 17;
            break;
          }
          case 's':
          {
            strcpy(category, "Comics");
            index = 18;
            break;
          }
          case 't':
          {
            strcpy(category, "Japanesse Anime & Manga");
            index = 19;
            break;
          }
          case 'u':
          {
            strcpy(category, "Cartoon & Animations");
            index = 20;
            break;
          }
          default:
          {
            strcpy(category, "General Knowledge");
            index = 0;
          }
          }
          if (read(fd, &username_size, sizeof(int)) <= 0) // Receiving username length
          {
            perror("Error receiving username length\n");
            return errno;
          }
          if (read(fd, &username, username_size) <= 0) // Receiving username
          {
            perror("Error receiving username\n");
            return errno;
          }
          printf("User %s has connected from %s. Category chosen: %s\n", username, conv_addr(from), category);
          users_data[index].users[users_data[index].nr_of_users].socket_address = fd;
          users_data[index].users[users_data[index].nr_of_users].flag = 1;
          strcpy(users_data[index].users[users_data[index].nr_of_users].username, username);

          FD_CLR(fd, &active_sockets); // Remove socket descriptor from active sockets
          users_data[index].nr_of_users++;

          if (users_data[index].nr_of_users == 3)
          {

            startgame = 2;

            for (k = 0; k < users_data[index].nr_of_users; k++)
            {
              /* Pings each socket to check if any users disconnected before starting the game*/

              if (write(users_data[index].users[k].socket_address, &startgame, sizeof(int)) <= 0) // Pinging the socket
              {
                close(users_data[index].users[k].socket_address);
                users_data[index].nr_of_users--;

                printf("User %s has disconnected.\n", users_data[index].users[k].username);

                startgame = 0;
                for (int j = k; j < users_data[index].nr_of_users; j++) // left shift users
                {
                  users_data[index].users[j].flag = users_data[index].users[j + 1].flag;
                  users_data[index].users[j].socket_address = users_data[index].users[j + 1].socket_address;
                  strcpy(users_data[index].users[j].username, users_data[index].users[j + 1].username);
                }
              }
              if (read(users_data[index].users[k].socket_address, &ping_response, sizeof(int)) <= 0) // Receiving ping response
              {

                close(users_data[index].users[k].socket_address);
                users_data[index].nr_of_users--;
                printf("User %s has disconnected.\n", users_data[index].users[k].username);

                startgame = 0;
                /* If any user disconnected ,we won't start the game and remove the user`s data from the structure by left shifting the elements at his position*/
                for (int j = k; j < users_data[index].nr_of_users; j++) // left shift users
                {
                  users_data[index].users[j].flag = users_data[index].users[j + 1].flag;
                  users_data[index].users[j].socket_address = users_data[index].users[j + 1].socket_address;
                  strcpy(users_data[index].users[j].username, users_data[index].users[j + 1].username);
                }
              }
            }
            if (startgame == 2) // We can start the game
            {
              startgame = 1;
              printf("\n");
              nr_of_games++;

              users_data[index].idThread = nr_of_games;
              for (k = 0; k < users_data[index].nr_of_users; k++)
                write(users_data[index].users[k].socket_address, &startgame, sizeof(int));

              pthread_create(&th[nr_of_games], NULL, &treat, &users_data[index]);
              // /* Waiting for the shared game data to be copied in the thread before we empty it in the root. Replace with a mutex?*/
              sleep(1);
              /* Emptying 'game lobby' structure for that category */
              for (k = 0; k < users_data[index].nr_of_users; k++)
               {
                    users_data[index].users[k].flag=0;
                    users_data[index].users[j].socket_address=0;
                    memset(users_data[index].users[k].username,0,sizeof(users_data[index].users[k].username));
               }
              users_data[index].nr_of_users = 0;
            }
          
        }
      }
    }
  }
}
static void *treat(void *data)
{
  struct game global_game = *((struct game *)data);
  struct game local_game_data = global_game;

  game(local_game_data);
  pthread_detach(pthread_self());
  for (int i = 0; i < local_game_data.nr_of_users; i++)
    close(local_game_data.users[i].socket_address);
  return (NULL);
}

void game(struct game local_game)
{
  struct user
  {
    char username[32];
    int socket;
    int score;
  };
  struct user user_data[3];
  int i;

  for (i = 0; i < local_game.nr_of_users; i++)
  {
    strcpy(user_data[i].username, local_game.users[i].username);
    user_data[i].socket = local_game.users[i].socket_address;
    user_data[i].score = 0;
  }

  char query[32846];
  char type[9];
  char question[512];
  char a1[512];
  char a2[512];
  char a3[512];
  char a4[512];
  char user_answer;
  int nr_of_active_clients = local_game.nr_of_users;

  int buffer_size;
  int multiple_order[4] = {0, 1, 2, 3};
  int boolean_order[2] = {0, 1};
  int path, user_path;
  unsigned int nr_questions = 10;

  sqlite3_stmt *result;
  const char *t;

  printf("The game %d has started\n", local_game.idThread);

  sprintf(query, "SELECT *FROM intrebari where category='%s'ORDER BY RANDOM() LIMIT %d;", local_game.category, nr_questions);

  if (sqlite3_prepare_v2(db, query, 128, &result, &t) != SQLITE_OK)
  {
    printf("Error generating questions%s\n", sqlite3_errmsg(db));
    for (path = 0; path < local_game.nr_of_users; path++)
      if (local_game.users[path].flag == 1) // flag=1 => active user;
        close(local_game.users[path].socket_address);
  }
  while (sqlite3_step(result) == SQLITE_ROW)
  {

    memset(type, 0, strlen(type));
    memset(question, 0, strlen(question));
    memset(a1, 0, strlen(a1));
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
        strcpy(a1, sqlite3_column_text(result, 3));
        strcpy(a2, sqlite3_column_text(result, 4));
        break;
      }
      case 1:
      {
        strcpy(a1, sqlite3_column_text(result, 4));
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
        strcpy(a1, sqlite3_column_text(result, 3));
        break;
      }
      case 1:
      {
        strcpy(a1, sqlite3_column_text(result, 4));
        break;
      }
      case 2:
      {
        strcpy(a1, sqlite3_column_text(result, 5));
        break;
      }
      case 3:
      {
        strcpy(a1, sqlite3_column_text(result, 6));
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

    for (user_path = 0; user_path < local_game.nr_of_users; user_path++)
    {

      if (local_game.users[user_path].flag == 1) // Only attempt to send quizz data to users who haven`t disconnected
      {

        buffer_size = (int)strlen(type);
        if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending type length
        {

          close(local_game.users[user_path].socket_address);
          local_game.users[user_path].flag = 0; // User disconnected, we change the flag to 0
          
          nr_of_active_clients--;
          printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
          continue;
        }

        if (write(local_game.users[user_path].socket_address, &type, buffer_size) <= 0) // Sending type
        {
          close(local_game.users[user_path].socket_address);
          local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
         
          nr_of_active_clients--;
          printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
          continue;
        }

        buffer_size = (int)strlen(question);
        if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending question length
        {
          close(local_game.users[user_path].socket_address);
          local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
      
          nr_of_active_clients--;
          printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
          continue;
        }

        if (write(local_game.users[user_path].socket_address, &question, buffer_size) <= 0) // Sending question
        {
          close(local_game.users[user_path].socket_address);
          local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
          
          nr_of_active_clients--;
          printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
          continue;
        }

        if (strcmp(type, "boolean") == 0)
        {

          buffer_size = (int)strlen(a1);
          if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending A1 length
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
         
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          if (write(local_game.users[user_path].socket_address, &a1, buffer_size) <= 0) // Sending A1
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
            
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          buffer_size = (int)strlen(a2);
          if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending A2 length
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
           
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          if (write(local_game.users[user_path].socket_address, &a2, buffer_size) <= 0) // Sending A2
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
            
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
        }
        else
        {

          buffer_size = (int)strlen(a1);
          if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending A1 length
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
           
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          if (write(local_game.users[user_path].socket_address, &a1, buffer_size) <= 0) // Sending A1
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
            
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          buffer_size = (int)strlen(a2);
          if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending A2 length
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
         
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          if (write(local_game.users[user_path].socket_address, &a2, buffer_size) <= 0) // Sending A2
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
           
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          buffer_size = (int)strlen(a3);
          if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending A3 length
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
            
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          if (write(local_game.users[user_path].socket_address, &a3, buffer_size) <= 0) // Sending A3
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
           
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          buffer_size = (int)strlen(a4);
          if (write(local_game.users[user_path].socket_address, &buffer_size, sizeof(int)) <= 0) // Sending A4 length
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
          
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          if (write(local_game.users[user_path].socket_address, &a4, buffer_size) <= 0) // Sending A4
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
           
            nr_of_active_clients--;
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
        }
      }
    }
    sleep(15);
    for (user_path = 0; user_path < local_game.nr_of_users; user_path++)
    {

      if (local_game.users[user_path].flag == 1) // Only attempt to read the answer of users who haven`t disconnected
      {

        if (read(user_data[user_path].socket, &user_answer, sizeof(char)) <= 0) // Receiving User answer
        {
          
          close(local_game.users[user_path].socket_address);
          local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
          
          nr_of_active_clients--;
          printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
          continue;
        }
        if (strcmp(type, "boolean") == 0)
          switch (user_answer)
          {
          case 'a':
          {
            if (boolean_order[0] == 0)
              user_data[user_path].score++;
            break;
          }
          case 'b':
          {
            if (boolean_order[1] == 0)
              user_data[user_path].score++;
          }
          }
        if (strcmp(type, "multiple") == 0)
          switch (user_answer)
          {
          case 'a':
          {
            if (multiple_order[0] == 0)
              user_data[user_path].score++;
            break;
          }
          case 'b':
          {
            if (multiple_order[1] == 0)
              user_data[user_path].score++;
            break;
          }
          case 'c':
          {
            if (multiple_order[2] == 0)
              user_data[user_path].score++;
            break;
          }
          case 'd':
          {
            if (multiple_order[3] == 0)
              user_data[user_path].score++;
          }
          }
      }
    }
    if (nr_of_active_clients == 0)
      break;
  }
  /* The quiz has ended */



  int max_score = user_data[0].score;
  for (user_path = 1; user_path < local_game.nr_of_users; user_path++) // finding the index of the user with the highest score;
    if (user_data[user_path].score > max_score)
      max_score = user_data[user_path].score;

  int nr_of_winners = 0;
  for (user_path = 0; user_path < local_game.nr_of_users; user_path++)
    if (user_data[user_path].score == max_score)
      nr_of_winners++;
  int user_length;
  for (user_path = 0; user_path < local_game.nr_of_users; user_path++)
    if (local_game.users[user_path].flag == 1) // only attempt to send the quiz results to the users who haven`t disconnected
    {
      if (write(local_game.users[user_path].socket_address, &user_data[user_path].score, sizeof(int)) <= 0) // Sending score of the user
      {
        close(local_game.users[user_path].socket_address);
        local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
        
        printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
        continue;
      }

      if (write(local_game.users[user_path].socket_address, &nr_of_winners, sizeof(int)) <= 0) // Sending the number of winners
      {
        close(local_game.users[user_path].socket_address);
        local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
        
        printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
        continue;
      }
      int user_length;
      for (int k = 0; k < local_game.nr_of_users; k++)
      {
        user_length = 0;
        if (user_data[k].score == max_score)
        {
          if (write(local_game.users[user_path].socket_address, &user_data[k].score, sizeof(int)) <= 0) // Sending the score of the winner
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            
            continue;
          }
          user_length = strlen(user_data[k].username);
          if (write(local_game.users[user_path].socket_address, &user_length, sizeof(int)) <= 0) // Sending username length of the winner
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
           
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
          if (write(local_game.users[user_path].socket_address, &user_data[k].username, user_length) <= 0) // Sending username of the winner
          {
            close(local_game.users[user_path].socket_address);
            local_game.users[user_path].flag = 0; // user disconnected, we change the flag to 0
           
            printf("User %s has left game %d\n", local_game.users[user_path].username, local_game.idThread);
            continue;
          }
        }
      }
    }
  /* We close the remaining connections */

  printf("The game %d has ended\n", local_game.idThread);
  for (user_path = 1; user_path < local_game.nr_of_users; user_path++)
    if (local_game.users[user_path].flag == 1)
    {
      close(local_game.users[user_path].socket_address);
      
    }

  /* We connect to the main server to add the game to history because the database is already opened in the main
  server and we can`t perform write operations here*/

  int main_server_port = 2116, dd;
  char d = '+';
  char game_type[5] = "Ffa";
  struct sockaddr_in server_management;

  if ((dd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Error creating socket.\n");
    sleep(1);
    return;
  }
  server_management.sin_family = AF_INET;
  server_management.sin_addr.s_addr = inet_addr("192.168.174.128");
  server_management.sin_port = htons(main_server_port);
  if (connect(dd, (struct sockaddr *)&server_management, sizeof(struct sockaddr)) == -1)
  {
    perror("Error connecting to the main server\n");
    sleep(1);
    return;
  }

  if (write(dd, &d, sizeof(char)) <= 0) // Sending command to main server
  {
    perror("Error sending command history.\n");
    sleep(1);
    close(dd);
    return;
  }
  int size;
  size = strlen(game_type);
  if (write(dd, &size, sizeof(int)) <= 0) // Sending type of game length to the main server
  {
    perror("Error sending game type\n");
    sleep(1);
    close(dd);
    return;
  }

  if (write(dd, &game_type, size) <= 0) // Sending type of game to the main server
  {
    perror("Error sending game type\n");
    sleep(1);
    close(dd);
    return;
  }
  if (write(dd, &local_game.nr_of_users, sizeof(local_game.nr_of_users)) <= 0) // Sending the number of users for flexibility
  {
    perror("Error sending number of users\n");
    sleep(1);
    close(dd);
    return;
  }
  for (user_path = 0; user_path < local_game.nr_of_users; user_path++)
  {
    size = strlen(user_data[user_path].username);
    if (write(dd, &size, sizeof(int)) <= 0) // Sending username length to the main server
    {
      perror("Error sending user data\n");
      sleep(1);
      close(dd);
      return;
    }
    if (write(dd, &user_data[user_path].username, size) <= 0) // Sending username
    {
      perror("Error sending user data\n");
      sleep(1);
      close(dd);
      return;
    }

    if (write(dd, &user_data[user_path].score, sizeof(int)) <= 0) // Sending score
    {
      perror("Error sending user data\n");
      sleep(1);
      close(dd);
      return;
    }
  }
  /* To prevent closing the socket before all data has been read by the main server */
  int data_received_confirmation;
  if (read(dd, &data_received_confirmation, sizeof(int)) <= 0)
  {
    printf("Error receiving confirmation from the main server\n");
    close(dd);
    return;
  }
  printf("The game %d has been added to history\n\n", local_game.idThread);
  close(dd);
  return;
}
