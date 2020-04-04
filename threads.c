#include <stdio.h>
#include <pthread.h>
# define workerThreads 4

int count[3]={2,5,10,20};
int countptr=0;
pthread_mutex_t lock;

void *testBehaviour (void *args){
    pthread_mutex_lock(&lock); // insure one thread at a time is incrementing ptr

printf("count = %d for thread %lu\n",count[countptr],pthread_self());
countptr=countptr+1;
    pthread_mutex_unlock(&lock);
return NULL;
}

int main(){



pthread_t workers [workerThreads]; // array of worker threads

for(int i= 0; i< workerThreads;){

    pthread_create(&workers[i], NULL,testBehaviour,NULL);// id,something, function, what to pass to thread
    //count = count+1;
    i=i+1;
}

for(int i= 0; i< workerThreads;){

    pthread_join(workers[i], NULL); // wait for them to finish in order
	printf("%lu finished\n",workers[i]);
    i=i+1;
}


}
