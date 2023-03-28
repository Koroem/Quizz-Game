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
#include <ctype.h>


#define PORT 2116

extern int errno;

/* threads */
typedef struct thData
{
  int idThread;    // Thread id
  int cl;          // Descriptor returned by accept
} thData;
static void *treat(); // Function executed for each accepted client
int current_thread=0;   // The maximum thread id
pthread_mutex_t mutex; // To prvent threads from modifying current_thread at the same time
/* end */

/* database */
sqlite3 *db;       // Pointer to the database
char *err_msg;     // For error message
int rc;            // For error handling database operations
/* end */

/* server */
int sd;            // Socket descriptor

/* converts sockaddr_in object to a IP ADDRESS:PORT NUMBER formatted string */
char *conv_addr(struct sockaddr_in address)
{
  static char str[25];
  char port[7];
  bzero(port, 7);
  strcpy(str, inet_ntoa(address.sin_addr)); // IP address
  sprintf(port, ":%d", ntohs(address.sin_port)); // Port
  strcat(str, port); //IP ADDRESS: PORT NUMBER
  return (str);
}
/* end  */


int  answer();
void history();
void update_history();
void logout();
int login();
void createAccount();

int main()
{

  struct sockaddr_in server;	
  struct sockaddr_in from;	


  pthread_t th[100000];    // Array of thread identificators
  int optval = 1;         // For SO_REUSEADDR

  rc = sqlite3_open("quizGame.db", &db); // Opening quizGame database

  if (rc)
  {
    fprintf(stderr, "Failed opening quizGame database: %s\n", sqlite3_errmsg(db));
    return 0;
  }

  /* logging off all users when starting the server */

  rc = sqlite3_exec(db, "UPDATE users SET logged=0;", 0, 0, &err_msg);

  if (rc != SQLITE_OK)
  {
    fprintf(stderr, "Failed to log off users.%s\n",sqlite3_errmsg(db));
    return 0;
  }

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("Error creating socket\n");
    return errno;
  }

  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

  bzero (&server, sizeof (server));
  bzero (&from, sizeof (from));

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(PORT);

 
  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("Error at bind.\n");
    return errno;
  }

  if (listen(sd, 1000) == -1)
  {
    perror("Error at listen.\n");
    return errno;
  }
  pthread_mutex_init(&mutex, NULL);
  printf("(Main-Server) Waiting for user requests\n\n");

  while (1)
    {
        int client;
        int length = sizeof (from);
        thData * td;

        if ( (client = accept (sd, (struct sockaddr *) &from, &length)) < 0)
          {
            printf("Error establishing connection with a client\n");
            continue;
          }

        td=(struct thData*)malloc(sizeof(struct thData));	
        td->idThread=current_thread++;
        td->cl=client;

        pthread_create(&th[current_thread], NULL, &treat, td);	
    }
  
  sqlite3_close(db);
}
static void *treat(void *arg)
{
  int id, temp_id;
  struct thData tdL; 
  tdL= *((struct thData*)arg); 

  printf("Client %d has connected to the server. Allocating thread %d\n\n",tdL.cl,tdL.idThread);

    while (1)
    {
      temp_id = answer(tdL.cl, tdL.idThread);
      if (temp_id != -404 && temp_id != -999 && temp_id != 0)
        id = temp_id;

      if (temp_id == -999) // Received exit command or the client disconnected
      {
        logout(id);
       
        /* To prevent race condition where two threads would 
       decrement the shared variable at the same time */

        pthread_mutex_lock(&mutex);
        current_thread--;
        pthread_mutex_unlock(&mutex);
        close(tdL.cl);
        pthread_detach(pthread_self());	
        return (NULL);
        break;
      }
    }
}

int answer(int cl, int idThread)
{
 
  int logged_user_id;

  char decision;

  if (read(cl, &decision, sizeof(decision)) <= 0)
  {
    printf("[Thread %d] Client disconnected\n", idThread);
    return -999;
  }

  decision=tolower(decision);

  switch (decision)
  {
    case 'l':{
    printf("[Thread %d] Server received login command\n", idThread); logged_user_id = login(cl, idThread); 
    if (logged_user_id >= 0) printf("[Thread %d] User with id %d has logged in\n", idThread, logged_user_id);
    if (logged_user_id != -404) return logged_user_id; else return -404;}

    case 'c': printf("[Thread %d] Server received account creation command\n", idThread); createAccount(cl, idThread);  return 0; 
    case 'h': printf("[Thread %d] Server received history command\n", idThread); history(cl, idThread); return 0;
    case 'o': printf("[Thread %d] Server received logout command\n", idThread); logout(logged_user_id); return -404;
    case 'q': printf("[Thread %d] Server received exit command\n", idThread); return -999;
    case '+': printf("[Thread %d] Server received update history command \n", idThread); update_history(cl, idThread); return -999;

    default: printf("[Thread %d] Unrecognized\n", idThread); return 0; // Should never happen
  }
}

void update_history(int cl, int idThread)
{
  char username[32];
  char game_type[5];
  char query[33134];
  
  char user_history[32768]; // Enough for ~4650, 6 digit id games,193 hours of gameplay
  int id,rc,user_id,gameid,size;
 
  sqlite3_stmt *result;
  const char *t;
  

 if (read(cl, &size, sizeof(int)) <= 0)  // Receiving type length
  {
    printf("[Thread %d] Solo game server has disconnected \n", idThread);
    return;
  }
  if (read(cl, &game_type, size) <= 0) // Receiving type
  {
    printf("[Thread %d] Solo game server has disconnected \n", idThread);
    return;
  }

  if (strcmp(game_type, "Solo") == 0)
  {
    int score;

    if (read(cl, &size, sizeof(int)) <= 0) // Receiving username length
    {
      printf("[Thread %d] Solo game server has disconnected \n", idThread);
      return;
    }
    if (read(cl, &username, size) <= 0) // Receiving username
    {
      printf("[Thread %d] Solo game server has disconnected \n", idThread);
      return;
    }
    if (read(cl, &score, sizeof(int)) <= 0) // Receiving score
    {
      printf("[Thread %d] Solo game server has disconnected \n", idThread);
      return;
    }
     int data_received_confirmation=1;

    /* Ping the solo game server to confirm that all data has been successfully received */
    
     if (write(cl, &data_received_confirmation, sizeof(int)) <= 0) 
        {
          printf("[Thread %d] Solo game server has disconnected \n", idThread);
          return;
        }
    
    sprintf(query, "SELECT id,history from users where username='%s';", username);

    /* Finding an id to asign to the game */

    if (sqlite3_prepare_v2(db, query, 33133, &result, &t) != SQLITE_OK)
    {
      printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
      sqlite3_free(err_msg);
      return ;
    }

    if (sqlite3_step(result) == SQLITE_ROW)
    {
      user_id = sqlite3_column_int64(result, 0);            // Id of the player
      strcpy(user_history, sqlite3_column_text(result, 1)); // history of the player
    }

    sqlite3_stmt *result2;
    const char *t2;

    memset(query, 0, strlen(query));
    sprintf(query, "SELECT max(gameid) from history;");

    if (sqlite3_prepare_v2(db, query, 33133, &result2, &t2) != SQLITE_OK) // Query is invalid
    {
     
      printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
      sqlite3_free(err_msg);
      return ;
    }

    if (sqlite3_step(result2) == SQLITE_ROW) //
    {
      gameid = sqlite3_column_int64(result2, 0); // Game id
    }

    else
    {
      printf("Can't retrieve gameid: %s\n", sqlite3_errmsg(db));
    }

    gameid++; // Max gameid from the database +1

    memset(query, 0, strlen(query));

    sprintf(query, "INSERT INTO history values(%d,'Solo',%d,%d);", gameid, user_id, score);
    int rc = sqlite3_exec(db, query, 0, 0, &err_msg);
    if (rc != SQLITE_OK) 
    {
      fprintf(stderr, "Failed to insert game into the database. %s\n",sqlite3_errmsg(db));
      sqlite3_free(err_msg);
      return ;
    }

    char gameidstring[9];
    
    sprintf(gameidstring, "%d", gameid);

    memset(query, 0, strlen(query));

    if (strlen(user_history)<1) // 'GameID' for null history ',GameID' for non-null history
    { 
    strcat(user_history, ",");
    strcat(user_history, gameidstring);
    }
    else
    {
      strcat(user_history, ",");
      strcat(user_history, gameidstring);
    }

    sprintf(query, "UPDATE users set history='%s' where username='%s';", user_history, username);
    rc = sqlite3_exec(db, query, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
      fprintf(stderr, "Failed to the history of the user.%s\n",sqlite3_errmsg(db));
      sqlite3_free(err_msg);
      return;
    }
    return;
  }
  else 
  if(strcmp(game_type,"Ffa")==0||strcmp(game_type,"Duo")==0)
  {
    int nr_of_users,score,size;
    char gameidstring[9];
    char useridstring[9];
    char scorestring[9];
    char users_ids[128];
    char users_scores[128];

    if (read(cl, &nr_of_users, sizeof(nr_of_users)) <= 0) // Receiving the number of users
    {
      printf("[Thread %d] Client disconnected \n", idThread);
      return;
    }
    sprintf(query, "SELECT max(gameid) from history;");

    if (sqlite3_prepare_v2(db, query, 33133, &result, &t) != SQLITE_OK)
    {
      printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
      return;
    }
    if (sqlite3_step(result) == SQLITE_ROW)
    {
      gameid = sqlite3_column_int64(result, 0);
      gameid++;
    }
    else
    {
      printf("Can't retrieve gameid: %s\n", sqlite3_errmsg(db)); sleep(1);
    }
    
  
    sprintf(gameidstring, "%d", gameid);

    /* For each user in the game, we receive from the game server his username and score 
       and append to his history the game id */

    for (int k=0;k<nr_of_users;k++) // For each user in the game
    {
     
          memset(query,0,strlen(query));
          memset(useridstring,0,strlen(useridstring));
          memset(user_history,0,strlen(user_history));
          memset(scorestring,0,strlen(scorestring));
          memset(username,0,strlen(username));

          user_id=-1;
        
        if (read(cl, &size, sizeof(int)) <= 0) // Receiving the username length
        {
          printf("[Thread %d] The game server has disconnected\n", idThread);
          return;
        }

        if (read(cl, &username, size) <= 0) // Receiving the username
        {
          printf("[Thread %d] The game server has disconnected\n", idThread);
          return;
        }
        if (read(cl, &score, sizeof(int)) <= 0) // Receiving the score
        {
          printf("[Thread %d] The game server has disconnected\n", idThread);
          return;
        }

        sprintf(query, "SELECT id,history from users where username='%s';", username);

      
        if (sqlite3_prepare_v2(db, query, 33133, &result, &t) != SQLITE_OK)
        {
          printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
          sqlite3_free(err_msg);
          return ;
        }
        if (sqlite3_step(result) == SQLITE_ROW)
        {
          user_id = sqlite3_column_int64(result, 0);            // User id
          strcpy(user_history, sqlite3_column_text(result, 1)); // User history
        }
    
        if(strlen(user_history)<1)
        {
          sprintf(user_history, "%s", gameidstring);
        }
        else
        {
        strcat(user_history,",");
        strcat(user_history,gameidstring);
        }
        
    sprintf(query, "UPDATE users set history='%s' where username='%s';", user_history, username);

    rc = sqlite3_exec(db, query, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
      fprintf(stderr, "Failed to update the history of the user\n.%s",sqlite3_errmsg(db));
      sqlite3_free(err_msg);
      return;
    }
   
    sprintf(useridstring,"%d",user_id);
    sprintf(scorestring,"%d",score);

    if(strlen(users_ids)<1)
    {
      strcat(users_ids,useridstring);
      strcat(users_scores,scorestring);
    }
    else
    {
      strcat(users_ids,",");
      strcat(users_ids,useridstring);

      strcat(users_scores,",");
      strcat(users_scores,scorestring);
    }
    }
    
    memset(query, 0, strlen(query));
    /* We insert the game into history */

    strcmp(game_type,"Duo")==0?sprintf(query, "INSERT INTO history values(%d,'Duo','%s','%s');", gameid, users_ids, users_scores):sprintf(query, "INSERT INTO history values(%d,'Ffa','%s','%s');", gameid, users_ids, users_scores);
    rc = sqlite3_exec(db, query, 0, 0, &err_msg);

    if (rc != SQLITE_OK)
    {
      fprintf(stderr, "Failed to insert game into history. %s\n",sqlite3_errmsg(db));
      sqlite3_free(err_msg);
      return;
    }
    int data_received_confirmation=1;
    if (write(cl, &data_received_confirmation, sizeof(data_received_confirmation)) <= 0) // Sending confirmation
        {
          printf("[Thread %d] The game server has disconnected \n", idThread);
          return;
        }
  }
  return;
}

void history(int cl, int idThread)
{
  char query[256];
  char history[1024];
  char username[32];
  int user_size,result;
  struct sqlite3_stmt *selectstmt;

  if (read(cl, &user_size, sizeof(user_size)) <= 0) // Receiving username length
  {
    printf("[Thread %d] Error receiving username length \n", idThread);
    return;
  }
  if (read(cl, &username, user_size) <= 0) // Receiving username
  {
    printf("[Thread %d] Error receiving username \n", idThread);
    return;
  }
  printf("Username:%s|\n",username);
  sprintf(query, "SELECT *FROM users WHERE username='%s';", username); 
  

  result = sqlite3_prepare_v2(db, query, -1, &selectstmt, NULL);

  if (result == SQLITE_OK)
    if (sqlite3_step(selectstmt) != SQLITE_ROW) // Checking if the requested username is not in the database
    {
      /* User not found in the database */
      printf("[Thread %d] User not found in the database \n", idThread);
      int response = -1; 
      if (write(cl, &response, sizeof(response)) <= 0)
      {
        printf("[Thread %d] Failed sending response to the client\n", idThread);
        return;
      }
      return;
    }
    else 
    {
      /* User found in the database */
      sprintf(query, "SELECT history FROM users WHERE username='%s';", username); // Getting the id of the games where the username played
      sqlite3_stmt *r1;
      const char *t;
      if (sqlite3_prepare_v2(db, query, 128, &r1, &t) != SQLITE_OK)
      {
        printf("[Thread %d]Invalid query %s\n",idThread,sqlite3_errmsg(db));
        sqlite3_free(err_msg);
        return;
      }
      sqlite3_step(r1); // Valid query
      strcpy(history, sqlite3_column_text(r1, 0)); //History string for that user
      int history_ids[1024]; 
      char *p;
      int contor = 0;
      int i = 0;
      while ((p = strtok(contor ? NULL : history, ",")) != NULL) // The game ids are stored as a comma separated string
      {
        history_ids[contor++] = atoi(p); // Saving the game ids in an array
      }

      if (write(cl, &contor, sizeof(int)) <= 0) // Sending to the client the nr of games
      {
          printf("[Thread %d] Failed sending the number of games to the client.\n", idThread);
          return;
      }

      int gameid;
      char gametype[5]; // ffa/duo/solo
      char users[120]; // comma separated string of all the id of users who played in that game
      char game_usernames[10][32];
      char scores[10]; // comma seperated scores of all the users who played in that game
      char subquery[128];
      int users_id[10];     // ids extracted from string
      int users_scores[1000]; // scores extracted from string
      int index_strtok,max_score_index;

      for (i = 0; i < contor; i++) // iterating over all the games
      {
        sprintf(query, "SELECT  *FROM history WHERE gameid='%d';", history_ids[i]); // for each game id we will get info from the history database
        sqlite3_stmt *r2;
        const char *t2;
        if (sqlite3_prepare_v2(db, query, 128, &r2, &t2) != SQLITE_OK) //invalid query
        {
        printf("[Thread %d]Invalid query %s\n",idThread,sqlite3_errmsg(db));
        sqlite3_free(err_msg);
        return;
        }
        //valid query
        gameid = -1;
        memset(gametype, 0, strlen(gametype));
        memset(users, 0, strlen(users));
        memset(game_usernames, 0, sizeof(game_usernames));
        memset(scores, 0, strlen(scores));
        memset(users_id, 0, sizeof(users_id));
        memset(users_scores, 0, sizeof(users_scores));
        index_strtok = 0;

        while (sqlite3_step(r2) == SQLITE_ROW)
        {
          gameid = sqlite3_column_int64(r2, 0);         // gameid
          strcpy(gametype, sqlite3_column_text(r2, 1)); // type of game
          strcpy(users, sqlite3_column_text(r2, 2));    // string of users` id

          while ((p = strtok(index_strtok ? NULL : users, ",")) != NULL) // Exctracting user ids from comma separated string users
          {
            users_id[index_strtok++] = atoi(p); // string to integer
          }
          strcpy(scores, sqlite3_column_text(r2, 3));
          index_strtok = 0;
          while ((p = strtok(index_strtok ? NULL : scores, ",")) != NULL) // Exctracting scores from comma separated string scores
          {
            users_scores[index_strtok++] = atoi(p);
          }
          max_score_index = 0;
          for (int k = 0; k < index_strtok; k++)
            if (users_scores[k] > users_scores[max_score_index])
              max_score_index = k;

          sqlite3_stmt *r3;
          const char *t3;
          char sub_sub_query[128];

          for (int op = 0; op < index_strtok; op++) // Getting the username for each userid in the game
          {
            sprintf(sub_sub_query, "SELECT username FROM users where id='%d';", users_id[op]);
            
            if (sqlite3_prepare_v2(db, sub_sub_query, 128, &r3, &t3) != SQLITE_OK)
            {
             printf("[Thread %d]Invalid query %s\n",idThread,sqlite3_errmsg(db));
            sqlite3_free(err_msg);
            return;
            }
            while (sqlite3_step(r3) == SQLITE_ROW)
            {
              strcpy(game_usernames[op], sqlite3_column_text(r3, 0));
            }
          }
        }

        if (write(cl, &gameid, sizeof(int)) <= 0) // Sending to client the gameid
        {
          printf("[Thread %d] Failed sending the gameid.\n", idThread);
          return;
        }
         if (write(cl, &gametype, sizeof(gametype)) <= 0) // Sending to client the game type
        {
          printf("[Thread %d] Failed sending the game type.\n", idThread);
          return;
        }
        if (write(cl, &game_usernames[max_score_index], sizeof(username)) <= 0) // Sending to client the winner of the game
        {
           printf("[Thread %d] Failed sending the winner of the game.\n", idThread);
          return;
        }
        if (write(cl, &index_strtok, sizeof(index_strtok)) <= 0) // Sending to client the nr of users in the game
        {
           printf("[Thread %d] Failed sending the number of players.\n", idThread);
          return;
        }
        for (int idk = 0; idk < index_strtok; idk++) // Iteratinng over all users
        {

          if (write(cl, &game_usernames[idk], sizeof(username)) <= 0) // Sending to client the usernames
          {
             printf("[Thread %d] Failed sending the players' usernames.\n", idThread);
          return;
          }
          if (write(cl, &users_scores[idk], sizeof(int)) <= 0) // Sending to client the scores
          {
             printf("[Thread %d] Failed sending players' scores.\n", idThread);
          return;
          }
        }
      }
    }
}

void logout(int id)
{
  char query[256];
  sprintf(query, "UPDATE users SET logged=0 WHERE id='%d';", id); 

  int rc = sqlite3_exec(db, query, 0, 0, &err_msg);
  if (rc != SQLITE_OK)
  {
    fprintf(stderr, "Failed logging off user with id %d. %s\n",id,sqlite3_errmsg(db));
    sqlite3_free(err_msg);
  }
  return;
}
void createAccount(int cl, int idThread)
{
  char userdata[64];
  char username[32];
  char password[32];
  char query[256];
  char *token;
  int user_size, cont = 0, result;

  if (read(cl, &user_size, sizeof(int)) <= 0) // Receiving the length of the username%password string
  {
    fprintf(stderr, "[Thread %d] Failed receiving credentials length. %s\n", idThread, err_msg);
    return ;
  }

  if (read(cl, &userdata, user_size) <= 0) // Receiving the username%password string
  {
    fprintf(stderr, "[Thread %d] Failed receiving credentials. %s\n", idThread, err_msg);
    return ;
  }

  token = strtok(userdata, "%");

  while (token != NULL)
  {
    cont ? strcpy(password, token) : strcpy(username, token);
    cont++;
    token = strtok(NULL, "%");
  }

  sprintf(query, "SELECT *FROM users WHERE username='%s';", username);

  struct sqlite3_stmt *selectstmt;
  struct sqlite3_stmt *asignid;

  result = sqlite3_prepare_v2(db, query, -1, &selectstmt, NULL); // Compiling the query

  if (result == SQLITE_OK) // Query is valid 
  {
    if (sqlite3_step(selectstmt) == SQLITE_ROW) // Username already taken
    {
      char response = 'x';

      if (write(cl, &response, sizeof(char)) <= 0) // Sending the client the answer 'x' (account creation request denied)
      {
        fprintf(stderr, "[Thread %d] Failed to send account creation request response to the client.%s\n", idThread, err_msg);
        return ;
      }
    }
    else /* Username not in the databse */
    {
      int id, sub_result;

      memset(query, 0, strlen(query));
      sprintf(query, "SELECT max(id) from users;"); // Finding an id for the user

      sub_result = sqlite3_prepare_v2(db, query, 256, &asignid, NULL);

      if (sub_result != SQLITE_OK) // Query is invalid
      {
        fprintf(stderr, "[Thread %d] Failed to fetch the maximum id. %s\n", idThread, sqlite3_errmsg(db));
        sqlite3_free(err_msg);
        return;
      }
      else if (sqlite3_step(asignid) == SQLITE_ROW) // Query is valid
      {
        id = sqlite3_column_int(asignid, 0);
        id++;
      }
      sqlite3_finalize(asignid);
      memset(query, 0, strlen(query));
      sprintf(query, "INSERT INTO users (id,username,password,history,logged) values(%d,'%s','%s','',%d);", id, username, password, 0);

      sub_result = sqlite3_exec(db, query, 0, 0, &err_msg);

      if (sub_result != SQLITE_OK) // Execution failed
      {
        fprintf(stderr, "[Thread %d] Failed to create account. %s\n", idThread, sqlite3_errmsg(db));
        sqlite3_free(err_msg);

        char response = 'f';
        if (write(cl, &response, sizeof(char)) <= 0) // Sending the client the answer 'f' (account creation request denied)
        {
          fprintf(stderr, "[Thread %d] Failed to send account creation request response to the client.%s\n", idThread, err_msg);
          return;
        }
        return;
      } 
      else // Execution successful
      {
        char response = 'l';
        if (write(cl, &response, sizeof(response)) <= 0) // Sending the client the answer 'l' (account creation request successful)
        {
          fprintf(stderr, "[Thread %d]Error sending response. %s\n", idThread, err_msg);
          return ;
        }
      }
    }
  } // Query is invalid
  else
  {
    fprintf(stderr, "[Thread %d] Failed to check if the username exists in the database. %s\n", idThread, sqlite3_errmsg(db));
    sqlite3_free(err_msg);
    return;
  }
  sqlite3_finalize(selectstmt);
}

int login(int cl, int idThread)
{

  char data[64];
  char username[32];
  char password[32];
  char query[128];
  char *token;
  int cont = 0;
  int id, user_size, result, rc;

  if (read(cl, &user_size, sizeof(int)) <= 0) // Receiving the length of the username%password string
  {
    fprintf(stderr, "[Thread %d] Failed to receive credentials length. %s\n", idThread, err_msg);
    return 0;
  }

  if (read(cl, &data, user_size) <= 0) // Receiving the username%password string
  {
    fprintf(stderr, "[Thread %d] Failed to receive credentials. %s\n", idThread, err_msg);
    return 0;
  }
 
  token = strtok(data, "%");

  while (token != NULL)
  {
    cont ? strcpy(password, token) : strcpy(username, token);
    cont++;
    token = strtok(NULL, "%");
  }
  
  sprintf(query, "SELECT *FROM users WHERE username='%s' and password='%s';", username, password);

  struct sqlite3_stmt *selectstmt;

  result = sqlite3_prepare_v2(db, query, -1, &selectstmt, NULL);

  if (result == SQLITE_OK) // Query is valid
  {
    if (sqlite3_step(selectstmt) == SQLITE_ROW) // Credentials are correct
    {
      /* check if user isn`t already logged in */
      sqlite3_stmt *r1;

      memset(query, 0, strlen(query));
      sprintf(query, "SELECT logged FROM users where username='%s';", username);

      if (sqlite3_prepare_v2(db, query, 128, &r1, NULL) != SQLITE_OK) // Query is invalid
      {
        printf("[Thread %d] Failed to get logged status of user %s\n", idThread, sqlite3_errmsg(db));
        return -404;
      }

        if (sqlite3_step(r1) == SQLITE_ROW) // User has a valid logged field
        {

          if (sqlite3_column_int64(r1, 0) == 1) // User is already logged in
          {

            sqlite3_finalize(r1);
            char result = 'x';
            if (write(cl, &result, sizeof(char)) <= 0) // Sending the client the answer 'x' (login request denied)
            {
              fprintf(stderr, "[Thread %d] Failed to send login request response to the client. %s\n", idThread, err_msg);
              return -404;
            }
            return -404;
          }
          else // User is not logged in
          {

            sqlite3_finalize(r1);

            memset(query, 0, strlen(query));
            sprintf(query, "UPDATE users SET logged=1 WHERE username='%s';", username);

            rc = sqlite3_exec(db, query, 0, 0, &err_msg); // Updating the logged flag of the user to 1

            if (rc != SQLITE_OK) // Failed to update the logged flag of the user
            {
              sqlite3_finalize(selectstmt);
              printf("[Thread %d] Failed to update user`s logged field.%s\n",idThread, sqlite3_errmsg(db)); 
              sqlite3_free(err_msg);
              return -404;
            }
            else // Succeeded setting the logged flag of the user to 1
            {

              sqlite3_finalize(selectstmt);
              memset(query, 0, strlen(query));

              sprintf(query, "SELECT id FROM users where username='%s';", username);

              if (sqlite3_prepare_v2(db, query, 128, &r1, NULL) != SQLITE_OK) // Query is invalid
              {
                printf(" [Thread %d] Failed to get the user`s id. %s\n",idThread, sqlite3_errmsg(db));
                sqlite3_free(err_msg);
                return -404;
              } 
              else // Query is valid
              {
                sqlite3_step(r1);
                id = sqlite3_column_int64(r1, 0);
                char result = 'l';
                if (write(cl, &result, sizeof(char)) <= 0) // Sending the client the answer 'l' (login request successful)
                {
                  fprintf(stderr, "[Thread %d] Failed to send login request response to the client. %s\n", idThread, err_msg);
                  return -404;
                }
                else // Login request was successful. We save the id of the user for logout
                  return id;
              }
            }
          }
        } //User doesn't have a logged field (database corruption)
        else
        {
          printf("[Thread %d]User %s doesn`t have a valid logged field in the database\n", idThread, username);
          char result = 'f';
          if (write(cl, &result, sizeof(char)) <= 0) // Sending the client the answer 'f' (login request denied)
          {
            fprintf(stderr, "[Thread %d] Failed to send login request response to the client. %s\n", idThread, err_msg);
            return -404;
          }
          return -404;
        }
    }
      else // Credentials are incorrect
      {
        char response = '0';
        if (write(cl, &response, sizeof(char)) <= 0) // Sending the client the answer '0' (login request denied)
        {
          fprintf(stderr, "[Thread %d] Failed to send response. %s\n", idThread, err_msg);
          return 0;
        }
        else
          return -404;
      }
    } // Invalid query
    else
    {
      printf("[Thread %d] Invalid query. %s\n", idThread,sqlite3_errmsg(db));
      sqlite3_free(err_msg);
      return -404;
    }
    return -404;
  }
