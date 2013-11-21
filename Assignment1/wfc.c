#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//
struct word
{
    char *content;
    int number;
};

//check whether an input is a letter
int checkChar(char ch)
{
    if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//check whether a *char is null
int isnull(char *str)
{
    if (*str == '\0')
        return 1;
    return 0;
}

//check whether a *char is a number
int isdigit_all(char *str)
{
    if (isnull(str))
        return 0;
    while (*str != '\0')
    {
        if (!isdigit(*str++))
            return 0;
    }
    return 1;
}

int main (int argc, char **argv)
{
    //variavles for clock
    double  duration;
    clock_t startClock, finishClock;

    //variables for getopt
    int parallelism;
    char *input = NULL;
    char *output = NULL;
    int index;
    int c;
    FILE *fpInput;
    FILE *fpOutput;
    opterr = 0;

    //variables for main function before create children
    int n,i,j,t;
    char temChar;
    int flag,found;
    char **wordsAll;
    char *temWord;
    int line, row;
    struct word *words;
    struct word temWordStruct;
    int status;
    int cProcessN;
    pid_t pid;
    int start,end;
    int separatedWordsRange[cProcessN][2];
    int remainder,range;
    int pfds[2];
    pipe(pfds);
    char *connect1;
    char connect2[120];
    char plusSign[2];

    //variables for children processes
    char frequencyPipe[20];
    char contentPipe[100];

    //variables for parent process
    struct word *wordsParent;
    char content[100];
    char *seperatePipe;

    while ((c = getopt (argc, argv, "p:i:o:")) != -1)
        switch (c)
        {
        case 'p':
            if(isdigit_all(optarg)==1)
            {
                parallelism = atoi(optarg);
            }
            else
            {
                printf("p option need a number!\n");
                return 0;
            }
            break;
        case 'i':
            input = optarg;
            if((fpInput=fopen(input,"r"))==NULL)
            {
                printf("Cannot read the input file: %s !\n",input);
                return 0;
            }
            break;
        case 'o':
            output = optarg;
            if((fpOutput=fopen(output,"wt"))==NULL)
            {
                printf("Cannot create the output file: %s !\n",output);
                return 0;
            }
            break;
        case '?':
            if (optopt == 'p')
            {
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            }
            else if(optopt == 'i')
            {
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            }
            else if(optopt == 'o')
            {
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            }
            else if (isprint (optopt))
            {
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            }
            else
            {
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
                return 0;
            }
        default:
            abort ();
        }

    printf ("parallelism = %d, input = %s, output = %s\n",
            parallelism, input, output);

    for (index = optind; index < argc; index++)
    {
        printf ("Non-option argument %s\n", argv[index]);
    }

    printf("enter the code\n");

    //set the number of children process
    cProcessN = parallelism;

    //set clock to calculate the run time
    startClock = clock();

    //start read word from file
    wordsAll = (char**)malloc(10*sizeof(char*));
    flag = 1;
    line= 0;

    temChar = fgetc(fpInput);
    while(temChar!=EOF)
    {
        if(checkChar(temChar))
        {
            if(flag == 1)
            {
                temWord = (char*)malloc(2*sizeof(char));
                row = 0;
            }
            else
            {
                row=row+1;
                temWord=(char*)realloc(temWord,(row+2)*sizeof(char));
            }
            temWord[row] = temChar;
            flag = 2;

            //last word
            temChar = fgetc(fpInput);
            if(temChar==EOF)
            {
                temWord[row+1]= '\0';
                wordsAll = (char**)realloc(wordsAll,(line+10)*sizeof(char*));
                wordsAll[line]=temWord;
                line++;
            }
        }
        else
        {
            if(flag == 2)
            {
                temWord[row+1]= '\0';
                wordsAll = (char**)realloc(wordsAll,(line+1)*sizeof(char*));
                wordsAll[line]=temWord;
                line++;
            }
            flag = 1;

            temChar = fgetc(fpInput);
            if(temChar==EOF)
            {
                break;
            }
        }
    }
    wordsAll[line] = 0;

    for(t=0; t<line; t++)
    {
        //printf("wordsAll[%d]: %s\n",t,wordsAll[t]);
    }

    //printf("line: %d\n",line);
    //separate the array wordsAll based on cProcess number
    start = 0;
    end = 1;
    range = line/cProcessN;
    remainder = line%cProcessN;
    //printf("remainder: %d\n",remainder);
    //printf("range: %d\n",range);
    for(i = 0; i<cProcessN; i++ )
    {
        separatedWordsRange[i][start] = i*range;
        separatedWordsRange[i][end] = ((i+1)*range)-1;
    }
    separatedWordsRange[cProcessN-1][end] = line-1;

    for(i = 0; i<cProcessN; i++ )
    {
        //printf("separatedWordsRange: %d~%d\n",separatedWordsRange[i][start],separatedWordsRange [i][end]);
    }

    printf("cProcessN: %d\n",cProcessN);

    for(i=0; i<cProcessN; i++)
    {

        pid = fork();

        if(pid==0)
        {
            printf( "This is in the %d child process,here read a string from the pipe. pid:%d ppid:%d\n",i,getpid(),getppid() );

            //count the words frequency
            n=0;
            words = (struct word*)malloc(1*sizeof(struct word));
            words[0].content = (char*)malloc(strlen(wordsAll[(separatedWordsRange[i][start])])*sizeof(char));
            strcpy(words[0].content,wordsAll[(separatedWordsRange[i][start])]);
            words[0].number=0;

            for(t=separatedWordsRange[i][start]; t<(separatedWordsRange[i][end]+1); t++)
            {
                found = 0;
                for(j=0; j<=n; j++)
                {

                    if(strcmp(wordsAll[t],words[j].content)==0)
                    {
                        found = 1;
                        words[j].number = words[j].number+1;
                        break;
                    }
                }
                if(found==0)
                {
                    n=n+1;
                    words = (struct word*)realloc(words,(n+1)*sizeof(struct word));
                    words[n].content = (char*)malloc(strlen(wordsAll[t])*sizeof(char));
                    strcpy(words[n].content,wordsAll[t]);
                    words[n].number=1;
                }
            }

            //sort the word frequency array, the length of the array is n+1
            for(t=0; t<=n-1; t++)
            {
                for(j=t+1; j<=n; j++)
                {
                    if(words[j].number>words[t].number)
                    {
                        temWordStruct=words[t];
                        words[t]=words[j];
                        words[j]=temWordStruct;
                    }
                }
            }

            for(t=0; t<=n; t++)
            {
                //printf("i: %d %s %d\n",i,words[t].content,words[t].number);
            }

            //add a flag element for marking the end of the array;
            words[n+1].content="1";
            //strcpy(words[n+1].content,"1");
            words[n+1].number=1;

            //send all data to pipe
            for(t=0; t<=n+1; t++)
            {
                sprintf(frequencyPipe, "%d", words[t].number);
                strcpy(plusSign,"+");
                strcpy(content,words[t].content);
                connect1 = strcat(content,plusSign);
                connect1 = strcat(connect1,frequencyPipe);

                write(pfds[1], connect1, 120);
            }

            //free(wordsAll);
            //free(wordsOrdered);
            //free(words);
            exit(3);
        }
        else if(pid>0)
        {
            //free(wordsAll);
            printf( "This is in the father process. pid %d\n",getpid() );

            //when all children processes have been created.
            if(i==cProcessN-1)
            {
                n = 1;
                flag=0;
                //crate array for saving the final sorted data
                wordsParent = (struct word*)malloc(1*sizeof(struct word));
                //add the first word to arrary
                read(pfds[0], connect2, 120);
                //separate the result
                seperatePipe = strtok(connect2,"+");
                strcpy(contentPipe,seperatePipe);
                seperatePipe = strtok(NULL,"+");
                strcpy(frequencyPipe,seperatePipe);
                wordsParent[0].content = (char*)malloc(100*sizeof(char));
                strcpy(wordsParent[0].content,contentPipe);
                wordsParent[0].number = atoi(frequencyPipe);
                //read all results from pipe

                free(wordsAll);

                while(flag<cProcessN)
                {
                    found = 0;
                    read(pfds[0], connect2, 120);
                    seperatePipe = strtok(connect2,"+");
                    strcpy(contentPipe,seperatePipe);
                    seperatePipe = strtok(NULL,"+");
                    strcpy(frequencyPipe,seperatePipe);

                    for(t=0; t<n; t++)
                    {
                        if(strcmp(wordsParent[t].content,contentPipe)==0)
                        {
                            wordsParent[t].number= wordsParent[t].number + atoi(frequencyPipe);
                            found = 1;
                            break;
                        }
                    }

                    if(found == 0)
                    {
                        wordsParent = (struct word*)realloc(wordsParent,(n+1)*sizeof(struct word));
                        wordsParent[n].content = (char*)malloc(100*sizeof(char));
                        strcpy(wordsParent[n].content,contentPipe);
                        wordsParent[n].number = atoi(frequencyPipe);
                        n++;
                    }

                    if(strcmp(contentPipe,"1")==0)
                    {
                        flag = flag+1;
                        n=n-1;//ignore flag, do not save it in arrary
                    }
                }

                //sort the word frequency array, the length of the array is n+1
                for(t=0; t<=n-2; t++)
                {
                    for(j=t+1; j<=n-1; j++)
                    {
                        if(wordsParent[j].number>wordsParent[t].number)
                        {
                            temWordStruct=wordsParent[t];
                            wordsParent[t]=wordsParent[j];
                            wordsParent[j]=temWordStruct;
                        }
                    }
                }

                for(t=0; t<n; t++)
                {
                    //printf("%s %d\n",wordsParent[t].content,wordsParent[t].number);
                }

                //write to the file
                for(t=0; t<n; t++)
                {
                    fprintf(fpOutput,"%s %d\n",wordsParent[t].content,wordsParent[t].number);
                }

                //to make sure all children correctly exit
                for(t=0; t<cProcessN; t++)
                {
                    pid=wait(&status);

                    printf("I catched a child process with pid of %d\n",pid);

                    if(WIFEXITED(status))
                    {
                        printf("the child process %d exit normally.\n",pid);
                        printf("the return code is %d.\n",WEXITSTATUS(status));
                    }
                    else
                    {
                        printf("the child process %d exit abnormally.\n",pid);
                        printf("the return code is %d.\n",WEXITSTATUS(status));
                    }
                }

                finishClock = clock();
                duration = (double)(finishClock - startClock) / CLOCKS_PER_SEC;
                //printf( "code runs %f seconds\n", duration );
                duration = (double)(finishClock - startClock);
                //printf( "code runs %f seconds\n", duration );
            }
        }
        else
        {
            printf("\n Fork failed, quitting!!!!!!\n");
            return 1;
        }
    }
    return 0;
}
