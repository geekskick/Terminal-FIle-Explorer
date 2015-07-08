//
//  main.c
//  File Explorer
//
// #################### CPP & typedefs ####################

#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define TOP_DIR             "/Users"   //Macs start at the top level directory /Users/
#define MAX_FILE_NAME       100        //longest file name
#define MAX_FILES           200        //maximum amount of files in a directory

#define DIR_REALLOC_ERROR   -1
#define STR_ERR             -1
#define STR_SUCCESS         0
#define HIDE_ITEM           0
#define SHOW_ITEM           1
#define EXIT                0
#define UP                  -1

#define TEMP_FILENAME       "/Users/patrickmintram/Documents/Work/XCODE/USBAccessor/Last_Place.txt"
#define FILE_R              "r"
#define FILE_RW             "r+"
#define FILE_W              "w"

#define print_contents(directory, files)    print_contents_p(directory, files, 0)
#define err_chk(a)  if(!a){ fprintf(stderr, "Error assigning memory [LINE %d]\n", __LINE__); goto error; }

//structure for holding the directory name and it's dirent structure
typedef struct {
    DIR* dir_s;
    char dir_n[MAX_FILE_NAME];
}dir_struct;

typedef unsigned int uint;

//#################### PROTOTYPES ####################
void print_contents_p(const dir_struct* directory, struct dirent **current, uint file_number);
char* get_DT_type(int DT_TYPE);
void put_contents_in_array(dir_struct *directory, struct dirent *dir_arr[MAX_FILES]);
dir_struct* open_dir(char* file_path);
int test_item(struct dirent* item);
void go_up(char*filename, char* output);
char* get_extension(char *filename);
int get_num_files(struct dirent **contents);
int realloc_dir(dir_struct* directory, char* file_path, FILE *fp);
void time_stamp_fp(FILE* fp);


//#################### Functions ####################

//.............converts the d_type of the file into a string for displaying.............
char* get_DT_type(int DT_TYPE){

    switch (DT_TYPE) {
        case DT_REG:
            return "FILE";
            
        case DT_DIR:

            return "DIR";
            
        default:
            return "OTHER";
        
    }
    
}

//shortens the filename input by removing everything from the last / and then putting it in the output
void go_up(char*filename, char* output){
    
    /*
     start at the end of the string and work backwards, until a / is encountered, signifying
     the start of the fcurrent directory. once the / is found replace it with the end marker
     */
    
    uint i = (uint)strlen(filename) - 1; //-1 to account for \0 at end of string
    
    while ( filename[i] != '/') { i--; }
    filename[i] = '\0';
    
    strcpy(output, filename);
}

//a..............ppends the files extension to the output string............
char* get_extension(char *filename){
    
    /*
     start at the end of the string and work baxkwards, until a . is encountered, signifying 
     the start of the file extn. Take the add of this and concatenate it with the the out put
     
     make sure to put a small gap to give a nice buffer
     */
    uint i = (uint)strlen(filename) - 1;
    
    while ( filename[i] != '.') { i--; }
    
    return &(filename[i]);
    
}

//..............shows contents of the directory.............
void print_contents_p(const dir_struct* directory, struct dirent **current, uint file_number){
    
    /*
     Since the function is recursive and when called it is initially passed a 0, as the file_number value
     display the name of the directory
     */
    if (file_number == 0) {
        printf("Items in: %s\n", directory->dir_n);
        file_number++;
    }
    
    /*
        Each time the function is called it's looking at the thing in the array, at an offset of +line number, so pass it the address of the next element of the array.
     
     printf formatting ensures the lines all end up nice on the screen with a guaranteed length of 20 characters at all times
     */
    if (*current){
       /*
        display the type of item, DIR, FILE or OTHER, and if it's a file it's extension. 
        In order to keep the columns neat restrict the filename/itemname to 40 characters in width
        */
        
        printf("%3.d > %40.40s\t\t%s\t%s\n", file_number++, (*current)->d_name, get_DT_type((int)(*current)->d_type), (*current)->d_type == DT_REG ? get_extension((*current)->d_name): " ");
    
        //recursion, passing the next item along in the array
        print_contents_p(directory, current + 1, file_number);
    }
    
    
}

//..........puts the directory contents into the array passed in...............
void put_contents_in_array(dir_struct *directory, struct dirent **dir_arr){
    
    //must start at the beginning fof the directory stream
    rewinddir(directory->dir_s);
    
    memset(dir_arr, 0, MAX_FILES);
    
    struct dirent *temp = NULL;
    int i = 0;
    
    /*
     readdir() automatically points to the next item in the directory, as it's a stream. therefore keep going until it returns a NULL pointer, indicating there are no more items in the directory
     */
    
    while((temp = readdir(directory->dir_s))){
            if(test_item(temp) == SHOW_ITEM){
                dir_arr[i++] = temp;
            }
    }
}


// ..........populates dir_struct.............
dir_struct* open_dir(char* file_path){
    
    dir_struct *current_dir = calloc(1, sizeof(dir_struct));
    assert(current_dir);
    
    strcpy(current_dir->dir_n, file_path);
    
    current_dir->dir_s = opendir(current_dir->dir_n);
    assert(current_dir->dir_s);
    
    return current_dir;
}

// ............ safety closes the directory and opens a new one, appending this to the file at the same time .....
int realloc_dir(dir_struct* directory, char* file_path, FILE *fp){
    
    /*
     the sting buffer must be cleared before copy, oher wise if going to a smaller string length then there may be a remainder at the end of thestring still
     */
    closedir(directory->dir_s);
    memset(directory->dir_n, 0, MAX_FILE_NAME);
    
    strcpy(directory->dir_n, file_path);
    
    directory->dir_s = opendir(directory->dir_n);
    err_chk(directory->dir_s);
    
    //append to file
    time_stamp_fp(fp);
    fprintf(fp, "%s\n", directory->dir_n);
    
    return 0;

error:
    return DIR_REALLOC_ERROR;
}

int test_item(struct dirent* item){
    if (item->d_name[0] == '.') {
        return HIDE_ITEM;
    }
    return SHOW_ITEM;
}

//...........gets user input and validates it ..........
int get_input(int max_num){
#define MAX_IN  5 //maximum input string length
    char in_c[MAX_IN] = { 0 };
    int  in_i = 0;
    
    /*
     input must be read in as a string to account for multi digit numvers, and -1 input. 
     
     strtol then converts this to an ineger and finally it is checked to see if it's in range before passing back the valid result
     */
    printf("\nWhich directory do you want to explore 1 - %d: \n<%d to quit, %d to go up> ", max_num, EXIT, UP);
    scanf("%s", in_c);
    
    in_i = (int)strtol(in_c, NULL, 10);
    
    //if the user input was out of range of the optons, display an error message and call function again
    if (in_i < UP || in_i > max_num) {
        printf("Wrong number, try again\n");
        get_input(max_num);
    }
    
    return in_i;
    
}

//........ counts the number of files in the directory ..........
int get_num_files(struct dirent **contents){
    int i = 0;
    
    while(contents[i] != NULL) { i++; }
    return i;
}

//.......... put a time samp on the file ............
void time_stamp_fp(FILE* fp){
    
    time_t timer = NULL;
    struct tm* GMT = NULL;
    
    //convert the epoch time into GMT
    time(&timer);
    GMT = gmtime(&timer);
    
    fprintf(fp, "%.2d:%.2d, %.2d-%.2d-%.2d ", GMT->tm_hour, GMT->tm_min, GMT->tm_mday, GMT->tm_mon, 1900 + GMT->tm_year);
    
}

// #################### MAIN ####################

int main(int argc, const char * argv[]) {
    
    printf("Welcome to the file manager\n---------------------------\n");
    
    dir_struct *current_dir = NULL;
    int i = -(int)strlen(TOP_DIR);
    
    FILE *fp = fopen(TEMP_FILENAME, FILE_RW);
    
    //file not in exisitance so create it and start file explorer from top level
    if (!fp) {
        fp = fopen(TEMP_FILENAME, FILE_W);
        current_dir = open_dir(TOP_DIR);
    }
    
    //file opened successfully,
    else{
        
        char from_file[MAX_FILE_NAME] = { 0 };
         /*
          iterate bacwards through the file until the /user phrase is found. 
          This is done by moveing the file stream backwards, reading from the file stream onwards, then companing the first 5 or so characters to the /user phrase. 
          
          iterative process means that i seek fursther from the end each time, inly stopping when a result is found, or the current poisition of the cursor is at the start for the file
          */
        do {
            fseek(fp, i--, SEEK_END);
            fscanf(fp, "%s", from_file);
        } while (strncmp(TOP_DIR, from_file, strlen(TOP_DIR)) != 0 && SEEK_CUR != SEEK_SET);
        
        //open saved directory
        current_dir = open_dir(from_file);
    }

    //initialise the starting directory to the top level mac folder '/Users/'
    
    struct dirent **dir_contents = calloc(MAX_FILES, sizeof(struct dirent*));
    err_chk(dir_contents);
    
    put_contents_in_array(current_dir, dir_contents);
    print_contents(current_dir, dir_contents);
    
    int input = 0;
    
    //present workin directory
    char *pwd = calloc(MAX_FILE_NAME, sizeof(char));
    err_chk(pwd);
    strcpy(pwd, current_dir->dir_n);
    

    //main loop
    while ((input = get_input(get_num_files(dir_contents))) != 0) {
        switch (input) {
                
            case EXIT:
                break;
                
            case UP:
                go_up(current_dir->dir_n, pwd);
                break;
                
            default:
                
                /*
                 the current directory filepath doesn't end in a /, so this must be
                 put on BEFORE the next part of the filename
                 */
                strcat(pwd, "/");
                strcat(pwd, dir_contents[input -1]->d_name);
                break;
        }
        
        
        /*
         realloc_dir safely closes and reopens the new directory, saving it's filepath at the same time
         */
        if(realloc_dir(current_dir, pwd, fp) == DIR_REALLOC_ERROR) goto error;
        put_contents_in_array(current_dir, dir_contents);
        print_contents(current_dir, dir_contents);
    }
    
    //cleaning up
error:
    printf("Exitting\n");
    fclose(fp);
    closedir(current_dir->dir_s);
    if (current_dir)    free(current_dir);
    if (dir_contents)   free(dir_contents);
    if (pwd)            free(pwd);
    return 0;
}
