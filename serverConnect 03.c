#include"simpleServer.h"

# define maxRequest 3

FILE* open; //dictionary file is global
FILE* logfile; // log file
// -----------------------------------gloabal variables
int numClients=0;
int requests[maxRequest]; // client array
int rPutPtr=0; // put clients pointer
int rTakePtr=0;// take client pointer
char logReports[maxRequest];// results to log
int lPutPtr=0; // put log pointer
int lTakePtr=0; // take log pointer

pthread_t workers[maxRequest];
pthread_mutex_t clientLock;
pthread_cond_t consumedOne;
pthread_cond_t alo; //at least one client
pthread_mutex_t logLock;
//---------------------------------------------------------------

void produceClient(int clientSocket)
{
    printf("in produce\n");
    requests[rPutPtr] = clientSocket;
    rPutPtr = (rPutPtr + 1) % maxRequest;
    numClients = numClients +1;
    printf("numClients= %d\n", numClients);
    printf("client put ptr = %d\n",rPutPtr);
    printf("out of produce\n");
}

int consumeClient ()  // this needs to be down while holding the lock
{
    printf("in consume\n");
    int socket = requests[rTakePtr];
    requests[rTakePtr] = 0; // clear taken job
    rTakePtr = (rTakePtr + 1) % maxRequest; // move pointer to next client (circular)
    printf("client consume ptr = %d\n",rTakePtr);
    numClients = numClients - 1 ;
    printf("numClients= %d\n", numClients);
    printf("out of it\n");
    return socket;
}

void *worker_Behaviour (void *args)
{


    char* welcome = "Welcome to dope boy servers\n";
    char* demand = "Gimme a word and I'll see if you can spell\n";
    char* prompt = "I want to look up : ";
    char* terminate = "Kicking you off the server\n";
    char* right = "Spelt correct\n";
    char* wrong = "Spelt wrong\n";
    char* serSees = "You said ";
    char* error = "Something went wrong with the input, try again and dont mess it up";

    while(1)  // i hope this loop keeps the thread alive to consume next job.. IT DID!!
    {

        pthread_mutex_lock(&clientLock);
        while(numClients == 0 && requests[rTakePtr] == 0)  // if know clients wait for full signal
        {
            pthread_cond_wait(&alo,&clientLock);
        }
        int socket = consumeClient();
        pthread_cond_signal(&consumedOne);
        pthread_mutex_unlock(&clientLock);
        int input;
        char recvBuffer[BUF_LEN];
        char compare[BUF_LEN];
        recvBuffer[0] = '\0';
        send(socket, welcome, strlen(welcome),0);
        send(socket, demand, strlen(demand),0);

        while(1) // on the job behaviour
        {
            fseek(open,0,SEEK_SET);
            send(socket, prompt, strlen(prompt),0);
            input = recv(socket,recvBuffer,BUF_LEN,0);

            size_t replace = strlen(recvBuffer); // store an unknown size
            recvBuffer[replace-2] = '\n';
            recvBuffer[replace-1] = '\0';

            if(input==-1)
            {
                send(socket,error,strlen(error),0);
            }
            else if(recvBuffer[0]== 27)
            {
                send(socket, terminate, strlen(terminate),0);
                close(socket);
                break;
            }
            else
            {

                send(socket, serSees, strlen(serSees), 0);
                send(socket, recvBuffer, input, 0);

                fgets(compare,sizeof(compare),open);

                while(1)
                {
                    if(strcmp(compare,recvBuffer)== 0)
                    {
                        send(socket, right, strlen(right),0);
                        fprintf(logfile,"%s",recvBuffer);
                        fprintf(logfile,"%s\n",right);

                        break;
                    }
                    fgets(compare,sizeof(compare),open);
                    if(feof(open))
                    {
                        send(socket, wrong, strlen(wrong),0);
                        fprintf(logfile,"%s\n",recvBuffer);
                        fprintf(logfile,"%s\n",wrong);

                        break;
                    }
                }
                memset(recvBuffer,'\0', BUF_LEN);
            }


        }
        // next step => compare words for client and pass to logger array
    }
}
//----------------------------------------------------------
void *createLog (void *args)
{
//get lock
// put to array
// move put ptr
// release lock
}


void *removeLog (void *args){

//get lock
// consume from log array
// move take ptr
// release lock

}

//--------------------------------------------------------------


int main(int argc, char** argv)
{

    if(argc == 1)
    {
        printf("No port number entered\n");
        return -1;
    }

    if(argv[2] == NULL)
    {
        open = fopen("words","r");

    }
    else
    {

        open = fopen(argv[2],"r");
    }
    logfile = fopen("log.txt","w");
    if(open == NULL || logfile == NULL)
    {

        printf("Dictionary file not open, quitting");
        return -1;

    }

    struct sockaddr_in client;
    int clientLen = sizeof(client);
    int connectionPort = atoi(argv[1]);
    int connectionSocket,clientSock;

    //ports below 1024 and above 65535 don't exist.
    if(connectionPort < 1024 || connectionPort > 65535)
    {
        printf("Port number is either too low(below 1024), or too high(above 65535).\n");
        return -1;
    }

    connectionSocket = open_listenfd(connectionPort);
    if(connectionSocket == -1)
    {
        printf("Could not connect to %s, maybe try another port number?\n", argv[1]);
        return -1;
    }

//-----------------------------------------------------------------------
// create thread workers and a logger that should go to sleep
    for(int i= 0; i< maxRequest;)
    {

        pthread_create(&workers[i], NULL,worker_Behaviour,NULL);// id,something, function, what to pass to thread
        i=i+1;
    }

//create log threads

//----------------------------------------------

    while(1)  //main thread accepts clients
    {


        if((clientSock = accept(connectionSocket, (struct sockaddr*)&client, &clientLen)) == -1)
        {
            printf("Error connecting to client.\n");
            return -1;
        }
        printf("Connection success!\n");
        pthread_mutex_lock(&clientLock); // get clientQ lock

        while(numClients == maxRequest-1)
        {
            printf("main waiting");
            pthread_cond_wait(&consumedOne,&clientLock); // wait for empty signal
        }
        produceClient(clientSock); // put client into Q
        pthread_cond_signal(&alo);
        pthread_mutex_unlock(&clientLock); // free clientQ

    }


    fclose(open);
    fclose(logfile);
    close(clientSock);
    return 0;
}
