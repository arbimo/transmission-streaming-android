#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <pthread.h>

#include "eventqueue.h"

int msgcount = 0;
int (*funcptr)(char *msg, int msglen);
int burstlength = 3;
int can_write = 0;

struct msg{
  struct msg *next;
  char *data;
  int len;
} *head= NULL, *tail= NULL;
pthread_mutex_t read_lock;
pthread_cond_t can_read;

int eventqueue_add(char *msg, int len)
{
  struct msg *new = (struct msg *)malloc(sizeof(struct msg));
  if (!new){
    fprintf(stderr, "Failed to allocate memory for new msg");
    return -1;
  }
  memset(new, 0, sizeof(*new));
  new->data = malloc(len);

  if (!new->data) {
    fprintf(stderr, "Failed to allocate memory for new msg");
    return -1;
  } 
  memcpy(new->data,  msg, len);
  new->len = len;

  pthread_mutex_lock(&read_lock);

  if (tail) {
    tail->next = new;
    tail = new;
  }
  else // If tail is not set then head is null as well
    head=tail=new;
  msgcount++;
  // printf("Added message %s\n", new->data);
  //printf("Message count now %d and burst threshold %d\n", msgcount, burstlength);
  if (msgcount >= burstlength && can_write){
    //printf("Trigger server thread\n");
    pthread_cond_signal(&can_read);
    sched_yield();
  }
  pthread_mutex_unlock(&read_lock);


  return 0;
}

void eventqueue_can_pop()
{

  pthread_mutex_lock(&read_lock);
  can_write = 1;
  //printf("eventqueue_can_write: wake up server thread\n");
  pthread_cond_signal(&can_read);
  sched_yield();
  pthread_mutex_unlock(&read_lock);

}

static int pop_all()
{
  while (head != NULL) {
	  struct msg *curr = head;
	  int success = funcptr(curr->data, curr->len);
	  if (success <= 0) {
		  /* Write failed, stop writing from queue */
		  fprintf(stderr,"ERROR: Write failed\n");
		  can_write = 0;
		  return -1;
	  }
	  head = head->next;
	  free(curr->data);
	  free(curr);
	  msgcount--;
	  //printf("Message count now %d\n", msgcount);
  }
  if (msgcount == 0)
	  tail = NULL;
  return 0;
}

int eventque_stop = 0;
static void* pop_msgs(void * arg)
{
	int *aptr = (int *)arg;
	if (aptr && *aptr == 1)
		can_write = 1;
	printf("Server thread started\n");

	eventque_stop = 0;

	while(!eventque_stop){
		pthread_mutex_lock(&read_lock);
		//	printf("Wait for condition\n");
		while((msgcount==0 || !can_write) && !eventque_stop){
			pthread_cond_wait(&can_read, &read_lock);
			//printf("Got condition signal\n");
		}
		if (pop_all())
			eventque_stop = 1;
  
		pthread_mutex_unlock(&read_lock);
	}
	printf("Exit thread\n");
	
	return NULL;
}

pthread_t server_thread;
void eventqueue_init(int (*fptr)(char *msg, int msglen), int msg_burst_len)
{
	funcptr = fptr;
	burstlength = msg_burst_len;
	printf("eventqueue_init , burst lenght %d\n", msg_burst_len);
	pthread_mutex_init(&read_lock, NULL);
	pthread_cond_init(&can_read, NULL);
}
 
pthread_t * eventqueue_threaded_init(int (*fptr)(char *msg, int msglen), int msg_burst_len)
{
  pthread_attr_t attrs;
  pthread_attr_init(&attrs);
  eventqueue_init(fptr, msg_burst_len);
  pthread_create(&server_thread, &attrs, pop_msgs, NULL);
  sleep(1);
  return &server_thread;
}
void eventqueue_run()
{
	int arg = 1;
	pop_msgs(&arg);
}
void eventqueue_shutdown()
{
  eventque_stop = 1;
//pthread_mutex_lock(&read_lock);
  eventque_stop = 1;
  pthread_cond_signal(&can_read);
  //pthread_mutex_unlock(&read_lock);
  fprintf(stderr,"eventqueue_shutdown() : setting eventque_stop = 1, thus eventqueue will stop in a short while \n");
}

#ifdef TEST_EVENTQ
// For testing the queue compile it 
// gcc eventqueue.c -lpthread -Wall -DTEST_EVENTQ
#include <assert.h>
int testvar = 0;
int msg_write(char *msg, int strlen)
{
  printf("Popped Message %s\n", msg);
  if (testvar == 1) {
    printf("\nSTOP\n");
    testvar = 0;
    return -1;
  }
  return 1;
}
int main(void)
{
  char test1[12] = "Teststring1";
  char test2[13] = "Teststring2";
  char test3[12] = "Teststring3";
  int i;
  pthread_t *sthread;
  sthread = eventqueue_threaded_init(msg_write, 3);
  for (i=0; i<10; i++){
    if (i==4) {
      printf("enable writes\n");
      eventqueue_can_pop();
    }
    eventqueue_add(test1, 12);
    eventqueue_add(test2, 13);
    eventqueue_add(test3, 12);
    sleep(1);
    if (i == 2) {
      printf("Stopping writes\n");
      testvar = 1;
    }
    eventqueue_add(test1, 12);
    eventqueue_add(test1, 12);
    eventqueue_add(test1, 12);
    eventqueue_add(test1, 12);
    eventqueue_add(test1, 12);
    eventqueue_can_pop();
    sleep(1);
  }
  printf("\nend\n");
  assert(msgcount==0);
  assert(head == NULL);
  assert(tail == NULL);

  //pthread_join(*sthread, NULL);
  eventqueue_shutdown();

    

  return 0;
}
#endif
