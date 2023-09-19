#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sqlite3.h>
#include <pthread.h>


#define PORT 5000
#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 256

// Structura pentru un mesaj
typedef struct {
  int id;
  char sender[MAX_MESSAGE_LENGTH];
  char receiver[MAX_MESSAGE_LENGTH];
  char message[MAX_MESSAGE_LENGTH];
} Message;

// Structura pentru un client
typedef struct {
  int sock;
  char username[MAX_MESSAGE_LENGTH];
} Client;

// Vector de clienti
Client clients[MAX_CLIENTS+5];

int messageidcounter = 1;

// Functie pentru trimiterea mesajelor catre toti clientii
void broadcast(char *sender, char *message) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].sock != 0) {
      send(clients[i].sock, sender, strlen(sender), 0);
      send(clients[i].sock, ": ", 2, 0);
      send(clients[i].sock, message, strlen(message), 0);
      send(clients[i].sock, "\n", 1, 0);
    }
  }
}

// Functie pentru trimiterea mesajelor catre un client specificat
void sendToClient(char *sender, char *receiver, char *message) {

  
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (strcmp(clients[i].username, receiver) == 0) {
      //printf("Checkpoint2\n");
      addMessage(sender, receiver, message, i);

  sleep(0.5);

  //printf("Checkpoint3\n");
  // Marcheaza mesajul ca citit in baza de date
  sqlite3 *db;
  char *err_msg = 0;

  printf("%s %s %s\n", sender, receiver, message);

  // Deschiderea bazei de date
  int rc = sqlite3_open("messages.db", &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  // Actualizarea tabelului "messages" pentru a marca mesajul ca citit
  char *sql = "UPDATE messages SET read = 1 WHERE sender = ? AND receiver = ? AND message = ?;";
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }
  sqlite3_bind_text(stmt, 1, sender, strlen(sender), 0);
  sqlite3_bind_text(stmt, 2, receiver, strlen(receiver), 0);
  sqlite3_bind_text(stmt, 3, message, strlen(message), 0);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Eroare la actualizarea tabelului: %s\n", sqlite3_errmsg(db));
  }
  sqlite3_finalize(stmt);

  // Inchiderea bazei de date
  sqlite3_close(db);
  

      return;
    }
  }
}

// Functie pentru afisarea istoricului conversatiilor pentru un utilizator specificat
void displayHistory(int sock, char *username) {
sqlite3 *db;
char *err_msg = 0;
int rc;

// Deschiderea bazei de date
rc = sqlite3_open("messages.db", &db);
if (rc != SQLITE_OK) {
fprintf(stderr, "Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
sqlite3_close(db);
return;
}

// Construirea interogarii
char *sql = "SELECT * FROM messages WHERE sender = ? OR receiver = ?;";
sqlite3_stmt *stmt;
rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
if (rc != SQLITE_OK) {
fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
sqlite3_close(db);
return;
}
sqlite3_bind_text(stmt, 1, username, strlen(username), 0);
sqlite3_bind_text(stmt, 2, username, strlen(username), 0);

// Trimiterea rezultatelor prin socketul asociat
send(sock, "Istoricul conversatiilor:\n", 26, 0);
while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
int id = sqlite3_column_int(stmt, 0);
char *sender = (char *)sqlite3_column_text(stmt, 1);
char *receiver = (char *)sqlite3_column_text(stmt, 2);
char *message = (char *)sqlite3_column_text(stmt, 3);
char history[MAX_MESSAGE_LENGTH];
sprintf(history, "%d: %s -> %s: %s\n", id, sender, receiver, message);
send(sock, history, strlen(history), 0);
}
}

// Functie pentru adaugarea unui mesaj in baza de date
void addMessage(char *sender, char *receiver, char *message, int i) {
  printf("Checkpoint0\n");
  sqlite3 *db;
  char *err_msg = 0;

  //printf("%s %s %s %d\n", sender, receiver, message, i);

  // Deschiderea bazei de date
  int rc = sqlite3_open("messages.db", &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  // Crearea tabelului daca nu exista deja
char *sql = "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, sender text, receiver text, message text, read INTEGER DEFAULT 0);";
rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
if (rc != SQLITE_OK) {
fprintf(stderr, "Eroare la crearea tabelului: %s\n", err_msg);
sqlite3_free(err_msg);
sqlite3_close(db);
return;
}


  // Adaugarea mesajului in baza de date
  sql = "INSERT INTO messages (id, sender, receiver, message, read) VALUES (?, ?, ?, ?, 0);";
sqlite3_stmt *stmt;
rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
if (rc != SQLITE_OK) {
fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
sqlite3_close(db);
return;
}
sqlite3_bind_null(stmt, 1);
sqlite3_bind_text(stmt, 2, sender, strlen(sender), 0);
sqlite3_bind_text(stmt, 3, receiver, strlen(receiver), 0);
sqlite3_bind_text(stmt, 4, message, strlen(message), 0);

rc = sqlite3_step(stmt);

sqlite3_int64 row_id = sqlite3_last_insert_rowid(db);
char id_str[MAX_MESSAGE_LENGTH];
sprintf(id_str, "%lld", row_id);

if (rc != SQLITE_DONE) {
fprintf(stderr, "Eroare la adaugarea mesajului: %s\n", sqlite3_errmsg(db));
}
sqlite3_finalize(stmt);
printf("Mesaj adaugat in baza de date\n");

  send(clients[i].sock, sender, strlen(sender), 0);
  send(clients[i].sock, ": ", 2, 0);
  send(clients[i].sock, message, strlen(message), 0);
  send(clients[i].sock, " ", 1, 0);
  send(clients[i].sock, id_str, strlen(id_str), 0);
  send(clients[i].sock, "(ID)", 4, 0);
  send(clients[i].sock, "\n", 1, 0);

// Inchiderea bazei de date
sqlite3_close(db);
}

// Functie pentru citirea mesajelor din baza de date si trimiterea lor catre client
void sendOfflineMessages(int sock, char *username) {
  sqlite3 *db;
  char *err_msg = 0;

  // Deschiderea bazei de date
  int rc = sqlite3_open("messages.db", &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  

  // Selectarea mesajelor care sunt destinate utilizatorului curent si care nu au fost marcate ca citite
  char *sql = "SELECT id, sender, message FROM messages WHERE receiver = ? AND read = 0;";
  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }
  sqlite3_bind_text(stmt, 1, username, strlen(username), 0);
  //printf("Checkpoint\n");
while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {

int id = sqlite3_column_int(stmt, 0);
char id_str[MAX_MESSAGE_LENGTH];
sprintf(id_str, "%d", id);

char* sender = (char*)sqlite3_column_text(stmt, 1);
char* message = (char*)sqlite3_column_text(stmt, 2);


send(sock, sender, strlen(sender), 0);
send(sock, ": ", 2, 0);
send(sock, message, strlen(message), 0);
send(sock, " ", 1, 0);
send(sock, id_str, strlen(id_str), 0);
send(sock, "(ID)", 4, 0);
send(sock, "\n", 1, 0);

}

sqlite3_finalize(stmt);



// Marcarea mesajelor ca citite
sql = "UPDATE messages SET read = 1 WHERE receiver = ? AND read = 0;";
rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
if (rc != SQLITE_OK) {
fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
sqlite3_close(db);
return;
}
sqlite3_bind_text(stmt, 1, username, strlen(username), 0);
rc = sqlite3_step(stmt);
if (rc != SQLITE_DONE) {
fprintf(stderr, "Eroare la actualizarea mesajelor: %s\n", sqlite3_errmsg(db));
}
sqlite3_finalize(stmt);

// Inchiderea bazei de date
sqlite3_close(db);
send(sock, "Mesaje livrate\n", sizeof("Mesaje livrate\n"), 0);
}

// Trimitem raspunsul la un mesaj
void sendReply(char *sender, int reply_id, char *message) {
  // Cauta mesajul original in baza de date
  sqlite3 *db;
  char *err_msg = 0;

  // Deschiderea bazei de date
  int rc = sqlite3_open("messages.db", &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

    // Interogarea tabelului "messages" pentru a gasi mesajul original
  char *sql = "SELECT * FROM messages WHERE id = ?";

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la pregatirea interogarii: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }
  sqlite3_bind_int(stmt, 1, reply_id);

  rc = sqlite3_step(stmt);

  bool ok=0;
  
  char raspuns[MAX_MESSAGE_LENGTH];
  char mor[MAX_MESSAGE_LENGTH];

  if (rc == SQLITE_ROW) {
    // Mesajul original a fost gasit in baza de date
    char *receiver = (char*)sqlite3_column_text(stmt, 1);
    char *text = (char*)sqlite3_column_text(stmt, 3);

    strcpy(mor, receiver);
    
    raspuns[0]=0;
    strcat(raspuns, message);
    strcat(raspuns, " (reply to) ");
    strcat(raspuns, text);

    ok=1;

    //printf("%s\n", raspuns);
  } else {
    // Mesajul original nu a fost gasit in baza de date
    ok=0;
    
  }
  sqlite3_finalize(stmt);
  sqlite3_close(db);

  //printf("%s %s %s\n", sender, mor, raspuns);
  if(ok)
    sendToClient(sender, mor, raspuns);
  else sendToClient("Server", sender, "Mesajul original nu a fost gasit in baza de date");
  printf("Checkpoint1\n");
}


// Functia pentru procesarea conexiunilor in thread-uri separate
void *connectionHandler(void *socket_desc) {
  // Extragerea descriptorului de socket din void*
  int sock = *(int*)socket_desc;
  int read_size;
  char client_message[MAX_MESSAGE_LENGTH];
  char buffer[MAX_MESSAGE_LENGTH];
  char username[MAX_MESSAGE_LENGTH];
  int logged_in = 0;

  // Citirea username-ului de la client

  int available = 1;

  read_size = recv(sock, client_message, MAX_MESSAGE_LENGTH, 0);
  if (read_size > 0) {
    sscanf(client_message, "%s", username);

    printf("%s\n", username);

    // Verificarea daca username-ul este disponibil
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].sock && (strcmp(clients[i].username, username) == 0) ) {
        printf("%s\n", clients[i].username);
        available = 0;
        break;
      }
    }
}

    if (available) {
      // Adaugarea clientului in vectorul de clienti
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == 0) {
          clients[i].sock = sock;
          strcpy(clients[i].username, username);
          logged_in = 1;
          break;
        }
      }

      sprintf(buffer, "Bine ai venit, %s!", username);
      send(sock, buffer, strlen(buffer), 0);
      send(sock, "\n", 1, 0);
    } else {
      send(sock, "Username-ul este deja folosit. Incearca unul nou.", 50, 0);

  }
  // Daca clientul s-a autentificat cu succes, procesarea mesajelor primite
  if (logged_in) {
    //Afisarea mesajelor primite
    sendOfflineMessages(sock, username);


    // Citirea mesajelor de la client si trimiterea lor catre toti ceilalti clienti
    for(int i=0; i<MAX_MESSAGE_LENGTH; ++i)
        client_message[i]=0;
    while ((read_size = recv(sock, client_message, MAX_MESSAGE_LENGTH, 0)) > 0) {
      Message message;


      printf("%s%s\n", "Am primit mesajul: ", client_message);


    for(int i=0; i<MAX_MESSAGE_LENGTH; ++i)
        message.receiver[i]=message.sender[i]=message.message[i]=0;

      strcpy(message.sender, username);
      char operatie[MAX_MESSAGE_LENGTH];
      

      char *p=strtok(client_message, " ");
      strcpy(message.receiver, p);
      //printf("%s %s\n", message.receiver, message.message);



      if(strcmp(message.receiver, "history")==0)
      {
        
          displayHistory(sock, message.sender);
          continue;
        
      }
      else
      {
        p=strtok(NULL, " ");
        if(p==NULL)
        {
          send(sock, "Formatul incorect\n", sizeof("Formatul incorect\n"), 0);
          continue;
        }
        if(strcmp(p, "send")==0)
        {
          //Urmeaza mesajul
          p=strtok(NULL, "");
          strcpy(message.message, p);

          //printf("%s %s %s\n", message.sender, message.receiver, message.message);

          send(sock, "Format corect\n", sizeof("Format corect\n"), 0);
        if (strcmp(message.receiver, "all") == 0) {
        // Trimiterea mesajului catre toti clientii
        broadcast(message.sender, message.message);
      } else {
        // Trimiterea mesajului catre un client specificat
        sendToClient(message.sender, message.receiver, message.message);

        // Daca utilizatorul specificat nu este online, adaugarea mesajului in baza de date pentru a fi trimis mai tarziu
        int online = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (strcmp(clients[i].username, message.receiver) == 0) {
            online = 1;
            break;
          }
        }
        if (!online) {
          addMessage(message.sender, message.receiver, message.message, 13); //13 e predefinit ca un client inexistent
        }
      }
        }
        else if(strcmp(p, "reply")==0)
        {
          int reply_id = atoi(strtok(NULL, " "));
          char *p = strtok(NULL, "");
          strcpy(message.message, p);

          send(sock, "Format corect\n", sizeof("Format corect\n"), 0);
          
          sendReply(message.sender, reply_id, message.message);
        }
        else{
        send(sock, "Formatul incorect\n", sizeof("Formatul incorect\n"), 0);
      }
      }

      for(int i=0; i<MAX_MESSAGE_LENGTH; ++i)
        client_message[i]=0;
    }

    // Daca s-a intrerupt conexiunea cu clientul, stergerea lui din vectorul de clienti
    if (read_size == 0) {
      printf("Clientul s-a deconectat\n");
      fflush(stdout);
    } else if (read_size == -1) {
      perror("Eroare la citirea mesajului de la client");
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clients[i].sock == sock) {
        clients[i].sock = 0;
        strcpy(clients[i].username, "");
        break;
      }
    }
  }

    // Inchiderea conexiunii cu clientul
  close(sock);
  return;
}


int main() {
int sock, new_sock, c;
struct sockaddr_in server, client;
char buffer[MAX_MESSAGE_LENGTH];
int n;

for (int i = 0; i < MAX_CLIENTS; i++)
{
    clients[i].sock=0;
    clients[i].username[0]=0;
}

//Creem baza de date
  sqlite3 *db;
  char *err_msg = 0;

  // Deschiderea bazei de date
  int rc = sqlite3_open("messages.db", &db);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Eroare la deschiderea bazei de date: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }

  // Crearea tabelului daca nu exista deja
char *sql = "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, sender text, receiver text, message text, read INTEGER DEFAULT 0);";
rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
if (rc != SQLITE_OK) {
fprintf(stderr, "Eroare la crearea tabelului: %s\n", err_msg);
sqlite3_free(err_msg);
sqlite3_close(db);
return;
}

// Crearea socketului
sock = socket(AF_INET, SOCK_STREAM, 0);
if (sock == -1) {
perror("Eroare la crearea socketului");
exit(1);
}
printf("Socket creat\n");

// Preluarea informatiilor despre adresa si portul serverului
server.sin_family = AF_INET;
server.sin_addr.s_addr = INADDR_ANY;
server.sin_port = htons(PORT);

// Legarea socketului la adresa si portul specificate
if (bind(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
perror("Eroare la legarea socketului");
exit(1);
}
printf("Socket legat la adresa si portul specificate\n");

// Ascultarea conexiunilor
  listen(sock, 3);
  printf("Ascultarea conexiunilor...\n");
  c = sizeof(struct sockaddr_in);

  // Acceptarea conexiunilor noi si procesarea lor intr-un thread separat
  while ((new_sock = accept(sock, (struct sockaddr*)&client, (socklen_t*)&c))) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, connectionHandler, (void*)&new_sock) < 0) {
      perror("Eroare la crearea thread-ului");
      return 1;
    }
    printf("Handler atribuit\n");
  }
  if (new_sock < 0) {
    perror("Eroare la acceptarea conexiunii");
    return 1;
  }

  return 0;
}


