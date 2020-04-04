#include"simpleServer.h"

# define maxRequest 3

FILE* open; //dictionary file is global
FILE* log;

int requests[maxRequest]; // client array
char logReports[maxRequests]// results to log

int main(int argc, char** argv)
{

    if(argc == 1)
    {
        printf("No port number\n");
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

    if(open == NULL)
    {

        printf("Dictionary file not open, quitting");
        return -1;

    }

    //sockaddr_in holds information about the user connection.
    //We don't need it, but it needs to be passed into accept().
    struct sockaddr_in client;
    int clientLen = sizeof(client);
    int connectionPort = atoi(argv[1]);
    int connectionSocket, clientSocket, input;
    char recvBuffer[BUF_LEN];
    char compare[BUF_LEN];
    recvBuffer[0] = '\0';

    connectionPort = atoi(argv[1]);

    //ports below 1024 and above 65535 don't exist.
    if(connectionPort < 1024 || connectionPort > 65535)
    {
        printf("Port number is either too low(below 1024), or too high(above 65535).\n");
        return -1;
    }

    //Does all the hard work for us.
    connectionSocket = open_listenfd(connectionPort);
    if(connectionSocket == -1)
    {
        printf("Could not connect to %s, maybe try another port number?\n", argv[1]);
        return -1;
    }

    //accept() waits until a user connects to the server, writing information about that server
    //into the sockaddr_in client.
    //If the connection is successful, we obtain A SECOND socket descriptor.
    //There are two socket descriptors being used now:
    //One by the server to listen for incoming connections.
    //The second that was just created that will be used to communicate with
    //the connected user.
    if((clientSocket = accept(connectionSocket, (struct sockaddr*)&client, &clientLen)) == -1)
    {
        printf("Error connecting to client.\n");
        return -1;
    }

    printf("Connection success!\n");

//------------------------------------------------------------------------------------ user connected, now par them off on a thread





    char* welcome = "Welcome to dope boy servers\n";
    char* demand = "Gimme a word and I'll see if you can spell\n";
    char* prompt = "I want to look up : ";
    char* terminate = "Kicking you off the server\n";
    char* right = "Spelt correct\n";
    char* wrong = "Spelt wrong\n";
    char* serSees = "You said ";
    char* error = "Something went wrong with the input, try try not messing up this time";

    send(clientSocket, welcome, strlen(welcome),0);
    send(clientSocket, demand, strlen(demand),0);

    while(1) // will run forever given the opportunity
    {
        fseek(open,0,SEEK_SET); // reset file point to beginning
        send(clientSocket, prompt, strlen(prompt),0);
        input = recv(clientSocket,recvBuffer,BUF_LEN,0);

        if(input==-1) // no bytes revived
        {
            send(clientSocket,error,strlen(error),0);
        }
        else if(recvBuffer[0]== 27) // escape key
        {
            send(clientSocket, terminate, strlen(terminate),0);
            close(clientSocket);
            break;
        }
        else
        {

            send(clientSocket, serSees, strlen(serSees), 0);
            send(clientSocket, recvBuffer, input, 0); // what was entered

            fgets(compare,sizeof(compare),open);

            while(1) //go thought file until either match or EOF
            {
                printf("%d\n",strcmp(compare,recvBuffer));
                if(strcmp(compare,recvBuffer)== -3) // for some reason this is not 0, it matches at -3
                {
                    send(clientSocket, right, strlen(right),0);
                    break;
                }
                fgets(compare,sizeof(compare),open);
                if(feof(open))
                {
                    send(clientSocket, wrong, strlen(wrong),0);
                    break;
                }

                send(clientSocket,compare,strlen(compare),0);
            }

        } // server else


    }
    return 0;
}
