#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>
#include "database.h"

/* portul folosit */
#define PORT 2908

typedef struct client_data      //datele despre clienti
{
  char name[100];
  int client_sd;      //sd ul clientului respectiv
  int question_ready_flag;
  int client_registred_flag;
  int answer_ready_flag;
  int game_over_flag;
  char game_winner[100];
  int scor;
  struct question qst;
  int idThread;     //id-ul thread-ului tinut in evidenta 
} client_data;

struct game_data      //datele despre joc
{
  int nr_clients;
  struct client_data *clients_data[100];
  pthread_t clients_threads[100];
};

/* codul de eroare returnat de anumite apeluri */
extern int errno;

static void *thread_loop(void *arg)
{
  struct client_data *cd = (struct client_data *)arg;
  int sd = cd->client_sd;
  int cmd;
  int username_length;
  char username[100];
  cmd = 1;
  write(sd, &cmd, sizeof(int));
  read(sd, &username_length, sizeof(int));
  read(sd, username, username_length);
  strcpy(cd->name, username);
  printf("User inregistrat: %s\n", cd->name);
  fflush(stdout);
  cd->client_registred_flag = 1;
  char buffer[500];
  int raspuns;
  while (!cd->game_over_flag)
  {
    while (!cd->question_ready_flag)
    {
      sleep(1);
      if(cd->game_over_flag)
        break;
    }
    if(cd->game_over_flag)
        break;
    cd->question_ready_flag = 0;
    cmd = 2;
    write(sd, &cmd, sizeof(int));
    sprintf(buffer, "enunt: %s\n1: %s\n2: %s\n3: %s\n Introdu numarul raspunsului:",
            cd->qst.enunt,
            cd->qst.raspuns1,
            cd->qst.raspuns2,
            cd->qst.raspuns3);
    int buffer_len = strlen(buffer) + 1;
    write(sd, &buffer_len, sizeof(int));
    write(sd, buffer, buffer_len);
    read(sd, &raspuns, sizeof(int));      
    if (raspuns == cd->qst.raspuns_corect)
      cd->scor += 10;
    cd->answer_ready_flag = 1;                           //nu i mai mancau dinozaurii
  }

  cmd = 3;
  write(sd, &cmd, sizeof(int));
  write(sd, &cd->scor, sizeof(int));
  cd->game_over_flag = 1;
}

struct game_data wait_clients(struct game_data gd)
{
  struct sockaddr_in server; // structura folosita de server
  struct sockaddr_in from;
  int sd;
  int length = sizeof(from);
  /* crearea unui socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[server]Eroare la socket().\n");
  }
  /* utilizarea optiunii SO_REUSEADDR */
  int on = 1;
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  /* pregatirea structurilor de date */
  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons(PORT);

  /* atasam socketul */
  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[server]Eroare la bind().\n");
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen(sd, 2) == -1)
  {
    perror("[server]Eroare la listen().\n");
  }
  printf("Asteptam sa se conecteze 5 clienti\n");
  fflush(stdout);
  while (gd.nr_clients < 5)
  {
    int client_sd;
    /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
    if ((client_sd = accept(sd, (struct sockaddr *)&from, &length)) < 0)
    {
      perror("[server]Eroare la accept().\n");
      continue;
    }
    printf("Client-ul cu id-ul: %d s-a conectat\n", gd.nr_clients+1);
    fflush(stdout);
    client_data *cl;      //datele despre clientul actual
    cl = (struct client_data *)malloc(sizeof(struct client_data));
    cl->client_sd = client_sd;      //se initializeaza threadurile pentru acesta
    cl->idThread = gd.nr_clients;
    cl->client_registred_flag = 0;
    cl->game_over_flag = 0;
    cl->question_ready_flag = 0;
    cl->answer_ready_flag = 0;
    cl->scor = 0;
    gd.clients_data[gd.nr_clients] = cl;
    pthread_create(
        &(gd.clients_threads[gd.nr_clients++]),
        NULL,
        &thread_loop,
        cl);
  }
  for (int client_id = 0; client_id < gd.nr_clients; client_id++)
  {
    while (!gd.clients_data[client_id]->client_registred_flag)
    {
      sleep(1);
    }
  }
  return gd;
}

void game_loop(struct game_data gd)
{
  sqlite3 *db = open_db();
  for (int qid = 1; qid < 3; qid++)     //////////// numarul de intrebari la care vreau sa raspunda clientii
  {
    struct question q = get_question(db, qid);      //q primeste intrebarile 
    for (int cid = 0; cid < gd.nr_clients; cid++)     //pentru toti clientii 
    {
      gd.clients_data[cid]->qst = q;      //toti clientii primesc intrebarile din baza de date
      gd.clients_data[cid]->question_ready_flag = 1; 
      while (!gd.clients_data[cid]->answer_ready_flag)      //pentru a lua toti userii in ordinea inregistrarii lor
        sleep(1);
      gd.clients_data[cid]->answer_ready_flag = 0;
    }
  }
  int scor_maxim=0;
  int id_scor_maxim=-1;
  for(int cid = 0; cid < gd.nr_clients; cid++)
  {
      if(gd.clients_data[cid]->scor>scor_maxim)     // verific care este cel care are scor ul maxim
      {
        scor_maxim=gd.clients_data[cid]->scor;      //daca sunt mai multi cu acelasi scor,
        id_scor_maxim=cid;       //il iau pe cel care s-a conectat primul
      }
  }
  for(int cid = 0; cid < gd.nr_clients; cid++)
  {
    if(cid == id_scor_maxim) 
      sprintf(gd.clients_data[cid]->game_winner,"Tu esti castigatorul jocului! Felicitari!\n");
    else
      sprintf(gd.clients_data[cid]->game_winner,"Castigatorul este: %s\n",gd.clients_data[id_scor_maxim]->name);
    gd.clients_data[cid]->game_over_flag=1;
  }
  for(int cid = 0; cid < gd.nr_clients; cid++)
  {
    pthread_join(gd.clients_threads[cid],NULL);   /////asteapta ca acel thread sa termine
  }
}



int main()
{
  int desfasurare_joc=1;
  struct game_data gd;
  gd = wait_clients(gd);
  while(desfasurare_joc)
  {
    gd.nr_clients = 0;
    game_loop(gd);
  }
  return 0;
}