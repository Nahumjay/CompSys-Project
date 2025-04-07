#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char*argv[]) {
    FILE *fptr;
    FILE *fp;
    //const char* file;

    fptr = fopen("example.txt","w"); // opening a file to write to
    fp = fopen("two.txt","w"); 
    if (fp == NULL)
    {
        printf("ERROR Creating File!\n");
        exit(1);
    }

    /*
   
    int L = atoi(argv[1]); // converting L into a integer
    int L;
    if (L < 20000) { // check to see if L > 20000
        printf("THE VALUE OF L HAS TO BE GREATER THAN 20000");
        exit(1);
    }

    for (int i = 0; i < L; i++) { // appening to file a list of numbers greater than 20000
        fprintf(fptr, "%d", rand());
    }
    fprintf(fptr, "\n");
    */

    


    for (int i = 0; i < 80; i++) {
        int random_num = rand() % 80;
        random_num = -(random_num + 1);
        fprintf(fp, "%d\n", random_num);
    }

    
}
