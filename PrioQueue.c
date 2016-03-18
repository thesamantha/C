#include "PrioQueue.h"
#include <stdio.h>
#include <stdlib.h>

						/** Implementation of a Priority Queue **/
/** Features deletion, insertion, printing of, and application of arbitrary function on queue. **/
/** Useful for scheduling paradigms: tweak priority to fit principle of schedule; for example, in Multi-Level Feedback, **/
/** priority can be determined by length of job and time spent utilizing CPU **/
/** Samantha Tite-Webber, 2015 **/

typedef struct q_elem_s
{
	int value;		//each element in the priority queue has a value, a priority, and a pointer to
	int priority;		//the next element in the queue
	struct q_elem_s *next;	
} q_elem;

struct PrioQueue
{
	int size;		//how many elements are in the queue
	q_elem *root;		//pointer to the first element in the queue
};

/** Create queue **/
PrioQueue* pqueue_new()
{
	//need to allocate space for one PrioQueue by means of malloc. This space will be as large as one PrioQueue struct. Because the PrioQueue is currently empty, the root will point to nowhere.

	PrioQueue* prioQ = (PrioQueue*)malloc(sizeof(PrioQueue));	//malloc returns a pointer (automatically of type void, to indicate that it points to a region of unknown data type, but in this case the pointer is of type PrioQueue as we've casted it so) which points to a region of memory of size specified in malloc(THIS AREA)
	prioQ->root = NULL;
	return prioQ;
}

/** Delete entire queue **/
void pqueue_free(PrioQueue *queue)
{
	//start at the top of the queue and keep going until we hit an element whose "next" == NULL.
	//at each element, we "delete" it, and once we've deleted the last element, we delete the queue itself, and then free the memory. we delete using the free(memoryAddress) function, which frees a space in memory at the specified address

	//first check if the queue is initialized:
	if(queue == NULL) {
		puts("Given queue is uninitialized.");
	}
	
	else if(queue->root == NULL) {		//because queue is a pointer to a struct, we can access its members using the arrow notation
		puts("Given queue is empty.");
	}
	
	else {
		//we need a current element and a to-delete element. we set the root as the current element, and set the root as the element to-delete element as well, and then set the current element as root->next. so we start with the current element, set its next as the current element, set it as the to-delete element, and then delete it. after we're done, we delete the queue itself.

		q_elem *current = queue->root;
		q_elem *to_delete = NULL;

		//if the queue only had a root, what would happen...? we wouldn't enter the loop, we'd just delete the root and then the queue! perfekt.
		while(current->next != NULL) {		//as soon as current->next equals NULL, we're at the end. we'll be out of the loop and at that point can delete current itself.

			current = current->next;	//move on to the next element
			to_delete = current;		//delete the element which came before the "next" (now the current)
			free(to_delete);	//(well here we actually delete)
		}

		free(current);		//we stopped as soon as we found the element whose next pointed to nothing, so we need to delete that element here
		free(queue);		//now take the queue itself out of memory

	}
}

//** Insert value into queue **/
/** inserted on basis of its priority **/
/** higher priority = higher place in queue **/
int pqueue_offer(PrioQueue *queue, int priority, int value)
{
	//create element to be added
	q_elem *new_guy = (q_elem*)malloc(sizeof(q_elem));
	new_guy->value = value;
	new_guy->priority = priority;
	new_guy->next = NULL;

	//is there yet an initialized root?
	if(queue->root == NULL) {
		//if not, make the element to be added the root
		queue->root = new_guy;
		queue->size++;		//reflect addition of new element
		return queue->root->value;	//return because we're done here
	}
	
	//otherwise....

	if(priority > queue->root->priority) {
		new_guy->next = queue->root;	//point new guy's next at root
		queue->root = new_guy;		//point queue's root at new_guy
		queue->size++;		//reflect addition of new element
		return new_guy->value;
	}
	
	//start with first element in queue
	q_elem *current = queue->root;

	while(current != NULL) {

		//if we've reached the end of the queue
		if(current->next == NULL) {
			
			current->next = new_guy;
			queue->size++;		//reflect addition of new element
			return new_guy->value;
		}
		
		//check if new element should be inserted between two elements
		else if(priority >= current->next->priority) {

			//point new guy's next at element that used to follow current
			new_guy->next = current->next;

			//point current element's next at new guy
			current->next = new_guy;
			queue->size++;		//reflect addition of new element
			return new_guy->value;
		}
		
		else if(priority < current->next->priority) {
			current = current->next;	
		}
	}
	return -1;
}

/** Return value of first element (element with highest priority in the queue, aka root), without deleting it **/
int pqueue_peek(PrioQueue *queue)
{
	if(queue == NULL) {
		printf("Queue does not exist.\n");
		return -1;
	}

	else if(queue->root == NULL) {
		printf("Queue is empty.\n");
		return -1;
	}

	else
		return queue->root->value;
}

/** Return value of last element in the queue **/
int pqueue_get_last(PrioQueue *queue) {

	if(queue == NULL) {
		printf("Queue does not exist.\n");
		return -1;
	}

	else if(queue->root == NULL) {
		printf("Queue is empty.\n");
		return -1;
	}

	else {		//*something* exists
		q_elem* current = queue->root;
		
		while(current->next != NULL) {

			//if we've reached the end of the queue
			if(current->next->next == NULL) {

				int last_val = current->next->value;	
				free(current->next);		//remove last element
				current->next = NULL;	//make element which preceded deleted element the new last element
				queue->size--;		//reflect removal of element
				return last_val;	//return the value of the last element, which we've already removed
			}

			else	//otherwise, continue down the queue
				current = current->next;
		}
	}

	return -1;
}

/** Remove element with highest priority and return its value **/
int pqueue_poll(PrioQueue *queue)
{

	if(queue == NULL) {
		printf("Queue does not exist.\n");
		return -1;
	}

	else if(queue->root == NULL) {
		printf("Queue is empty.\n");
		return -1;
	}

	else {		//*something* exists
		int rootsVal = queue->root->value;	//get root's value so we can return it after deleting root.
		q_elem* deletePtr = queue->root;	//point the queue to the new root, the element that formerly followed root. root is now lost.
		queue->root = queue->root->next;
		free(deletePtr);
		queue->size--;		//reflect removal of element
		return rootsVal;	//return the value of the element with the highest priority, which we've already removed
	}

	return -1;
}

/** Return size of queue **/
int pqueue_size(PrioQueue *queue)
{
	return queue->size;	//was incremented with every addition of an element and decremented with every removal of an element
}

/** Print queue **/
void pqueue_print(PrioQueue *queue)
{
	q_elem* current = queue->root;
	//char string_rep[15];

	while(current != NULL) {

		//if we've reached the end of the queue
		if(current->next == NULL) {
			printf("(%d, %d) ", current->priority, current->value);
			printf("\n");		//print a new line for cleanliness
			return;		//leave because we're done here
		}

		//string_rep = (current->next == NULL) ? current->priority 
		printf("(%d, %d) ", current->priority, current->value);
		current = current->next;
	}
}

/** Print given priority and value in clean format **/
void print(const int *priority, const int *value) {
	printf("(%d, %d)\n", *priority, *value);
}

/* Apply given function to each element in the queue **/
void pqueue_apply(PrioQueue *queue, void (*func)(const int *, const int *))
{
	q_elem *current;
	for (current = queue->root; current != NULL; current = current->next)
	{
		func(&current->priority, &current->value);
	}
}

