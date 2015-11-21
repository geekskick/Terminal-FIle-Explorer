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
#include <sys/ioctl.h>

//maximum values
#define MAX_FILE_NAME       100        //longest file name
#define MAX_FILES           200        //maximum amount of files in a directory
#define MAX_IN              5          //maximum input string length

//for hidden files
#define HIDE_ITEM           0
#define SHOW_ITEM           1

//controls
#define EXIT                0
#define UP                  -2
#define TOP_LEVEL           -1

//return values
#define SUCCESS             0
#define ERR                 -1

//file operators
#define TEMP_FILENAME       "/Last_Place.txt"
#define FILE_R              "r"
#define FILE_RW             "r+"
#define FILE_W              "w"

//for ease
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
    return ERR;
}

/* 
 checks if item is hidden or not
 hidden files are '.filename', so test the first character and return whether it needs to be hidden or shown
 */
int test_item(struct dirent* item){
    if (item->d_name[0] == '.') {
        return HIDE_ITEM;
    }
    return SHOW_ITEM;
}

//...........gets user input and validates it ..........
int get_input(int max_num){

    char in_c[MAX_IN] = { 0 };
    int  in_i = 0;
    
    /*
     input must be read in as a string to account for multi digit numvers, and -1 input.
     
     strtol then converts this to an ineger and finally it is checked to see if it's in range before passing back the valid result
     */
    printf("\nWhich directory do you want to explore 1 - %d: \n<%d quit><%d top level><%d go up>\n", max_num, EXIT, TOP_LEVEL, UP);
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

//.......... put a time stamp on the file ............
void time_stamp_fp(FILE* fp){
    
    time_t timer;
    struct tm* GMT = NULL;
    
    //convert the epoch time into GMT
    time(&timer);
    GMT = gmtime(&timer);
    
    fprintf(fp, "%.2d:%.2d, %.2d-%.2d-%.2d ", GMT->tm_hour, GMT->tm_min, GMT->tm_mday, GMT->tm_mon, 1900 + GMT->tm_year);
    
}

// #################### MAIN ####################
int main(int argc, const char * argv[]) {
    
    printf("Welcome to the File Explorer\n---------------------------\n");
    
    //filepath strings
    char    *current_path   = NULL,
            *root_dir       = getenv("HOME"),                       //automatically gets the environment ;home' property 'users/myname'
            *temp_file_path = calloc(MAX_FILE_NAME, sizeof(char));
    
    err_chk(temp_file_path);
    
    //get the exe's folder
    current_path = getcwd(NULL, MAX_FILE_NAME);
    
    //create the filepath string
    strcpy(temp_file_path, current_path);
    strcat(temp_file_path, TEMP_FILENAME);
    
    dir_struct *current_dir = NULL;
    int input = (int)strlen(root_dir) - 2; //randomly chose two
    
    /*
     open file to read the last place out of, 
     however if it's not there, create it in w mode
     
     if it is there, close then make a new one in w mode
     */
    
    //open in read mode
    FILE *fp = fopen(temp_file_path, FILE_R);
    
    //if file doesnt exist create and open for writing
    if (!fp) {
        fp = fopen(temp_file_path, FILE_W);
        current_dir = open_dir(root_dir);
    }
    else{
        char from_file[MAX_FILE_NAME] = { 0 };
        /*
         iterate bacwards through the file until the /user phrase is found.
         This is done by moving the file stream backwards, reading from the file stream onwards, then comparing the first 5 or so characters to the /user phrase.
         
         iterative process means that i seek fursther from the end each time, inly stopping when a result is found, or the current poisition of the cursor is at the start for the file
         */
        do {
            fseek(fp, -(input++), SEEK_END);
            fscanf(fp, "%s", from_file);
        } while (strncmp(root_dir, from_file, strlen(root_dir)) != 0 && input < MAX_FILE_NAME);
        
        //open saved directory, or the root director if no info in the file
        current_dir = open_dir(strlen(from_file) == 0 ? root_dir : from_file);
        
        //open in write mode
        fclose(fp);
        fp = fopen(temp_file_path, FILE_W);
        err_chk(fp);
    }
    
    struct dirent **dir_contents = calloc(MAX_FILES, sizeof(struct dirent*));
    err_chk(dir_contents);
    
    put_contents_in_array(current_dir, dir_contents);
    print_contents(current_dir, dir_contents);
    
    //present working directory
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
                
            case TOP_LEVEL:
                memset(pwd, 0, MAX_FILE_NAME);
                strcpy(pwd, root_dir);
                break;
                
            default:
                
                /*
                 the current directory filepath doesn't end in a /, so this must be
                 put on BEFORE the next part of the filename
                 
                 get_input() validates the input, so no need for checking again
                 */
                strcat(pwd, "/");
                strcat(pwd, dir_contents[input -1]->d_name);
                break;
        }
        
        /*
         realloc_dir safely closes and reopens the new directory, saving it's filepath at the same time
         */
        if(realloc_dir(current_dir, pwd, fp) == ERR) goto error;
        put_contents_in_array(current_dir, dir_contents);
        print_contents(current_dir, dir_contents);
    }
    

    /*
     although controversial labels are mentioned as ok in k&r as long as it's infrequent and carefully managed. 
     In this case any error will send it to here, then clean up HEAP usage
     */
error:
    printf("Exitting\n");
    if(fp)              fclose(fp);
    if(current_dir->dir_s)
        closedir(current_dir->dir_s);
    if (temp_file_path) free(temp_file_path);
    if (current_dir)    free(current_dir);
    if (current_path)   free(current_path);
    if (dir_contents)   free(dir_contents);
    if (pwd)            free(pwd);
    
    return 0;
}
