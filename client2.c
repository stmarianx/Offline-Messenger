#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


#define PORT 5000
#define MAX_MESSAGE_LENGTH 1024

int main(int argc, char *argv[]) {
  int sock, read_size;
  struct sockaddr_in server;
  char message[MAX_MESSAGE_LENGTH];
  char server_reply[MAX_MESSAGE_LENGTH];


  // Crearea socketului
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    perror("Eroare la crearea socketului");
    exit(1);
  }
  printf("Socket creat\n");

  // Preluarea informatiilor despre adresa si portul serverului
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);

  // Conectarea la server
  if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
    perror("Eroare la conectarea la server");
exit(1);
}
printf("Conectat la server\n");

  // Citirea si trimiterea username-ului la server
  printf("Introdu username-ul: ");
  fgets(message, MAX_MESSAGE_LENGTH, stdin);
  message[strcspn(message, "\n")] = '\0';   // Replace newline with null terminator
  if (send(sock, message, strlen(message), 0) < 0) {
    printf("Eroare la trimiterea username-ului\n");
    exit(1);
  }

for(int i=0; i<MAX_MESSAGE_LENGTH; ++i)
    server_reply[i]=0;

// Citirea raspunsului de la server
if ((read_size = recv(sock, server_reply, MAX_MESSAGE_LENGTH, 0)) < 0) {
printf("Eroare la primirea raspunsului de la server\n");
exit(1);
}
server_reply[read_size] = '\0';
printf("%s\n", server_reply);

// Daca s-a autentificat cu succes, citirea si trimiterea mesajelor la server
if (strstr(server_reply, "Bine ai venit")) {
//printf("Succes\n");

//Verificam daca avem vreun mesaj de la server
sleep(1);
for(int i=0; i<MAX_MESSAGE_LENGTH; ++i)
    server_reply[i]=0;
  if ((read_size = recv(sock, server_reply, MAX_MESSAGE_LENGTH, 0)) < 0) {
printf("Eroare la primirea raspunsului de la server\n");
exit(1);
}
printf("%s\n", server_reply);

while (1) {

    


printf("Mesajul tau: ");

  // Check if input is available
  if (feof(stdin)) {
    break;
  }
  
  for(int i=0; i<MAX_MESSAGE_LENGTH; ++i)
    message[i]=0;
    

  fgets(message, MAX_MESSAGE_LENGTH, stdin);
  message[strcspn(message, "\n")] = '\0';   // Replace newline with null terminator

//Verificam daca avem vreun mesaj de la server
  sleep(1);
  fcntl(sock, F_SETFL, O_NONBLOCK);
    char buffer[MAX_MESSAGE_LENGTH];
    for(int i=0; i<MAX_MESSAGE_LENGTH; ++i)
        buffer[i]=0;
    int bytes_read = read(sock, buffer, MAX_MESSAGE_LENGTH);
    if (bytes_read > 0) {
      printf("%s\n", buffer);
    }

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);

  // Trimiterea mesajului la server
  if (send(sock, message, strlen(message), 0) < 0) {
    printf("Eroare la trimiterea mesajului\n");
    exit(1);
}
if(strcmp(message, "history")==0)
    sleep(1);
// Citirea raspunsului de la server
if ((read_size = recv(sock, server_reply, MAX_MESSAGE_LENGTH, 0)) < 0) {
printf("Eroare la primirea raspunsului de la server\n");
exit(1);
}
server_reply[read_size] = '\0';
printf("%s\n", server_reply);
}
}
else printf("Esec\n");
// Inchiderea conexiunii cu serverul
close(sock);
return 0;
}
