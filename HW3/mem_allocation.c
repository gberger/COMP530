#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_NAME_LEN 30

typedef struct {
    char* name;
    int pid;
} Student;

//This intializes the student
Student* makeStudent(char *name, int PID){
    Student *result = malloc(sizeof(Student));
    /* malloc dynamically requests memory on the heap at runtime.
     * It returns a pointer to the new allocated memory.
     * an alternative is calloc which inializes everything to 0.
     */
    
    //Next we initalize the student's informaiton
    result->name = name;
    
    //Note this is a soft copy of the name
    /* result->name = malloc(strlen(name)*sizeof(char)) create space on heap for name
     * strcpy(result->name,name)  copies the chars to new allocated location
     */
    
    result->pid = PID;

    return result;
}

void printStudent(Student *student){
    printf("%s's PID is %d\n", student->name, student->pid);
}

/* This program asks user to put in name and pid and then makes a student struct and prints it. */
int main(int argc, char** argv) {
    
    size_t numChars = sizeof(char)*MAX_NAME_LEN;
	//This is for getline. It doesn't matter the size we set
	//Because getline reallocates if input is bigger.   

    // Set up some attributes for a student 
    int myPid;
    char *myName = malloc(numChars); //allocate heap space for name

    printf("Enter Your Name: ");
    getline(&myName, &numChars, stdin);//This returns everything before a \n
    
    myName[strlen(myName)-1] = '\0'; // remove the Newline     


    printf("Enter Your PID: ");
    scanf("%d",&myPid);//reads in an integer from stdin

    //intialize the student struct
    Student *me = makeStudent(myName,myPid);

    //print Struct
    printStudent(me);
   
    //You are responsible for the memory you allocate, which means you have to deallocate when you're done.
    //This is done with free
    free(me);
    free(myName);

    return 0;
}
