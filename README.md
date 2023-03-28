
# QuizzGame

QuizzGame is a popular game in which players are asked questions about
different topics and they have to get as many correct answers as possible.

In my implementation, each player will be required to have a
registered account to play the game. There will be 3 types of games available for
users. A single player game/training mode, where the user plays alone, a duo
game where the user competes with another user, and a free for all game where
10 users compete against eachother. There will be 21 categories of questions for
users to choose from. The matchmaking will select users who have chosen the
same category and game type. Users will also have access to the history of all
games that have been played and there will be a tutorial section that explains
the game features to the user and how the game works.

## Authors

- Victor Condurat [@Koroem](https://github.com/Koroem) (github)

 
## Demos
- https://streamable.com/gsmle4 (Start)
- https://streamable.com/wofxl1 (Login)
- https://streamable.com/7igsj4 (Account creation)
- https://streamable.com/4dj6hu (History)
- https://streamable.com/1t8tnp (Singleplayer games)
- https://streamable.com/0oonar (Duo games)
- https://streamable.com/n2ijz2 (FFA games)

## Documentation

[Documentation](https://linktodocumentation)


## Features

- 3 Different game types: Solo (1 player), Duo (2 players), FFA (10 players)
- 21 Question categories and 50 questions for each category
- Only logged-in users with a valid account can play the game
- Any logged-in user can check the game history of any user
- Tutorial section
- Easy to use and intuitive interface


## Installation

To install the sqlite3 library, you can use the command sudo apt-get install libsqlite3-dev on Ubuntu or Debian-based systems.

```bash
sudo apt-get install libsqlite3-dev
```
    
## Execution
To open all the servers and one client you can run the start.sh script. You also need to replace the IP adddress in the Client
with your own (line 1975) . The IP address was removed from the 'terminal' argument for simplicity and testing.
```c
server.sin_addr.s_addr = inet_addr("$$$.$$$.$$$.$$$");
```

```bash
./start.sh
```
The script can be modified to open a different number of clients. Or the servers and the client can be compiled and opened individually

```bash
gcc Server.c -o server -l sqlite3  -pthread
gcc ffa_server.c -o ffa -l sqlite3  -pthread
gcc solo_server.c -o solo -l sqlite3  -pthread
gcc duo_server.c -o duo -l sqlite3  -pthread

./server
./ffa
./solo
./duo
```

```bash
gcc Client.c -o client -l sqlite3 -pthread -lm
./client
```
## License

[MIT](https://choosealicense.com/licenses/mit/)


## Screenshots

- https://ibb.co/8jxqqjt
- https://ibb.co/2N8Y4w4
- https://ibb.co/gwXdgHH

## Roadmap
 - A method to prevent bad actors from overfilling the database by creating hundreds of thousands of accounts (captcha, email registration and confirmation, ip blacklist, etc)
 - Be able to play in teams
 - A GUI
- Correct answers/Wrong answers (similar to k/d) and quiz win/lose ratios
- Waiting queue if the number of max threads is exceeded
- Password reset/Forgot password method
- In game chat between players
- Players to be able to rejoin the game if they disconnected
- Different levels of difficulty
- Custom number of rounds per game.
- Automatically add questions to the database with the Trivia API
- Custom games
- Single elimination tournament
- Users to be able to choose a user to play against
- Users to be logged out after a number of minutes of inactivity
- Delete inactive accounts after a number of months

