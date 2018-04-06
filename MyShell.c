#define _BSD_SOURCE
#define MAXBUF 512
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
/*#include <errno.h>*/

int BACK=0;
int REDIRECT = 0;/*for redirection*/

int isWhiteSpace(char *command)/*checks for whitespace*/
{
	while(*command != '\0')
	{
		if(!isspace((unsigned char)*command))
			return 0;
		command++;
	}
	return 1;
}

void isRedirect(char **commands)
{
	int i;
	REDIRECT = 0;
	for(i=0;commands[i] != '\0';i++);
/*	if(i>2)
		printf("is redir: %s\n",commands[i-2]);*/
	if(i>2)
	{
		if(strcmp(commands[i-2],">")==0)/*stdout to file*/
			REDIRECT = 1;
	}
/*	printf("redirect = %d\n",REDIRECT);*/
}

char* getCommand(void)/*reads input to string*/
{
/*	printf("start getcommand\n");*/
	char *command=malloc(sizeof(char) * MAXBUF);/*needed to prevent seg fault*/
	if(command == NULL)
	{
		perror("Error allocating\n");
		exit(-1);
	}
	if(fgets(command,MAXBUF,stdin) == NULL )/*gets the input from stdin and passes string*/
	{
		perror("Error getting stdin\n");
		exit(-1);
	}
	
	/*removes trailing newline from fgets. this method is used cause other was seg faults*/
	char *source, *destination;
	for(source=destination=command;*source !='\0';source++)
	{
		*destination=*source;
		if(*destination != '\n')
			destination++;
	}
	*destination= '\0';

	/*replacing '\t' with a ' '*/
	int i =0;
	while(command[i] != '\0')
	{
		if(command[i]=='\t')
			command[i]=' ';
		i++;
	}

	if(strcmp(command,"\n\0")==0)/*if user just hits enter*/
		return NULL;
	
/*	printf("end getcommand\n");*/
	return command;
}




char** getTokens(char *command)
{	
	char **tokens=malloc(sizeof(char *) * MAXBUF);/*needed to prevent seg fault*/
	if(tokens == NULL)
	{
		perror("Error allocating\n");
		exit(-1);
	}
	int i;	
	for(i=0;i<MAXBUF;i++)
	{
		char *pass = strsep(&command," ");/*not passed to tokes to prevent storing NULL*/
		if(pass==NULL)/*if just enter is pressed go to next prompt*/
                        break;
		tokens[i] = pass;
/*		printf("in get tokens: %s\n",tokens[i]);*/

	}
	return tokens;
}

char** noSpace(char **tokens)
{
	char **nospaceTokens=malloc(sizeof(char *) * MAXBUF);
	if(nospaceTokens == NULL)
        {
                perror("Error allocating\n");
                exit(-1);
        }
	int i;
	int spaces=0;
	for(i=0;tokens[i] != '\0';i++)
	{
		if(isWhiteSpace(tokens[i])!=1)
		{
			nospaceTokens[i-spaces]=tokens[i];
		}
		else
		{
			spaces++;
		}
	}
	return nospaceTokens;
}

void isBackground(char **tokens)/*return 1 if there is a &, 0 if not*/
{	
	BACK=0;
/*	printf("in isBackground\n");*/
	int count=0;/*for how many elements*/
	int i;
	for(i=0;tokens[i] != '\0';i++)/*check the amount of elements*/
		count++;

	if(tokens[count-1][strlen(tokens[count-1])-1] == '&')
	{
		tokens[count-1][strlen(tokens[count-1])-1]='\0';
		BACK=1;
	}
/*	printf("leaving isBackground\n");*/
}

void interactiveMode(void)/*driver for interactive mode*/
{
/*      printf("start interactiveMode\n");*/
        for(;;)
        {
                char *command=malloc(sizeof(char) * MAXBUF);
                if(command == NULL)
                {
                        perror("Error allocating\n");
                        exit(-1);
                }
                char **tokens=malloc(sizeof(char *) * MAXBUF);
                if(tokens == NULL)
                {
                        perror("Error allocating\n");
                        exit(-1);
                }
                char **nospaceTokens=malloc(sizeof(char *) * MAXBUF);
                if(nospaceTokens == NULL)
                {
                        perror("Error allocating\n");
                        exit(-1);
                }


                printf("~");
                command = getCommand();
                if(command == NULL)/*checks if nothing is passed as command*/
                        continue;
                if(isWhiteSpace(command) == 1)/*checks if command is only white space*/
                        continue;
/*              printf("The passed string is:%s\n",command);*/
                tokens = getTokens(command);/*spits by space*/
                nospaceTokens = noSpace(tokens);

                isRedirect(nospaceTokens);

                isBackground(nospaceTokens);
                if(executeCommand(nospaceTokens)==1)
                        exit(0);

/*              int i;
                for(i=0;nospaceTokens[i] != '\0';i++)
                        printf("Token %d is %s\n",i,nospaceTokens[i]);*/

                free(command);/*for dealocating the string in getCommand*/
                free(tokens);/*for dealocating the array in getTokens*/
                free(nospaceTokens);/*for dealocating nospaceTokens*/
        }
/*              printf("End interactivemode\n");*/
}

char* getBASH(FILE* file)
{
/*      printf("start getcommand\n");*/
        char *command=malloc(sizeof(char) * MAXBUF);/*needed to prevent seg fault*/
        if(command == NULL)
        {
                perror("Error allocating\n");
                exit(-1);
        }
        if(fgets(command,MAXBUF,file) == NULL )
                interactiveMode();

        /*removes trailing newline from fgets. this method is used cause other was seg faults*/
        char *source, *destination;
        for(source=destination=command;*source !='\0';source++)
        {
                *destination=*source;
                if(*destination != '\n')
                        destination++;
        }
        *destination= '\0';

        /*replacing '\t' with a ' '*/
        int i =0;
        while(command[i] != '\0')
        {
                if(command[i]=='\t')
                        command[i]=' ';
                i++;
        }

        if(strcmp(command,"\n\0")==0)/*if user just hits enter*/
                return NULL;

/*      printf("end getcommand\n");*/
        return command;
}

int executeCommand(char **tokens)
{
	int barrier =0;
	int EXIT =0;
	if(strcmp(tokens[0],"quit")==0||strcmp(tokens[0],"exit")==0)
	{		
		EXIT =1;
	}

	if(strcmp(tokens[0],"barrier&")==0)
		return 0;
	if(strcmp(tokens[0],"barrier")==0)
                barrier=1;

	pid_t pidCheck;
	pidCheck = fork();
	int status;
	

	if(pidCheck<0)
	{
		perror("Failed to fork\n");
                exit(-1);

	}
	else if(pidCheck==0)/*child*/
	{
		if(barrier ==1)	
			exit(0);
		if(EXIT==1)
			exit(0);
		if(REDIRECT == 1)/*for > redirection*/
		{
			int i;
			for(i=0;tokens[i] != '\0';i++);
			char *path=tokens[i-1];
			tokens[i-1]=NULL;
			tokens[i-2]=NULL;
/*			printf("THIS IS THE FILE: %s\n",path);*/
			int outfd=open(path,O_WRONLY|O_CREAT,0666);
			dup2(outfd,STDOUT_FILENO);
			close(outfd);
			if(execvp(tokens[0],tokens)==-1)
                        {
                                perror("Input Error");
                                exit(-1);
                        }		
		}
		else
		{
	 		if(execvp(tokens[0],tokens)==-1)
                	{
                       		perror("Input Error");
				exit(-1);
			}
                }
	}
	else/*parent*/
	{
		if(BACK==0)/*checks if there is a & sign,if no waits if yes continue*/
		{
/*			printf("waiting\n\n");*/
			waitpid(pidCheck,&status,0);
/*			printf("done waiting\n\n");*/
		}
		if(barrier ==1)
			while((pidCheck = waitpid(-1, &status, 0)) != -1);
		if(EXIT == 1)
		{
			while((pidCheck = waitpid(-1, &status, 0)) != -1);
			return 1;
		}
	}	
	
	return 0;
}

void batchMode(char *path)
{
	FILE* file = fopen(path,"r");
	if(!file)	
	{
		perror("ERROR");
		exit(-1);
		printf("\n");
	}
	else
	{
		for(;;)
	        {
        	        char *command=malloc(sizeof(char) * MAXBUF);
        	        if(command == NULL)
        	        {
        	                perror("Error allocating\n");
                	        exit(-1);
               		}
	                char **tokens=malloc(sizeof(char *) * MAXBUF);
	                if(tokens == NULL)
        	        {
	                        perror("Error allocating\n");
       		                exit(-1);
	                }
        	        char **nospaceTokens=malloc(sizeof(char *) * MAXBUF);
	                if(nospaceTokens == NULL)
        	        {
                	        perror("Error allocating\n");
	                        exit(-1);
	                }

	                command = getBASH(file);
        	        if(command == NULL)/*checks if nothing is passed as command*/
 	                       continue;
	                if(isWhiteSpace(command) == 1)/*checks if command is only white space*/
        	                continue;
/*              printf("The passed string is:%s\n",command);*/
 		     	tokens = getTokens(command);/*spits by space*/
	                nospaceTokens = noSpace(tokens);

        	        isRedirect(nospaceTokens);

	                isBackground(nospaceTokens);
	                if(executeCommand(nospaceTokens)==1)
        	                exit(0);
	
/*              int i;
                for(i=0;nospaceTokens[i] != '\0';i++)
                        printf("Token %d is %s\n",i,nospaceTokens[i]);*/

	                free(command);/*for dealocating the string in getCommand*/
	                free(tokens);/*for dealocating the array in getTokens*/
        	        free(nospaceTokens);/*for dealocating nospaceTokens*/
       		}
	fclose(file);
	}

}

/* --------------------------	MAIN ---------------------------------*/
int main(int argc, char **argv)
{
	if(argc > 2)/*too many arguments when run*/
	{
		printf("ERROR: Too many arguments\n");
		exit(-1);
	}
	else if(argc == 1)/*no arguments*/
		interactiveMode();
	else/*two arguments*/
		batchMode(argv[1]);

	return 0;
} 

