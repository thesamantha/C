#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/syscall.h>

#include "resources.h"
#include "deadlock.h"
#include "print.h"

				/** Simulation of Banker's Algorithm **/
/** Detects deadlock given sequence of process execution **/
/** Samantha Tite-Webber, 2015 **/

const char LABEL[] = "ABCD";

Matrix currentNeeds;

void init_globals(){

  /* initialize resource state */
  Matrix tmp;
  tmp.thread[T1] = (Vector) {{1,1,1,2}};
  tmp.thread[T2] = (Vector) {{3,3,0,0}};
  tmp.thread[T3] = (Vector) {{3,0,0,0}};
  tmp.thread[T4] = (Vector) {{0,3,3,0}};
  g_state.G = tmp;
  g_state.v = (Vector) {{3,3,3,3}};
  g_state.f = g_state.v;
  g_state.R = g_state.G;

	//create Belegte Matrix, all the processes currently have nothing (we know this because f = v and G = R)
	  Matrix tmp_B;
	  tmp_B.thread[T1] = (Vector) {{0,0,0,0}};
	  tmp_B.thread[T2] = (Vector) {{0,0,0,0}};
	  tmp_B.thread[T3] = (Vector) {{0,0,0,0}};
	  tmp_B.thread[T4] = (Vector) {{0,0,0,0}};

	g_state.B = tmp_B;

  /* initialize mutexes/signals */
  for(unsigned r=FIRST_RESOURCE; r<NUM_RESOURCES; r++){
    if( pthread_cond_init (&(g_state.resource_released[r]), NULL) ){
      handle_error("cond_init");
    }
  }
  if( pthread_mutex_init(&(g_state.mutex), NULL) ){
    handle_error("mutex_init");
  }
  if( pthread_mutex_init(&g_cfd_mutex, NULL) ){
    handle_error("mutex_init");
  }   

  g_checkForDeadlocks = false;
  # if DETECTION
  g_checkForDeadlocks = true;
  # endif
}

bool vectorEmpty(Vector vector) {

	for(int i = 0; i < NUM_THREADS; i++) {

		if(vector.resource[i] != 0) return false;
	}

	return true;
}

bool allLessEqual(Vector shouldBeLess, Vector shouldBeMore) {

	for(int j = 0; j < NUM_RESOURCES; j++) {

		if(shouldBeLess.resource[j] > shouldBeMore.resource[j]) return false;
	}

	return true;

}

void addVectors(Vector *v1, Vector *v2) {
	
	for(int i = 0; i < NUM_RESOURCES; i++) {

		v1->resource[i] = v1->resource[i] + v2->resource[i];
	}
}

bool isSafe(unsigned t, unsigned r, unsigned a){

  	bool answer = UNDEFINED;
	int allocationOk = 0;

	Vector f_copy = g_state.f;

	char out[57];
	sprintf(out, "[%d] T%u: is \"allocate(%c, %u)\" safe?",
		  gettid(), t+1, LABEL[r], a);

	//check if there is enough of resource 'r' available to grant allocation 'a' to thread 't'
	if(g_state.f.resource[r] >= a) {

		allocationOk = 1;
		//make temporary allocation for testing purposes
		f_copy.resource[r] = f_copy.resource[r] - a;
	}

	else {
		answer = UNSAFE;
		return answer;
	}

# if AVOIDANCE

	int answerInt = -1;		//"undefined"
	//int sequenceHolder = 0;
	Vector done = {{4}};

	for(int i = 0; i < 4; i++) {

			done.resource[i] = false;
	}
	

	while(allocationOk && answerInt == -1) {


		for(int i = 0; i < 4; i++) {

			if(done.resource[i] == false) {

				if(allLessEqual(g_state.R.thread[i], f_copy)) {
			
					addVectors(&f_copy, &g_state.B.thread[i]);
					done.resource[i] = true;
				}
			}

			else {

				for(int i = 0; i < 4; i++) {

					if(done.resource[i] == false) {

						answerInt = 0;
						goto LABEL;
					}
				}

				answerInt = 1;
			}
		}	
	}
	
#endif
  LABEL:;
  char tmp[60];
  sprintf(tmp, "%s : %s\n", out, answer? "yes" : "no" );
  printc(tmp, t);
  
  #ifdef DEBUG
  print_State();
  #endif

  return answer;
}

bool isDeadlocked(unsigned t){

  Vector f_copy = g_state.f;
  bool answer = UNDEFINED;
  char out[57];
  sprintf(out, "[%d] T%u: Deadlock detected?", gettid(), t+1);

# if DETECTION

	int answerInt = -1;		//"undefined"
	//int sequenceHolder = 0;
	Vector done = {{4}};

	for(int i = 0; i < 4; i++) {

			done.resource[i] = false;
	}
	

	while(answerInt == -1) {


		for(int i = 0; i < 4; i++) {

			if(done.resource[i] == false) {

				if(allLessEqual(g_state.R.thread[i], f_copy)) {
			
					addVectors(&f_copy, &g_state.B.thread[i]);
					done.resource[i] = true;
				}
			}

			else {

				for(int i = 0; i < 4; i++) {

					if(done.resource[i] == false) {

						answerInt = 0;	//deadlock
						goto LABEL;
					}
				}

				answerInt = 1;
			}
		}	
	}

	if(answerInt) answer = SAFE;
	else if(!answerInt) answer = UNSAFE;
	
#endif
  LABEL:;
  
  char tmp[60];
  sprintf(tmp, "%s : %s\n", out, answer? "no" : "yes" );
  printc(tmp, t);
  
  #ifdef DEBUG
  print_State();
  #endif

  return answer? false : true;
}

void lock_state(unsigned t){
  printd("about to lock state");
  pthread_mutex_lock(&(g_state.mutex));
  printd("state locked");
}

void unlock_state(unsigned t){
  printd("state unlocked");
  pthread_mutex_unlock(&(g_state.mutex));
}


//after an allocation, the process which has allocated should have those allocated resources in its "Belegt"/"Allocated" matrix, and the "free" matrix should be decremented by the same amount
void allocate_r(unsigned t, unsigned r, unsigned a){

  char tmp[50]; 
  struct timespec ts;

  lock_state(t);

  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2;
  int alreadyWaited = 0;

  /* wait if request wasn't granted */
  while( (isSafe(t, r, a) == UNSAFE) ){
    sprintf(tmp, "[%d] T%u: waiting to allocate(%c, %u)\n",
        gettid(), t+1, LABEL[r], a);
    printc(tmp, t);
	
	//fill current needs matrix
	currentNeeds.thread[t].resource[r] = a;
    
    /* first wait is a timed wait */
    if( !alreadyWaited )
      alreadyWaited = pthread_cond_timedwait(&(g_state.resource_released[r]),
        &(g_state.mutex), &ts);
    else
      pthread_cond_wait(&(g_state.resource_released[r]),
        &(g_state.mutex));
  }

	//subtract 'a' amount of resource from the index to which resource r corresponds in the free resource vector
	g_state.f.resource[r] = g_state.f.resource[r] - a;

	//subtract 'a' amount of resource from the index to which resource r corresponds in the Restanforderung matrix
	g_state.R.thread[t].resource[r] = g_state.R.thread[t].resource[r] - a;

	//add 'a' amount of resource to the index in which r resource is stored within the vector for thread 't' in Belegt matrix 'B'
	g_state.B.thread[t].resource[r] = g_state.B.thread[t].resource[r] + a;		
	//Matrix B; /* Belegt - Allocation */
	//Matrix R; /* Restanforderung - Need */
	//Vector f; /* frei - Available */

    printd("%u unit(s) of resource %c allocated", a, LABEL[r]);
  
  sprintf(tmp, "[%d] T%u: allocate(%c, %u)\n", gettid(), t+1,
      LABEL[r], a);
  printc(tmp, t);
  
  #ifdef DEBUG
  print_State();
  #endif

  unlock_state(t);
}

void release_r(unsigned t, unsigned r, unsigned a){

  char tmp[30];
  printd("[%d] T%u: about to release(%c, %u)\n",
      gettid(), t+1, LABEL[r], a);

  lock_state(t);

	//add 'a' amount of resource to the index in which r resource is stored within the 'free' vector
	g_state.f.resource[r] = g_state.f.resource[r] + a;	//(after a thread releases a resource, it is made free again, thus incrementing the free vector by the amount of the resource released)

	//subtract 'a' amount of resource from index for resource 'r' within vector for thread 't' in 'B' matrix
	g_state.B.thread[t].resource[r] = g_state.B.thread[t].resource[r] - a;	//(after a thread releases a resource, its "Belegt" vector is diminished by the amount of that resource it has released)


  printd("%u unit(s) of resource %c released", a, LABEL[r]);

  pthread_cond_signal(&(g_state.resource_released[r]));

  sprintf(tmp, "[%d] T%u: release(%c, %u)\n", getpid(), t+1,
      LABEL[r], a);
  printc(tmp, t);
  
  #ifdef DEBUG
  print_State();
  #endif

  unlock_state(t);
}

void *thread_work(void *thread_number){

  long t = (long)thread_number;
  char tmp[20];

  sprintf(tmp, "[%u] T%ld: started\n", gettid(), t+1);
  printc(tmp, t);
  switch(t) {
    case T1:
      usleep(10000);
      allocate_r(t, A, 1);
      allocate_r(t, D, 2);
      usleep(10000);
      release_r(t, A, 1);
      allocate_r(t, B, 1);
      usleep(10000);
      allocate_r(t, C, 1);
      usleep(50000);
      release_r(t, B, 1);
      usleep(10000);
      release_r(t, C, 1);
      release_r(t, D, 2);
      break;
    case T2:
      usleep(10000);
      allocate_r(t, A, 2);
      allocate_r(t, B, 3);
      usleep(10000);
      allocate_r(t, A, 1);
      usleep(50000);
      release_r(t, A, 3);
      usleep(10000);
      release_r(t, B, 3);          
      break;
    case T3:
      usleep(10000);
      allocate_r(t, A, 3);
      usleep(50000);
      release_r(t, A, 3);
      break;
    case T4:
      usleep(10000);
      allocate_r(t, B, 2);
      allocate_r(t, C, 3);
      usleep(10000);
      allocate_r(t, B, 1);
      usleep(50000);
      release_r(t, B, 3);
      usleep(10000);
      release_r(t, C, 3);
      break;
    case NUM_THREADS:
      /* DL-WatchDog */
      /* poll resource state to check for deadlocks */
      pthread_mutex_lock(&g_cfd_mutex);
      while( g_checkForDeadlocks ){
        pthread_mutex_unlock(&g_cfd_mutex);
        usleep(1000000);
        if( isDeadlocked(t) ){
          char tmp[35];
          sprintf(tmp, "[%d] T%ld: Deadlock detected!\n",
              gettid(), t+1);
          printc(tmp, t);
          pthread_exit((void*)EXIT_FAILURE);
        }
        pthread_mutex_lock(&g_cfd_mutex);
      }
      pthread_mutex_unlock(&g_cfd_mutex);
      break;
    default:
      printf("unexpected!");
      exit(EXIT_FAILURE);
      break;
  }

  pthread_exit(EXIT_SUCCESS);
}

int main(){

  init_globals();

  printf("Total resources are v:\n");
  print_Vector(&(g_state.v), true);
  printf("\n");
  printf("Currently available resources are f:\n");
  print_Vector(&(g_state.f), true);
  printf("\n");
  printf("Maximum needed resources are G:\n");
  print_Matrix(&(g_state.G), true);
  printf("\n");
  printf("Currently allocated resources are B:\n");
  print_Matrix(&(g_state.B), true);
  printf("\n");
  printf("Still needed resources are R:\n");
  print_Matrix(&(g_state.R), true);
  printf("\n");

  fflush(stdout);
  setbuf(stdout, NULL);

  /* spawn threads */
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_t thread[NUM_THREADS+1] = { 0 };
  for(long t=FIRST_THREAD; t<=NUM_THREADS; t++){
    if( pthread_create(&thread[t], &attr, thread_work, (void *)t) ){
      handle_error("create");
    }
  }
  pthread_attr_destroy(&attr);

  /* wait for threads */ 
  void *status;
  unsigned finished = 0;
  for(unsigned t=FIRST_THREAD; t<=NUM_THREADS; t++){
    if( pthread_join(thread[t], &status) ){
      handle_error("join");
    }
    if( status ){
      printf("\n[%d] T%u exited abnormally. Status: %ld\n",
          gettid(), t+1, (long)status);
    } else {
      printf("\n[%d] T%u exited normally.\n", gettid(), t+1);
    }    
    if( t<NUM_THREADS ) finished++;
    /* tell DL-WatchDog to quit*/
    if(finished==NUM_THREADS){
      pthread_mutex_lock(&g_cfd_mutex);
      g_checkForDeadlocks = false;
      pthread_mutex_unlock(&g_cfd_mutex);
    }  
  }

  /* Clean-up */
  if( pthread_mutex_destroy(&g_cfd_mutex) ){
    handle_error("mutex_destroy");
  } 
  if( pthread_mutex_destroy(&(g_state.mutex)) ){
      handle_error("mutex_destroy");
  }
  for(unsigned r=FIRST_RESOURCE; r<NUM_RESOURCES; r++){
    if( pthread_cond_destroy(&(g_state.resource_released[r])) ){
        handle_error("cond_destroy");
    }
  }

  printf("Main thread exited normally.\nFinal resource state:\n");

  print_State();

  exit(EXIT_SUCCESS);
}

