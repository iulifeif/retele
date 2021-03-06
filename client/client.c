#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

void handler(int signo)
{
  return;
}

/* portul de conectare la server*/
int port;

void send_username(int);      //functiile pe care le poate apela clientul
void send_answer(int);
void end_game(int);

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
  int nr=0;
  char buf[10];

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

  int running=1;
  while(running)
  {
    int cmd;
    if(-1 == read(sd,&cmd,sizeof(int)))
    {
      printf("%s",strerror(errno));
      exit(errno);
    }
    switch (cmd)
    {
    case 1:
      send_username(sd);
      break;
    case 2:
      send_answer(sd);
      break;
    case 3:
      end_game(sd);
      running=0;
      break;

    default:
      running=0;
      break;
    }
  }
  close(sd);
}

void send_username(int sd)      //initializeaza user-ul
{
  char username[100];
  printf("Introdu un username: ");
  scanf("%s",username);
  int username_length = strlen(username)+1;
  write(sd,&username_length,sizeof(int));
  write(sd,username,username_length);
}

void send_answer(int sd)      //desfasoara sesiunea de joc
{
  int answer;
  int question_length;
  char question[500];
  read(sd,&question_length,sizeof(int));
  read(sd,question,question_length);
  printf("%s \n", question);

  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);
  alarm(10);

  if (scanf("%d", &answer) == 1)
  {
    if(answer<1 || answer>3)
      printf("Numarul %d nu este o comanda valida!\nLa aceasta intrebare nu vrei fi punctat!", answer);
    else
      printf("Am primit raspunsul: %d\n", answer);
    alarm(0); // opreste alarma
  }
  else
  {
    printf("S-a scurs timpul!\nLa aceasta intrebare nu vei fi punctat!\n");
    answer = 0;
  }
  write(sd, &answer,sizeof(int));
}

void end_game(int sd)
{
  int scor;
  read(sd,&scor,sizeof(int));
  printf("Jocul s-a sfarsit!\nScorul tau este: %d\n",scor);
}