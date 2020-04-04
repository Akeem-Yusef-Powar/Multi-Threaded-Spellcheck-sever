#include <stdio.h>
#include <stdlib.h>
r


int spellCheck(char word[],FILE* open){
//fseek(open,0,SEEK_SET); // words belongs the mains threads, i want each thread to start at the beginning

char compare[20];
fgets(compare,sizeof(compare),open);

while(compare != NULL){

    if(strcmp(compare,word)==0){
            printf("happened\n");
            return 0;
    }

fgets(compare,sizeof(compare),open);

    if(feof(open)){
    return 1;
    }
}


}

int main(){


char test[20];
printf("gimme a word?\n");
scanf("%s",&test);
strcat(test,"\n");
// it's easier to add the newline terminator to the test word
// that it is to take it away from the fgets i read from words
FILE* open=fopen("words","r");
if(open == NULL){

    printf("words not open");
    return -1;
}

//int pthread_create(*worker,&spellCheck);

int result = spellCheck(test,open);

    if(result == 0){
    printf("Spelt correct\n");
        printf("%s\n",test);

    } else {
    printf("Spelt incorrect\n");
    printf("%s\n",test);

    }

}
