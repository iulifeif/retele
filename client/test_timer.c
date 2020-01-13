#include <stdio.h>
#include <signal.h>

void handler(int signo)
{
  return;
}

int main()
{
  int x;
  struct sigaction sa;

  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);

  alarm(5);

  if (scanf("%d", &x) == 1)
  {
    printf("%d\n", x);
    alarm(0); // cancel the alarm
  }
  else
  {
    printf("timedout\n");
  }
  return 0;
}