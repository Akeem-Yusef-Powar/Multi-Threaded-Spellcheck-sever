#include"simpleServer.h"

# define maxRequest 3

FILE* open; //dictionary file is global
FILE* logfile; // log file
// -----------------------------------global variables
int numClients=0;
int requests[maxRequest]; // client array
int rPutPtr=0; // put clients pointer
int rTakePtr=0;// take client pointer
char *logReports[maxRequest];// results to log
int correctness[maxRequest];// indicates if log = right/wrong
int numLogs = 0;
int lPutPtr=0; // put log pointer
int lTakePtr=0; // take log pointer

pthread_t workers[maxRequest];
pthread_mutex_t clientLock;
pthread_cond_t consumedOne;
pthread_cond_t producedOne; //at least one client
pthread_t logger;
pthread_mutex_t logLock;
pthread_cond_t producedLog;
pthread_cond_t consumedLog;
//---------------------------------------------------------------

void loggerWorking() //  help see how server is behaving
{
    printf("logger is done\n");
}

void produceClient(int clientSocket)
{
    printf("in produce\n");//  help see how server is behaving
    requests[rPutPtr] = clientSocket;
    rPutPtr = (rPutPtr + 1) % maxRequest;
    numClients = numClients +1;
    printf("numClients= %d\n", numClients);
    printf("client put ptr = %d\n",rPutPtr);
    printf("out of produce\n");
}

int consumeClient ()  // this needs to be down while holding the lock
{
    printf("in consume\n");//  help see how server is behaving
    int socket = requests[rTakePtr];
    requests[rTakePtr] = 0; // clear taken job
    rTakePtr = (rTakePtr + 1) % maxRequest; // move pointer to next client (circular)
    printf("client consume ptr = %d\n",rTakePtr);
    numClients = numClients - 1 ;
    printf("numClients= %d\n", numClients);
    printf("out of consume\n");
    return socket;
}
void createLogEntry (char *what, int correctnesslocal)
{
    char placeholder[BUF_LEN];
    memset(placeholder,'\0',1); // clear the buffer

    printf("creating log with %d\n",correctnesslocal);//  help see how server is behaving
    strcat(placeholder,what);
    logReports[lPutPtr] = malloc(sizeof(placeholder)); // this is why i was getting a seg fault, i needed to malloc the space for each report
    strcpy(logReports[lPutPtr],placeholder);
    correctness[lPutPtr] = correctnesslocal;
    lPutPtr = (lPutPtr + 1) % maxRequest;
    numLogs = numLogs +1;

    printf("log created, numlogs = %d\n ",numLogs);

}

//-------------------------------------------------------------
void *worker_Behaviour (void *args)
{
    char* welcome = "Welcome to dope boy servers\n";
    char* demand = "Gimme a word and I'll see if you can spell\n";
    char* prompt = "I want to look up : ";
    char* terminate = "Kicking you off the server\n";
    char* right = "Spelt correct\n";
    char* wrong = "Spelt wrong\n";
    char* serSees = "You said ";
    char* error = "Something went wrong with the input, try again and don't mess it up";

    while(1)  // i hope this loop keeps the thread alive to consume next job.. IT DID!!
    {

        pthread_mutex_lock(&clientLock);
        while(numClients == 0 && rTakePtr == rPutPtr)  // if know clients wait for full signal
        {
            pthread_cond_wait(&producedOne,&clientLock);
        }
        int socket = consumeClient();
        pthread_cond_signal(&consumedOne);
        pthread_mutex_unlock(&clientLock);
        int input;
        char recvBuffer[BUF_LEN];
        char *transfer[BUF_LEN];
        char compare[BUF_LEN];
        recvBuffer[0] = '\0';
        send(socket, welcome, strlen(welcome),0);
        send(socket, demand, strlen(demand),0);
        while(1) // on the job behaviour
        {
	    int correct = 0; //assume wrong, 1 is right, maybe should be boolean

            fseek(open,0,SEEK_SET);
            send(socket, prompt, strlen(prompt),0);
            input = recv(socket,recvBuffer,BUF_LEN,0);

            size_t replace = strlen(recvBuffer); // store an unknown size
            recvBuffer[replace-2] = '\n';
            recvBuffer[replace-1] = '\0';

            if(input== -1)
            {
                send(socket,error,strlen(error),0);
		break;

            }
            if(recvBuffer[0]== 27)
            {
                send(socket, terminate, strlen(terminate),0);
                close(socket);
                break;
            }
            if(recvBuffer[0]== 63)  // ? is the quit character
            {
                exit(0);

            }

            send(socket, serSees, strlen(serSees), 0);
            send(socket, recvBuffer, input, 0);
            fgets(compare,sizeof(compare),open);

            while(1)
            {
                if(strcmp(compare,recvBuffer)== 0)
                {
                    send(socket, right, strlen(right),0);
                    correct = 1;
                    break;
                }
                fgets(compare,sizeof(compare),open);
                if(feof(open))
                {
                    send(socket, wrong, strlen(wrong),0);
                    break;
                }
            }

            pthread_mutex_lock(&logLock);// get lock

            while(numLogs == maxRequest && lPutPtr==lTakePtr) // if LogQ is full && nothing has been taken since it was filled
            {
                pthread_cond_wait(&consumedLog,&logLock); // wait for consumed signal
            }

            createLogEntry(recvBuffer,correct); // print to file and increment file

            pthread_cond_signal(&producedLog); // signal that a job was produced

            pthread_mutex_unlock(&logLock); // release lock

            memset(recvBuffer,'\0', BUF_LEN); // clear buffer for house keeping
        }
    } // pull job while loop
}


//----------------------------------------------------------

void *logger_Behaviour (void *args)
{

    char zero[15] = "Spelt Wrong\n";
    char one[15] = "Spelt Correct\n";

    while(1) // keep looger pulling jobs
    {
        pthread_mutex_lock(&logLock);

        while(numLogs == maxRequest || lTakePtr == lPutPtr)
        {
            pthread_cond_wait(&producedLog,&logLock);
        }
        fprintf(logfile,"%s",logReports[lTakePtr]);

        if(correctness[lTakePtr]== 1)
        {
            fprintf(logfile,"%s\n",one);

        }
        else
        {

            fprintf(logfile,"%s\n",zero);

        }
        loggerWorking();
        correctness[lTakePtr] = 0;
        lTakePtr = (lTakePtr + 1) % maxRequest;
        numLogs = numLogs-1;
        pthread_mutex_unlock(&logLock);
    }
}

//--------------------------------------------------------------


int main(int argc, char** argv)
{
    int connectionPort;

    if(argc == 1)
    {
        connectionPort = 1234;
    }
    else
    {

        connectionPort = atoi(argv[1]);
    }

    if(argc <= 2) // argc = 2 if only port number was given
    {
        open = fopen("words","r");

    }
    else {
	open = fopen(argv[2],"r");
    	}

    logfile = fopen("log.txt","w");

    if(open == NULL || logfile == NULL)
    {

        printf("Dictionary file not open, quitting\n");
        return -1;

    }

    struct sockaddr_in client;
    int clientLen = sizeof(client);
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

    pthread_create(&logger,NULL,logger_Behaviour,NULL);

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

        while(numClients == maxRequest-1 && rPutPtr == rTakePtr)
        {
            printf("main waiting");
            pthread_cond_wait(&consumedOne,&clientLock); // wait for empty signal
        }
        produceClient(clientSock); // put client into Q
        pthread_cond_signal(&producedOne);
        pthread_mutex_unlock(&clientLock); // free clientQ

    }
    for(int i=0; i<maxRequest;)
    {

        pthread_join(workers[i],NULL);
        i=i+1;
    }
// on exit
    fclose(open);
    fclose(logfile);
    close(clientSock);
    return 0;
}
