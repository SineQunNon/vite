#ifdef _WIN32
    #include <stdio.h>
    #include <stdlib.h>
    #include <conio.h>
    #include <windows.h>       
    #include <string.h>    
    #define UP_ARROW 72
    #define DOWN_ARROW 80
    #define LEFT_ARROW 75
    #define RIGHT_ARROW 77
    #define HOME 71
    #define END 79
    #define PAGE_UP 73
    #define PAGE_DOWN 81
    #define BACK_SPACE 8
    #define ENTER 13
    #define ESC 27
    #define CTRL_S 19
    #define CTRL_F 6
    #define CTRL_Q 17
    #define CLEAR "cls"
#else   
    #include <stdio.h>
    #include <stdlib.h>
    #include <termios.h>
    #include <unistd.h>
    #include <string.h>
    #include <sys/ioctl.h>
    #define CTRL_KEY(k) ((k)& 0x1f)
    #define UP_ARROW 1111
    #define DOWN_ARROW 2222
    #define LEFT_ARROW 3333
    #define RIGHT_ARROW 4444
    #define HOME 5555
    #define END 6666
    #define PAGE_UP 7777
    #define PAGE_DOWN 8888
    #define ESC 27
    #define CLEAR "clear"
    struct termios orig_termios; //terminal setting
    struct termios orig_termios_quit; //terminal re-setting
#endif

#ifdef __APPLE__
    #define ENTER '\r'
    #define BACK_SPACE 127
#elif __linux__
    #define ENTER 13
    #define BACK_SPACE 127
#endif

int cursor_x; //location of terminal x-coordinate
int cursor_y; //location of terminal y-coordinate
int cursor_y_out; //tracking of file row
int terminal_row_size; //terminal row size
int terminal_col_size; //terminal column size
int file_row_length; //file total length

int down_status; //input down arrow key at bottom
int up_status; //input up arrow key at top
int page_up_status; //input page up key at top
int page_down_status; //input page down key at bottom

char * msg_bar1; //first message bar
char * msg_bar2; //second message bar
char * filename;

int quit_status=0;

//store file information row by row
typedef struct file_row_info{
    char * row;
    int len;
}file_row_info;

//store location of searching information
typedef struct search_info{
    int row_location;
    int col_location;
}search_info;

struct file_row_info *row_info;

#ifdef _WIN32
    
#else
    /* get terminal size */
    int get_cursor_position(int *rows, int *cols) {
        char buf[32];
        unsigned int i = 0;

        if (write(STDOUT_FILENO, "\033[6n", 4) != 4) {
            return -1;
        }

        while (i < sizeof(buf) - 1) {
            if (read(STDIN_FILENO, &buf[i], 1) != 1) {
                break;
            }
            if (buf[i] == 'R') {
                break;
            }
            i++;
        }
        buf[i] = '\0';

        if (buf[0] != '\033' || buf[1] != '[') {
            return -1;
        }
        if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
            return -1;
        }

        return 0;
    }

    int get_window_size(int *rows, int *cols) {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
            if (write(STDOUT_FILENO, "\033[999C\033[999B", 12) != 12) {
                return -1;
            }
            return get_cursor_position(rows, cols);
        } else {
            *cols = ws.ws_col;
            *rows = ws.ws_row;
            return 0;
        }
    }
#endif

void draw_msg_line(int terminal_line){
    #ifdef _WIN32
        if(terminal_line == terminal_row_size -2){
            char cursor_status[101];
            sprintf(cursor_status, "no ft | %ld/%ld", cursor_x + 1, cursor_y + cursor_y_out + 1);
            msg_bar1 = (char*)malloc(terminal_col_size * (sizeof(char)));
            int empty_space=0;
            if(filename==NULL){
                sprintf(msg_bar1, "\n\x1B[7m[No Name] - %d lines", file_row_length);
                empty_space = terminal_col_size - strlen(msg_bar1) - strlen(cursor_status)+4;
            }else{
                sprintf(msg_bar1, "\x1B[7m[%s] - %d lines",filename, file_row_length);
                empty_space = terminal_col_size - strlen(msg_bar1) - strlen(cursor_status);
            }
            //printf("msg_bar1 : %d\n", strlen(msg_bar1));
            

            char * back_msg_bar1 =malloc(sizeof(char));
            for(int idx =0; idx <=empty_space; ++idx){
                back_msg_bar1 = realloc(back_msg_bar1, sizeof(char)* (idx+1));
                back_msg_bar1[idx] = ' ';
            }
            
            printf("%s", msg_bar1);
            printf("%s", back_msg_bar1);
            printf("%s", cursor_status);
            printf("\n");
        }else{
            msg_bar2 = (char *)malloc(terminal_col_size * sizeof(char));
            sprintf(msg_bar2,"\x1B[0mHELP: Ctrl-s = save | Ctrl-q = quit | Ctrl-f = find");
            printf("%s", msg_bar2);
        }
    #elif __APPLE__
         if(terminal_line == terminal_row_size - 2){
            char cursor_status[101];
            sprintf(cursor_status, "no ft | %d/%d", cursor_x + 1, cursor_y + cursor_y_out + 1);
            msg_bar1 = (char *)malloc(terminal_col_size * sizeof(char));
            if(filename == NULL){
                sprintf(msg_bar1, "\x1B[7m[No Name] - %d lines",file_row_length);
            }else{
                sprintf(msg_bar1, "\x1B[7m[%s] - %d lines",filename, file_row_length);
            }
            // int len = strlen(msg_bar1);
            int empty_space = terminal_col_size - strlen(msg_bar1) - strlen(cursor_status) + 4;
            
            // sprintf(buf, " size : %d string : %d", empty_space, len);
            char * back_msg_bar1 = (char *)malloc(empty_space * sizeof(char));
            for(int idx =0; idx < empty_space; ++idx){
                back_msg_bar1[idx] = ' ';
            }                        
            // printf("\x1B[0m");
            // write(STDOUT_FILENO,"\x1B[0m", strlen("\x1B[0m"));
            write(STDOUT_FILENO, msg_bar1, terminal_col_size);
            // write(STDOUT_FILENO, buf, strlen(buf));
            write(STDOUT_FILENO, back_msg_bar1, strlen(back_msg_bar1));
            write(STDOUT_FILENO, cursor_status, strlen(cursor_status));
            write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
            // write(STDOUT_FILENO, "\x1B[0m\r\n", strlen("\r\n"));
        }else if (terminal_line == terminal_row_size - 1){
            msg_bar2 = (char *)malloc(terminal_col_size * sizeof(char));
            sprintf(msg_bar2,"\x1B[0mHELP: Ctrl-s = save | Ctrl-q = quit | Ctrl-f = find");
            write(STDOUT_FILENO, msg_bar2, strlen(msg_bar2));
        }

    #elif __linux__
        if(terminal_line == terminal_row_size - 2){
                char cursor_status[101];
                sprintf(cursor_status, "no ft | %d/%d", cursor_x + 1, cursor_y_out + 1);
                msg_bar1 = (char*)malloc(terminal_col_size *(sizeof(char)));

                int empty_space = 0;
                if(filename == NULL){
                    sprintf(msg_bar1, "\x1B[7m[No Name] - %d lines",file_row_length);
                    empty_space = terminal_col_size - strlen(msg_bar1) - strlen(cursor_status)+4;
                }else{
                    sprintf(msg_bar1,"\x1B[7m[%s] - %d lines",filename, file_row_length);
                    empty_space = terminal_col_size - strlen(msg_bar1) - strlen(cursor_status) + 4;
                }
                write(STDOUT_FILENO, msg_bar1, strlen(msg_bar1));
                //write(STDOUT_FILENO, cursor_status, strlen(cursor_status));

                for(int i=0; i<empty_space; ++i){
                    write(STDOUT_FILENO, " ", strlen(" "));
                }
                // write(STDOUT_FILENO, back_msg_bar1, strlen(back_msg_bar1));
                write(STDOUT_FILENO, cursor_status, strlen(cursor_status));

            }else if (terminal_line == terminal_row_size - 1){
                msg_bar2 = (char *)malloc(terminal_col_size * sizeof(char));
                sprintf(msg_bar2,"\x1B[0mHELP: Ctrl-s = save | Ctrl-q = quit | Ctrl-f = find");
                write(STDOUT_FILENO, msg_bar2, strlen(msg_bar2));
            }
    #endif
}
        
       

/*initialize - no argument*/
void open_new_terminal(void){
    #ifdef _WIN32
        for(int terminal_line=0; terminal_line<terminal_row_size;++terminal_line){
            if(terminal_line == terminal_row_size-1 || terminal_line == terminal_row_size-2){

            }else if(terminal_line < terminal_row_size -3){
                if(terminal_line == terminal_row_size / 3){
                    printf("~");

                    for(int i=0; i < terminal_col_size / 3; ++i){
                        printf(" ");
                    }
                    printf("Viusal Text Editor -- version 0.0.1\n");
                }else{
                    printf("~\n");
                }
            }else{
                printf("~");
            }    
        }
    #else
        int terminal_line = 0;

        for(int terminal_line = 0; terminal_line < terminal_row_size; ++terminal_line){
            // if(terminal_line == terminal_row_size - 2){
            //     msg_bar1 = (char *)malloc(terminal_col_size * sizeof(char));
            //     sprintf(msg_bar1, "\x1B[7m[No Name] - %d lines",file_row_length);
                
            //     int empty_space = terminal_col_size - strlen(msg_bar1);
            //     char * back_msg_bar1 = (char *)malloc(empty_space * sizeof(char));
            //     for(int idx =0; idx < empty_space; ++idx){
            //         back_msg_bar1[idx] = ' ';
            //     }                        
            //     // printf("\x1B[0m");
            //     // write(STDOUT_FILENO,"\x1B[0m", strlen("\x1B[0m"));
            //     write(STDOUT_FILENO, msg_bar1, terminal_col_size);
            //     write(STDOUT_FILENO, back_msg_bar1, strlen(back_msg_bar1));
            //     write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
            //     // write(STDOUT_FILENO, "\x1B[0m\r\n", strlen("\r\n"));
            // }else if (terminal_line == terminal_row_size - 1){
            //     msg_bar2 = (char *)malloc(terminal_col_size * sizeof(char));
            //     sprintf(msg_bar2,"\x1B[0mHELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");
            //     write(STDOUT_FILENO, msg_bar2, strlen(msg_bar2));
            // }
            if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                draw_msg_line(terminal_line);
            }
            else if(terminal_line == terminal_row_size / 3){
                int title_left_empty_size = terminal_col_size / 3;
                write(STDOUT_FILENO, "~", 1);
                while(title_left_empty_size > 0){
                    write(STDOUT_FILENO, " ", 1);
                    title_left_empty_size--;
                }
                write(STDOUT_FILENO, "Visual Text Editor -- version 0.0.1\r\n", strlen("Visual Text Editor -- version 0.0.1\r\n"));
            }else{
                if(terminal_line < terminal_row_size -1){
                    write(STDOUT_FILENO, "~\r\n", strlen("~\r\n"));
                }else{
                    write(STDOUT_FILENO, "~", strlen("~"));
                }
            }
        }
        write(STDOUT_FILENO, "\033[H", strlen("\033[H")); 
    #endif

   
}


/* drawing file initial screen */
void draw_file_line(void){ // 0 ~
    #ifdef _WIN32
        for (int terminal_line = 0; terminal_line < terminal_row_size; ++terminal_line) {
            if (terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size - 1) {
                draw_msg_line(terminal_line);
            }else if(terminal_line > file_row_length){
                if (terminal_line < terminal_row_size - 1) {                    
                    printf("~\n");
                } else {
                    printf("~");
                }
            }else {
                if (row_info[terminal_line].len > terminal_col_size) {
                    row_info[terminal_line].len = terminal_col_size;
                }
                printf("%s", row_info[terminal_line].row); 
                if(terminal_line < terminal_row_size - 2){
                    printf("\n");
                }
            }
        }
    #else
        for(int terminal_line = 0; terminal_line < terminal_row_size; ++terminal_line){
            // if(terminal_line == terminal_row_size-1){
            //     char buf[100];
            //     for(int i=0; i< 10; ++i){
            //       sprintf(buf, "%d ", row_info[terminal_line].row[i]);  
            //       write(STDOUT_FILENO, buf, strlen(buf));
            //     }
            //     // sprintf(buf, "")
            //     // write(STDOUT_FILENO, "hello\r", strlen("hello\r"));
            // }else 
            if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                draw_msg_line(terminal_line);
            }else if(terminal_line > file_row_length){
                // char buf[100];
                // sprintf(buf, "line length : %d %d %d", row_info[terminal_line].len, terminal_line, terminal_row_size);
                // write(STDOUT_FILENO, row_info[terminal_line].row, row_info[terminal_line].len);
                // write(STDOUT_FILENO, buf, strlen(buf));
                if(terminal_line < terminal_row_size - 1){
                    write(STDOUT_FILENO, "~\r\n", strlen("~\r\n"));
                }else{
                    write(STDOUT_FILENO, "~", strlen("~"));
                }
            }else{
                // char buf[100];
                // sprintf(buf, "line length : %d %d %d", row_info[terminal_line].len, terminal_line, terminal_row_size);
                //If the size of the line is larger than the column size of the terminal, it exceeds it, so the size is reduced
                if(row_info[terminal_line].len > terminal_col_size) row_info[terminal_line].len = terminal_col_size;
                
                
                write(STDOUT_FILENO, row_info[terminal_line].row, row_info[terminal_line].len);
                // write(STDOUT_FILENO, buf, strlen(buf));
                if(terminal_line < terminal_row_size - 1){
                    write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
                }
                write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
            }
        }
        write(STDOUT_FILENO, "\033[H", strlen("\033[H"));
    #endif
    
}


/* Drawing the screen when the screen size is exceeded using the arrow keys */
void down_update_file_line(void){
    #ifdef _WIN32
        if(cursor_y == terminal_row_size - 3 && down_status == 1){
            for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
                if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                    draw_msg_line(terminal_line);
                }else{
                    int cur_line = terminal_line + cursor_y_out;
                    printf("\033[K");
                    
                    if(row_info[terminal_line].len > terminal_col_size) row_info[terminal_line].len = terminal_col_size;
                    printf("%s", row_info[cur_line].row);
                    
                    if(terminal_line < terminal_row_size-1){
                        printf("\n");
                }
                } 
            }
            printf("\033[%d;%dH", terminal_row_size-2, cursor_x);
            down_status = 0;
        }
    #else
        //cursor_y가 터미널 사이즈보다 크게 되었을 때 한 번만 업데이트되면 된다.
        if(cursor_y == terminal_row_size - 3 && down_status == 1){
            for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
                if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                    draw_msg_line(terminal_line);
                }else{
                    int cur_line = terminal_line + cursor_y_out;
                    write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
                    if(row_info[terminal_line].len > terminal_col_size) row_info[terminal_line].len = terminal_col_size;
                    write(STDOUT_FILENO, row_info[cur_line].row, row_info[cur_line].len);
                    if(terminal_line < terminal_row_size-1){
                        write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
                }
                } 
            }
            char buf[30];
            sprintf(buf,"\033[%d;%dH", terminal_row_size-2, cursor_x);
            write(STDOUT_FILENO, buf, strlen(buf));
            down_status = 0;
        }
    #endif
    
}

void up_update_file_line(){
    #ifdef _WIN32
        if(cursor_y == 0 && up_status == 1 && cursor_y_out > 0){
            for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
                if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                    draw_msg_line(terminal_line);
                }else{
                    int cur_line = terminal_line + cursor_y_out - 1;
                    printf("\033[K");
                    
                    if(row_info[terminal_line].len > terminal_col_size) row_info[terminal_line].len = terminal_col_size;
                    printf("%s",row_info[cur_line].row);
                    if(terminal_line < terminal_row_size-1){
                        printf("\n");
                    }
                }    
            }
            printf("\033[H");
            up_status = 0;
        }
    #else
        //it only needs to be updated once when cursor_y becomes larger than the terminal size.
        if(cursor_y == 0 && up_status == 1 && cursor_y_out > 0){
            for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
                if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                    draw_msg_line(terminal_line);
                }else{
                    int cur_line = terminal_line + cursor_y_out - 1;
                    write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
                    if(row_info[terminal_line].len > terminal_col_size) row_info[terminal_line].len = terminal_col_size;
                    write(STDOUT_FILENO, row_info[cur_line].row, row_info[cur_line].len);
                    if(terminal_line < terminal_row_size-1){
                        write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
                    }
                }
                
            }
            write(STDOUT_FILENO, "\033[H", strlen("\033[H"));
            up_status = 0;
        }
    #endif
    
}

char* tabs_to_spaces(char * line, int length){
    char * modified_line = malloc(length * 4);

    int pos = 0;

    for(int i=0; i<length; ++i){
        if(line[i]=='\t'){
            for(int j=0; j<8; ++j){
                modified_line[pos++] = ' ';
            }
        }else{
            modified_line[pos] = line[i];
            pos++;
        }
    }
    return modified_line;
}


char* initialize_row(char *line, int read) {
    char *new_line = malloc(sizeof(char) * (read + 1)); // Allocate enough space

    int pos = 0;

    for (int i = 0; i < read; ++i) {
        if (line[i] == '\t') {
            for (int j = 0; j < 4; ++j) { // Change tabs to 4 spaces
                new_line[pos++] = ' ';
            }
        } else {
            new_line[pos++] = line[i];
        }
    }

    new_line[pos] = '\0'; // Null-terminate the string

    return new_line;
}


void open_file(const char * filename){
    #ifdef _WIN32
        char * line = NULL;
        size_t len = 0;
        int read;
        FILE * fp = fopen(filename, "rt");
        if(fp == NULL){
            perror("error : failed to open file");
            exit(1);
        }

        file_row_length=0;
        
        while ((read = getline(&line, &len, fp)) != -1) {
            // Change tabs to 4 spaces and store in row_info
            if(read > 0 && line[read-1]=='\r'||line[read-1]=='\n') read--;
            row_info = (file_row_info *)realloc(row_info, sizeof(file_row_info) * (file_row_length + 1));
            row_info[file_row_length].row = initialize_row(line, read);
            row_info[file_row_length].len = strlen(row_info[file_row_length].row) + 1;
            row_info[file_row_length].row[row_info[file_row_length].len-1]='\0';
            file_row_length++;
        }
        // file_row_length--;

        fclose(fp);
    #elif __APPLE__
        char * line = NULL;
        size_t len = 0;
        ssize_t read;
        FILE * fp = fopen(filename, "rt");
        if(fp == NULL){
            perror("error : failed to open file");
            exit(1);
        }

        file_row_length=0;
        
        while((read = getline(&line, &len, fp)) != -1){
            /* Change tabs to 4 spaces*/
            row_info = (file_row_info *)realloc(row_info, sizeof(file_row_info)*(file_row_length+1));

            if(read > 0 && line[read-1]=='\r'||line[read-1]=='\n') read--;
            char * modified_line = tabs_to_spaces(line, read);
            
            row_info[file_row_length].row = modified_line;
            row_info[file_row_length].len = strlen(modified_line) + 1;
            row_info[file_row_length].row[row_info[file_row_length].len-1] = '\0';
            // row_info[file_row_length].row[row_info[file_row_length].len+1] = '\r';
            // row_info[file_row_length]->len = strlen(line);
            // char * p;
            // sprintf(p, "read : %d || len : %d ", read, len);
            // write(STDOUT_FILENO, p, strlen(p));
            // write(STDOUT_FILENO, line, read);
            // write(STDOUT_FILENO, "\r", 2);
            // write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
            file_row_length++;
        }
        // file_row_length--;
        // for(int i=0; i<5; ++i){
        //     char buf[50];
        //     sprintf(buf, "%d line : %d\r\n", i+1, file_col_length[i]);
        //     write(STDOUT_FILENO, buf, strlen(buf));
        // }
        
        fclose(fp);
    #else
        char * line = NULL;
        size_t len = 0;
        ssize_t read;
        FILE * fp = fopen(filename, "rt");
        if(fp == NULL){
            perror("error : failed to open file");
            exit(1);
        }

        file_row_length=0;
        
        while((read = getline(&line, &len, fp)) != -1){
            /* Change tabs to 4 spaces*/
            row_info = (file_row_info *)realloc(row_info, sizeof(file_row_info)*(file_row_length+1));

            if(read > 0 && line[read-1]=='\r'||line[read-1]=='\n') read--;
            char * modified_line = initialize_row(line, read);
            
            row_info[file_row_length].row = modified_line;
            row_info[file_row_length].len = strlen(modified_line) + 1;
            row_info[file_row_length].row[row_info[file_row_length].len-1] = '\0';
            // row_info[file_row_length].row[row_info[file_row_length].len+1] = '\r';
            // row_info[file_row_length]->len = strlen(line);
            // char * p;
            // sprintf(p, "read : %d || len : %d ", read, len);
            // write(STDOUT_FILENO, p, strlen(p));
            // write(STDOUT_FILENO, line, read);
            // write(STDOUT_FILENO, "\r", 2);
            // write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
            file_row_length++;
        }
        // file_row_length--;
        // for(int i=0; i<5; ++i){
        //     char buf[50];
        //     sprintf(buf, "%d line : %d\r\n", i+1, file_col_length[i]);
        //     write(STDOUT_FILENO, buf, strlen(buf));
        // }
        
        fclose(fp);
    #endif

    
}


int read_keypress(void){
    #ifdef _WIN32

    #else
        char buf[4];
        int num_read;
        
        while ((num_read = read(STDIN_FILENO, &buf[0], 1)) != 1) {
            if (num_read == -1) {
                perror("read");
                exit(1);
            }
        }
        if(buf[0]=='\033'){ 
            if(read(STDOUT_FILENO, &buf[1], 1) != 1) return '\033';
            // write(STDOUT_FILENO, "try esc1\r\n", strlen("try esc1\r\n"));
            if(read(STDOUT_FILENO, &buf[2], 1) != 1) return '\033';
            // write(STDOUT_FILENO, "try esc2\r\n", strlen("try esc2\r\n"));

            // if(read(STDOUT_FILENO, &buf[3], 1) != 1) return '\033';
            // return buf[0];

            if(buf[1]=='['){
                switch(buf[2]){
                    case 'A': //Up
                        return UP_ARROW; // \033[A
                    case 'B': //down
                        return DOWN_ARROW; // \033[B
                    case 'C': //right 
                        return RIGHT_ARROW;  // \033[C
                    case 'D': //left
                        return LEFT_ARROW; // \033[D
                    case 'H': //'\033[H' Home
                        return HOME;
                    case 'F': //'\033[F' End
                        return END;
                }
                if(read(STDOUT_FILENO, &buf[3], 1)!=1) return '\033';

                if(buf[3]=='~'){
                    switch(buf[2]){
                        case '1': //'\033[1~' Home
                            return HOME;
                        case '4': //'\033[4~' end
                            return END;
                        case '5': //'\033[5~' page up
                            return PAGE_UP;
                        case '6': //'\033[6~' page down
                            return PAGE_DOWN;
                    }
                }
            }
            return '\033';
        }else{
            return buf[0];
        }
    #endif
    
}

void page_down_draw_line(){
    #ifdef _WIN32
        if(page_down_status == 1){
            for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
                if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                    draw_msg_line(terminal_line);
                }else{
                    int cur_line = terminal_line + cursor_y_out;
                    printf("\033[K");
                    
                    if(row_info[cur_line].len > terminal_col_size) row_info[cur_line].len = terminal_col_size;
                    printf("%s",row_info[cur_line].row);
                    if(terminal_line < terminal_row_size-1){
                        printf("\n");
                }
                } 
            }
            page_down_status = 0;
        }
    #else
        if(page_down_status == 1){
            for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
                if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                    draw_msg_line(terminal_line);
                }else{
                    int cur_line = terminal_line + cursor_y_out;
                    write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
                    if(row_info[cur_line].len > terminal_col_size) row_info[cur_line].len = terminal_col_size;
                    write(STDOUT_FILENO, row_info[cur_line].row, row_info[cur_line].len);
                    if(terminal_line < terminal_row_size-1){
                        write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
                }
                } 
            }
            page_down_status = 0;
        }
    #endif
    
}

void page_up_draw_line(){
    #ifdef _WIN32
        if(page_up_status == 1){
            for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
                if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                    draw_msg_line(terminal_line);
                }else{
                    int cur_line = terminal_line + cursor_y_out;
                    printf("\033[K");
                    if(row_info[cur_line].len > terminal_col_size) row_info[cur_line].len = terminal_col_size;
                    printf("%s",row_info[cur_line].row);
                    if(terminal_line < terminal_row_size-1){
                        printf("\n");
                }
                } 
            }
            page_up_status = 0;
        }
    #else
    if(page_up_status == 1){
        for(int terminal_line=0; terminal_line < terminal_row_size; ++terminal_line){
            if(terminal_line == terminal_row_size - 2 || terminal_line == terminal_row_size -1){
                draw_msg_line(terminal_line);
             }else{
                int cur_line = terminal_line + cursor_y_out;
                write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
                if(row_info[cur_line].len > terminal_col_size) row_info[cur_line].len = terminal_col_size;
                write(STDOUT_FILENO, row_info[cur_line].row, row_info[cur_line].len);
                if(terminal_line < terminal_row_size-1){
                    write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
              }
            } 
        }
        page_up_status = 0;
    }
    #endif

    
}


void move_cursor(int keypress, char * filename){
        #ifdef _WIN32
        if(keypress == UP_ARROW){
            if(cursor_y > 0){
                cursor_y--;
                if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                    cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                }
            }else if (cursor_y==0 && cursor_y_out > 0){
                up_status = 1;
                cursor_y_out--;
                if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                    cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                }
            }
        }
        if(keypress == DOWN_ARROW){
            //0 ~ 18
            if(cursor_y + cursor_y_out != file_row_length-1){
                
                // char buf[30];
                // sprintf(buf, "into");
                // write(STDOUT_FILENO, buf, strlen(buf));

                if(cursor_y < terminal_row_size - 3){
                    if(cursor_y < file_row_length-1){
                        cursor_y++;
                        if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                        cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                        }                
                    }
                }else if(cursor_y == terminal_row_size-3){
                    if(cursor_y_out+cursor_y <= file_row_length){
                        cursor_y_out++;
                        down_status = 1;
                        if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                        cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                        }
                    }
                }
            }
        }
        if(keypress == LEFT_ARROW){
            if(cursor_x > 0){
                cursor_x--;
            }
        }
        if(keypress == RIGHT_ARROW){
            if(filename != NULL){
                if(cursor_x < row_info[cursor_y+cursor_y_out].len -1){
                    cursor_x++;
                }
            }else{
                if(cursor_x < row_info[cursor_y+cursor_y_out].len -1){
                    cursor_x++;
                }
            }
        }
        if(keypress == HOME){
            cursor_x = 0;
        }
        if(keypress == END){
            if(filename != NULL){
                cursor_x = row_info[cursor_y+cursor_y_out].len-1;
            }else{
                cursor_x = row_info[cursor_y+cursor_y_out].len-1;
            }
        }
        if(keypress == PAGE_UP){
            if(cursor_y != 0) {
                cursor_y = 0;
            }else{
                cursor_y_out -= terminal_row_size-2;
                
                if(cursor_y_out < terminal_row_size-2){
                    cursor_y_out = 0;
                }
                page_up_status = 1;
                page_up_draw_line();
            }
        }
            
        if(keypress == PAGE_DOWN){
            if(file_row_length < terminal_row_size - 2){
                cursor_y = file_row_length - 1;
            }else if(cursor_y != terminal_row_size - 3){
                cursor_y = terminal_row_size -3;
            }else{
                cursor_y_out += terminal_row_size-2;
                
                if(cursor_y_out + terminal_row_size > file_row_length){
                    cursor_y_out = file_row_length - terminal_row_size+2;
                }
                page_down_status = 1;
                page_down_draw_line();
            }
        }

        printf("\033[%d;%dH", cursor_y+1, cursor_x+1);
    #else
        if(keypress == UP_ARROW){
            if(cursor_y > 0){
                cursor_y--;
                if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                    cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                }
            }else if (cursor_y==0 && cursor_y_out > 0){
                up_status = 1;
                cursor_y_out--;
                if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                    cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                }
            }
        }
        if(keypress == DOWN_ARROW){
            //0 ~ 18
            if(cursor_y + cursor_y_out != file_row_length-1){
                // char buf[30];
                // sprintf(buf, "into");
                // write(STDOUT_FILENO, buf, strlen(buf));

                if(cursor_y < terminal_row_size - 3){
                    if(cursor_y < file_row_length-1){
                        cursor_y++;
                        if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                        cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                        }                
                    }
                }else if(cursor_y == terminal_row_size-3){
                    if(cursor_y_out+cursor_y <= file_row_length){
                        cursor_y_out++;
                        down_status = 1;
                        if(cursor_x > row_info[cursor_y+cursor_y_out].len-1){
                        cursor_x = row_info[cursor_y+cursor_y_out].len-1;
                        }
                    }
                }
            }
        }
        if(keypress == LEFT_ARROW){
            if(cursor_x > 0){
                cursor_x--;
            }
        }
        if(keypress == RIGHT_ARROW){
            if(filename != NULL){
                if(cursor_x < row_info[cursor_y+cursor_y_out].len -1){
                    cursor_x++;
                }
            }else{
                if(cursor_x < row_info[cursor_y+cursor_y_out].len -1){
                    cursor_x++;
                }
            }
            
        }
        if(keypress == HOME){
            cursor_x = 0;
        }
        if(keypress == END){
            if(filename != NULL){
                cursor_x = row_info[cursor_y+cursor_y_out].len-1;
            }else{
                cursor_x = row_info[cursor_y+cursor_y_out].len-1;
            }
        }
        if(keypress == PAGE_UP){
            if(cursor_y != 0) {
                cursor_y = 0;
            }else{
                cursor_y_out -= terminal_row_size-2;
                
                if(cursor_y_out < terminal_row_size-2){
                    cursor_y_out = 0;
                }
                page_up_status = 1;
                page_up_draw_line();
            }
        }
            
        if(keypress == PAGE_DOWN){
            if(file_row_length < terminal_row_size - 2){
                cursor_y = file_row_length - 1;
            }
            else if(cursor_y != terminal_row_size - 3){
                cursor_y = terminal_row_size -3;
            }else{
                // char buf[30];
                // sprintf(buf, "into");
                // write(STDOUT_FILENO, buf, strlen(buf));
                cursor_y_out += terminal_row_size-2;
                
                if(cursor_y_out + terminal_row_size > file_row_length){
                    cursor_y_out = file_row_length - terminal_row_size+2;
                }
                page_down_status = 1;
                page_down_draw_line();
            }
        }

        char cursor_location[32];
        // write(STDOUT_FILENO, "\033[?25l", strlen("\x1b[?25l"));
        sprintf(cursor_location,"\033[%d;%dH", cursor_y+1, cursor_x+1);
        write(STDOUT_FILENO, cursor_location, strlen(cursor_location));
    #endif
}



/* Screen updates when input something */
void input_file_line(void){
    #ifdef _WIN32
        printf("\033[H");

        for(int terminal_line = 0; terminal_line < terminal_row_size; ++terminal_line){
            printf("\033[K");
            if(terminal_line == terminal_row_size -2 || terminal_line == terminal_row_size -1){
                draw_msg_line(terminal_line);
            }else if(terminal_line > file_row_length){
                if(terminal_line < terminal_row_size - 1){
                    printf("~\n");
                }else{
                    printf("~");
                }
            }else{
                if(row_info[terminal_line+cursor_y_out].len > terminal_col_size) row_info[terminal_line+cursor_y_out].len = terminal_col_size;
                printf("%s",row_info[terminal_line+cursor_y_out].row);
                
                // write(STDOUT_FILENO, buf, strlen(buf));
                if(terminal_line < terminal_row_size - 1){
                    printf("\n");
                }
                printf("\033[K");
            }
        }
    #else
        write(STDOUT_FILENO, "\033[H", strlen("\033[H"));

        for(int terminal_line = 0; terminal_line < terminal_row_size; ++terminal_line){
            write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
            if(terminal_line == terminal_row_size -2 || terminal_line == terminal_row_size -1){
                draw_msg_line(terminal_line);
            }else if(terminal_line > file_row_length){
                if(terminal_line < terminal_row_size - 1){
                    write(STDOUT_FILENO, "~\r\n", strlen("~\r\n"));
                }else{
                    write(STDOUT_FILENO, "~", strlen("~"));
                }
            }else{
                if(row_info[terminal_line+cursor_y_out].len > terminal_col_size) row_info[terminal_line+cursor_y_out].len = terminal_col_size;
                
                write(STDOUT_FILENO, row_info[terminal_line+cursor_y_out].row, row_info[terminal_line+cursor_y_out].len);
                // write(STDOUT_FILENO, buf, strlen(buf));
                if(terminal_line < terminal_row_size - 1){
                    write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
                }
                write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
            }
        }
    #endif
    
}

/*Process backspace*/
void backspace_process(void){
    if(cursor_x >0){
        memmove(&(row_info[cursor_y+cursor_y_out].row[cursor_x-1]), &(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len - 1);
        row_info[cursor_y+cursor_y_out].row = realloc(row_info[cursor_y+cursor_y_out].row, row_info[cursor_y+cursor_y_out].len-1);
        row_info[cursor_y+cursor_y_out].len--;
        input_file_line();
        cursor_x--;
    }else if(cursor_x==0){ //When the cursor is at the very front and is not the first line
        if(cursor_y==0&&cursor_y_out==0){
            //nothing is happened
        }else{
            //Combine the sentence above the current cursor with the sentence where the cursor is currently located
            int pre_len = row_info[cursor_y+cursor_y_out-1].len;

            //expand storage space for the above sentence size
            row_info[cursor_y+cursor_y_out-1].row = realloc(row_info[cursor_y+cursor_y_out-1].row, row_info[cursor_y+cursor_y_out-1].len+row_info[cursor_y+cursor_y_out].len);
            
            //concatenate above sentence with current sentence
            memcpy(&(row_info[cursor_y+cursor_y_out-1].row[row_info[cursor_y+cursor_y_out-1].len-1]), row_info[cursor_y+cursor_y_out].row, row_info[cursor_y+cursor_y_out].len);

            //reallocate storage space
            row_info[cursor_y+cursor_y_out-1].len = row_info[cursor_y+cursor_y_out-1].len+row_info[cursor_y+cursor_y_out].len - 1;

            memmove(&row_info[cursor_y+cursor_y_out], &row_info[cursor_y+cursor_y_out+1], sizeof(file_row_info)*(file_row_length - (cursor_y+cursor_y_out+1)));
            
            // free(row_info[file_row_length-1].row);
            row_info = realloc(row_info, sizeof(file_row_info)*(file_row_length-1));
            file_row_length--;
            
            
            input_file_line();
            cursor_y--; 
            if(row_info[cursor_y+cursor_y_out].len - pre_len < terminal_col_size){
                cursor_x = row_info[cursor_y+cursor_y_out].len - pre_len;
            }else{
                cursor_x = terminal_col_size;
            }
        }
    
    }
}

#ifdef _WIN32
    char * enter_initialize_row(void){
        char * buf = (char*)malloc(sizeof(char)*1);
        buf[0] = '\0';

        return buf;
    }
#endif



/* Enter key process */
void enter_process(void){
    #ifdef __linux__
        if(cursor_x == row_info[cursor_y+cursor_y_out].len-1){//tail
            char * buf = (char*)malloc(sizeof(char)*2);
            buf[0] = ' ';
            buf[1] = '\0';
            // write(STDOUT_FILENO, buf, strlen(buf));
            if(cursor_y + cursor_y_out != file_row_length - 1){
                memmove(&row_info[cursor_y+cursor_y_out+2], &(row_info[cursor_y+cursor_y_out+1]), sizeof(file_row_info) *  (file_row_length - (cursor_y+cursor_y_out)-1));
            }
            row_info[cursor_y+cursor_y_out+1].row = buf;
            row_info[cursor_y+cursor_y_out+1].len = strlen(buf);
            file_row_length++;
            input_file_line();
            // for(int i =0; i < 20; ++i){
            //     char buf2[30];
            //     sprintf(buf2, "%d : %s\r\n", i+1,row_info[i].row);
            //     write(STDOUT_FILENO, buf2, strlen(buf2));
            // }
            cursor_x = 0;
        }else if(cursor_x==0){ //head of
            char * buf = (char*)malloc(sizeof(char)*2);
            buf[0] = ' ';
            buf[1] = '\0';

            // write(STDOUT_FILENO, buf, strlen(buf));
            memmove(&row_info[cursor_y+cursor_y_out+1], &(row_info[cursor_y+cursor_y_out]), sizeof(file_row_info) *  (file_row_length - (cursor_y+cursor_y_out)));
            row_info[cursor_y+cursor_y_out].row = buf;
            row_info[cursor_y+cursor_y_out].len = 1;
            file_row_length++;
            input_file_line();
            // for(int i =0; i < 20; ++i){
            //     char buf2[30];
            //     sprintf(buf2, "%d : %s\r\n", i+1,row_info[i].row);
            //     write(STDOUT_FILENO, buf2, strlen(buf2));
            // }
            cursor_x = 0;
        }else if(cursor_x!=0 && cursor_x!=row_info[cursor_y+cursor_y_out].len-1){ //in the middle of line
            char split_sentence[500];
            char * replace_line=NULL; //restoring the string before enter
            memcpy(split_sentence,&(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len - cursor_x);
            // write(STDOUT_FILENO, split_sentence, strlen(split_sentence));

            char * buf = (char*)realloc(buf, sizeof(char)*(row_info[cursor_y+cursor_y_out].len - cursor_x + 1));
            replace_line = (char*)realloc(replace_line, sizeof(char) * (cursor_x +1));
            memcpy(buf, split_sentence, row_info[cursor_y+cursor_y_out].len - cursor_x);
            // write(STDOUT_FILENO, buf, strlen(buf));
            buf[strlen(split_sentence)] = '\0';


            memmove(&row_info[cursor_y+cursor_y_out+2], &(row_info[cursor_y+cursor_y_out+1]), sizeof(file_row_info) *  (file_row_length - (cursor_y+cursor_y_out)));
            
            
            memcpy(replace_line, row_info[cursor_y+cursor_y_out].row, cursor_x);
            row_info[cursor_y+cursor_y_out].row = replace_line;
            row_info[cursor_y+cursor_y_out].len = strlen(replace_line) + 1;
            // row_info[cursor_y+cursor_y_out].row = (char*)realloc(row_info[cursor_y+cursor_y_out].row, sizeof(char) * cursor_x+1);
            // memcpy(row_info[cursor_y+cursor_y_out].row[cursor_x+1], NULL, (row_info[cursor_y+cursor_y_out].len - cursor_x));
            // row_info[cursor_y+cursor_y_out].row[cursor_x] = '\0';
            // row_info[cursor_y+cursor_y_out].len = cursor_x+1;

            row_info[cursor_y+cursor_y_out+1].row = buf;
            row_info[cursor_y+cursor_y_out+1].len = strlen(buf)+1;
            file_row_length++;
            input_file_line();
            cursor_x = 0;
        }
        if(cursor_y == terminal_row_size - 3){
            cursor_y_out++;
        }else if(cursor_y < terminal_row_size -3){
            cursor_y++;
        }
         
    #else
        row_info = realloc(row_info, sizeof(file_row_info)*(file_row_length + 1));

    /*allocate new line*/
        if(cursor_x == row_info[cursor_y+cursor_y_out].len-1){//tail
            char * buf = (char*)malloc(sizeof(char)*2);
            buf[0] = ' ';
            buf[1] = '\0';
            // write(STDOUT_FILENO, buf, strlen(buf));
            memmove(&row_info[cursor_y+cursor_y_out+2], &(row_info[cursor_y+cursor_y_out+1]), sizeof(file_row_info) *  (file_row_length - (cursor_y+cursor_y_out)-1));
            row_info[cursor_y+cursor_y_out+1].row = buf;
            row_info[cursor_y+cursor_y_out+1].len = strlen(buf);
            file_row_length++;
            input_file_line();
            // for(int i =0; i < 20; ++i){
            //     char buf2[30];
            //     sprintf(buf2, "%d : %s\r\n", i+1,row_info[i].row);
            //     write(STDOUT_FILENO, buf2, strlen(buf2));
            // }
            cursor_x = 0;
        }else if(cursor_x==0){ //head of
            char * buf = (char*)malloc(sizeof(char)*1);
            buf[0] = ' ';
            buf[1] = '\0';
            // write(STDOUT_FILENO, buf, strlen(buf));
            memmove(&row_info[cursor_y+cursor_y_out+1], &(row_info[cursor_y+cursor_y_out]), sizeof(file_row_info) *  (file_row_length - (cursor_y+cursor_y_out)));
            row_info[cursor_y+cursor_y_out].row = buf;
            row_info[cursor_y+cursor_y_out].len = strlen(buf);
            file_row_length++;
            input_file_line();
            // for(int i =0; i < 20; ++i){
            //     char buf2[30];
            //     sprintf(buf2, "%d : %s\r\n", i+1,row_info[i].row);
            //     write(STDOUT_FILENO, buf2, strlen(buf2));
            // }
            cursor_x = 0;
        }else if(cursor_x!=0 && cursor_x!=row_info[cursor_y+cursor_y_out].len-1){ //in the middle of line
            // char buf2[30];
            // sprintf(buf2, "into");
            // write(STDOUT_FILENO, buf2, strlen(buf2));
            char split_sentence[500];
            char * replace_line=NULL; //restoring the string before enter
            char * buf=NULL;

            memcpy(split_sentence,&(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len - cursor_x);
            // write(STDOUT_FILENO, split_sentence, strlen(split_sentence));

            buf = (char*)realloc(buf, sizeof(char)*(row_info[cursor_y+cursor_y_out].len - cursor_x + 1));
            replace_line = (char*)realloc(replace_line, sizeof(char) * (cursor_x +1));
            memcpy(buf, split_sentence, row_info[cursor_y+cursor_y_out].len - cursor_x);
            // write(STDOUT_FILENO, buf, strlen(buf));
            buf[strlen(split_sentence)] = '\0';


            memmove(&row_info[cursor_y+cursor_y_out+2], &(row_info[cursor_y+cursor_y_out+1]), sizeof(file_row_info) *  (file_row_length - (cursor_y+cursor_y_out)));
            
            
            memcpy(replace_line, row_info[cursor_y+cursor_y_out].row, cursor_x);
            row_info[cursor_y+cursor_y_out].row = replace_line;
            row_info[cursor_y+cursor_y_out].len = strlen(replace_line) + 1;
            // row_info[cursor_y+cursor_y_out].row = (char*)realloc(row_info[cursor_y+cursor_y_out].row, sizeof(char) * cursor_x+1);
            // memcpy(row_info[cursor_y+cursor_y_out].row[cursor_x+1], NULL, (row_info[cursor_y+cursor_y_out].len - cursor_x));
            // row_info[cursor_y+cursor_y_out].row[cursor_x] = '\0';
            // row_info[cursor_y+cursor_y_out].len = cursor_x+1;

            row_info[cursor_y+cursor_y_out+1].row = buf;
            row_info[cursor_y+cursor_y_out+1].len = strlen(buf)+1;
            file_row_length++;
            input_file_line();
            cursor_x = 0;
        }

        if(cursor_y == terminal_row_size - 3){
            cursor_y_out++;
        }else if(cursor_y < terminal_row_size -3){
            cursor_y++;
        }
    #endif

    

}

/*input character given arguments*/
void draw_character(int c){
    #ifdef _WIN32
        row_info[cursor_y+cursor_y_out].row = realloc(row_info[cursor_y+cursor_y_out].row, sizeof(char)*(row_info[cursor_y+cursor_y_out].len+1));
        memmove(&(row_info[cursor_y+cursor_y_out].row[cursor_x+1]), &(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len +1);
        row_info[cursor_y+cursor_y_out].row[cursor_x] = c;
        row_info[cursor_y+cursor_y_out].len +=1;
        input_file_line();
        cursor_x++;
    #else
        row_info[cursor_y+cursor_y_out].row = realloc(row_info[cursor_y+cursor_y_out].row, sizeof(char)*(row_info[cursor_y+cursor_y_out].len+1));
        memmove(&(row_info[cursor_y+cursor_y_out].row[cursor_x+1]), &(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len +1);
        row_info[cursor_y+cursor_y_out].row[cursor_x] = c;
        row_info[cursor_y+cursor_y_out].len +=1;
        input_file_line();
        cursor_x++;
    #endif
    
}

/*input charater given no arguments*/
void draw_character_newfile(int c){
    #ifdef _WIN32   
        row_info = (file_row_info *)realloc(row_info, sizeof(file_row_info)*(file_row_length+1));

        row_info[cursor_y+cursor_y_out].row = realloc(row_info[cursor_y+cursor_y_out].row, sizeof(char)*(row_info[cursor_y+cursor_y_out].len+1));
        memmove(&(row_info[cursor_y+cursor_y_out].row[cursor_x+1]), &(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len +1);
        row_info[cursor_y+cursor_y_out].row[cursor_x] = c;
        row_info[cursor_y+cursor_y_out].len +=1;
        input_file_line();
        cursor_x++;
    #elif __APPLE__
        // row_info = (file_row_info *)realloc(row_info, sizeof(file_row_info)*(file_row_length+1));
        row_info[cursor_y+cursor_y_out].row = realloc(row_info[cursor_y+cursor_y_out].row, sizeof(char)*(row_info[cursor_y+cursor_y_out].len+2));
        memmove(&(row_info[cursor_y+cursor_y_out].row[cursor_x+1]), &(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len + 1);
        row_info[cursor_y+cursor_y_out].row[cursor_x] = c;
        row_info[cursor_y+cursor_y_out].len +=1;
        input_file_line();
        cursor_x++;
    #else
        char *new_row = realloc(row_info[cursor_y+cursor_y_out].row, row_info[cursor_y+cursor_y_out].len + 1);
        
        row_info[cursor_y+cursor_y_out].row = new_row;
         // row_info = (file_row_info *)realloc(row_info, sizeof(file_row_info)*(file_row_length+1));
        row_info[cursor_y+cursor_y_out].row = realloc(row_info[cursor_y+cursor_y_out].row, sizeof(char)*(row_info[cursor_y+cursor_y_out].len+2));
        // memmove(&(row_info[cursor_y+cursor_y_out].row[cursor_x+1]), &(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len + 1);
        memmove(&(row_info[cursor_y+cursor_y_out].row[cursor_x+1]), &(row_info[cursor_y+cursor_y_out].row[cursor_x]), row_info[cursor_y+cursor_y_out].len - cursor_x);
        row_info[cursor_y+cursor_y_out].row[cursor_x] = c;
        row_info[cursor_y+cursor_y_out].len +=1;
        input_file_line();
        cursor_x++;
    #endif
    
}

int save_read_keypress(void){
    #ifdef _WIN32
        int c;

        c= getch();

        switch(c){
            case ESC: return ESC;
            case ENTER: return ENTER;
            case BACK_SPACE: return BACK_SPACE;
            default:
                return c;
        }

    #else
        char buf;

        int num_read;

        while ((num_read = read(STDIN_FILENO, &buf, 1)) != 1) {
            if (num_read == -1) {
                perror("read");
                exit(1);
            }
        }

        if(buf=='\033') return ESC;
        else if(buf=='\r') return ENTER;
        else if(buf==BACK_SPACE)return BACK_SPACE;
        else return buf;
    #endif
   
}

void save_draw_msg_line(char * save_string, int len){
    #ifdef _WIN32
        char save_msgbar[300];
        char clear[100];
        char relocation[100];

        printf("\033[%dH", terminal_row_size);
        printf("\033[K");
        printf("file name(Enter to Save/ESC to Cancel) : %s", save_string);
        printf("\033[%d;%dH", terminal_row_size, len+42);
        printf("\033[K");
    #else
        char save_msgbar[300];
        char clear[100];
        char relocation[100];

        sprintf(relocation,"\033[%dH", terminal_row_size);
        write(STDOUT_FILENO, relocation, strlen(relocation));
        write(STDOUT_FILENO, "\033[K", strlen("\033[K"));

        sprintf(save_msgbar,"file name(Enter to Save/ESC to Cancel) : %s",save_string);
        write(STDOUT_FILENO, save_msgbar, strlen(save_msgbar));

        sprintf(relocation,"\033[%d;%dH", terminal_row_size, len+42);
        write(STDOUT_FILENO, relocation, strlen(relocation));
        write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
    #endif
    
}

void save_file(void){
    if(filename==NULL){
        char * save_string =NULL;
        int len=0;
        save_draw_msg_line("", 0);
        while(1){
            int c;
            c = save_read_keypress();

            if(c==ESC){
                input_file_line();
                break;
            }else if(c==ENTER){
                if(save_string!=NULL){
                    filename = save_string;
                    save_file();
                    // input_file_line();
                    quit_status=0;
                    break;
                }
            }else if(c==BACK_SPACE){
                if(len > 0){
                // write(STDOUT_FILENO, "bs", strlen("bs"));
                save_string = (char*)realloc(save_string, sizeof(char)*len-1);
                len--;
                save_draw_msg_line(save_string, len);
            }
            }else{
                len++;
                save_string = (char*)realloc(save_string, sizeof(char)*len);
                save_string[len-1] = c;

                save_draw_msg_line(save_string, len);
            }

        }
    }else{
        FILE * fp = fopen(filename, "wt");
    
        if(fp==NULL){
            perror("error : failed to open file");
            exit(1);
        }

        for(int line=0; line<file_row_length; ++line){
            #ifdef __linux__
                row_info[line].row[row_info[line].len-1] = '\n';
                fputs(row_info[line].row, fp);
                row_info[line].row[row_info[line].len-1] = '\0';
            #else
                row_info[line].row = realloc(row_info[line].row, row_info[line].len+1);
                row_info[line].row[row_info[line].len-1] = '\n';
                fputs(row_info[line].row, fp);
                row_info[line].row[row_info[line].len-1] = '\0';
            #endif
           
        }
        quit_status=0;

        char save_msgbar[300];
        char relocation[100];

        #ifdef _WIN32
            printf("\033[%dH", terminal_row_size);
            printf("\033[K");
            printf("Saved Successfully");
        #else
            sprintf(relocation,"\033[%dH", terminal_row_size);
            write(STDOUT_FILENO, relocation, strlen(relocation));
            write(STDOUT_FILENO, "\033[K", strlen("\033[K"));

            sprintf(save_msgbar, "Saved Successfully");
            write(STDOUT_FILENO, save_msgbar, strlen(save_msgbar));
        #endif

        fclose(fp);
    }    
}

int search_read_keypress(void){
    #ifdef _WIN32
        int c;

        c = getch();

        switch(c){
            case ESC:
                return ESC;
            case ENTER:
                return ENTER;
            case BACK_SPACE:
                return BACK_SPACE;
            default:
                return c;
        }

    #else
        char buf;

        int num_read;

        while ((num_read = read(STDIN_FILENO, &buf, 1)) != 1) {
            if (num_read == -1) {
                perror("read");
                exit(1);
            }
        }

        if(buf=='\033') return ESC;
        else if(buf=='\r') return ENTER;
        else if(buf==BACK_SPACE)return BACK_SPACE;
        else return buf;
    #endif
    
}

void search_draw_msg_line(char * search_string, int len){
    #ifdef _WIN32
        printf("\033[%dH", terminal_row_size);
        printf("\033[K");
        printf("Search(Enter to search / ESC to cancel) : %s",search_string);
        printf("\033[%d;%dH", terminal_row_size, len+43);
        printf("\033[K");
    #else
        char search_msgbar[300];
        char clear[100];
        char relocation[100];
        // sprintf(clear, "\033[%dK", terminal_row_size);

        sprintf(relocation,"\033[%dH", terminal_row_size);
        write(STDOUT_FILENO, relocation, strlen(relocation));
        write(STDOUT_FILENO, "\033[K", strlen("\033[K"));

        sprintf(search_msgbar,"Search(Enter to search / ESC to cancel) : %s",search_string);
        write(STDOUT_FILENO, search_msgbar, strlen(search_msgbar));

        sprintf(relocation,"\033[%d;%dH", terminal_row_size, len+43);
        write(STDOUT_FILENO, relocation, strlen(relocation));
        write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
    #endif
    
    // write(STDOUT_FILENO, relocation, strlen(relocation));

}

int KMP(const char * row, const char * pattern, int row_len, int pattern_len){
    int * next = (int *)malloc((pattern_len+1)* sizeof(int));

    /*initialize array*/
    for(int i=0; i<pattern_len+1; ++i){
        next[i]=0;
    }

    for(int i=1; i<pattern_len; ++i){
        int j = next[i];

        while(j > 0 && pattern[j] != pattern[i]){
            j = next[j];
        }

        if( j>0 || pattern[j]==pattern[i]){
            next[i+1] = j+1;
        }
    }

    for (int i = 0, j = 0; i <row_len; i++) {
        if (*(row + i) == *(pattern + j)) {
            if (++j == pattern_len) {
                 return (i - j + 1);
            }
        } else if (j > 0) {
            j = next[j];
            i--;
        }
    }
}

void while_search_draw_line(int cur_row, int cur_col){

    #ifdef _WIN32
        printf("\033[H");
        
        for(int terminal_line=0; terminal_line<terminal_row_size-2; ++terminal_line){
            printf("\033[K");
            // if(terminal_line == 0){

            // }else 
            
            if(terminal_line+cur_row > file_row_length){
                if(terminal_line < terminal_row_size - 1){
                    printf("\n");
                }else{
                    printf("~");
                }
            }else{
                if(row_info[terminal_line+cur_row].len > terminal_col_size) row_info[terminal_line+cur_row].len = terminal_col_size;
                printf("%s",row_info[terminal_line + cur_row].row);

                if(terminal_line < terminal_row_size - 3){
                    printf("\n");
                }
                printf("\033[K");
            }
        }
        printf("\033[%d;%dH", 0, cur_col+1);
    #else
        write(STDOUT_FILENO, "\033[H", strlen("\033[H"));

        for(int terminal_line=0; terminal_line<terminal_row_size-2; ++terminal_line){
            write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
            // if(terminal_line == 0){

            // }else 
            
            if(terminal_line+cur_row > file_row_length){
                if(terminal_line < terminal_row_size - 1){
                    write(STDOUT_FILENO, "~\r\n", strlen("~\r\n"));
                }else{
                    write(STDOUT_FILENO, "~", strlen("~"));
                }
            }else{
                if(row_info[terminal_line+cur_row].len > terminal_col_size) row_info[terminal_line+cur_row].len = terminal_col_size;

                write(STDOUT_FILENO, row_info[terminal_line + cur_row].row, row_info[terminal_line + cur_row].len);

                if(terminal_line < terminal_row_size - 3){
                    write(STDOUT_FILENO, "\r\n", strlen("\r\n"));
                }
                write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
            }
        }
        char buf[30];
        sprintf(buf, "\033[%d;%dH", 0, cur_col+1);
        write(STDOUT_FILENO, buf, strlen(buf));
    #endif
    
}

int while_search_read_keypress(void){
    #ifdef _WIN32
        int c;
        int d;
        c = getch();

        switch(c){
            case 224:
                d = getch();      
                switch(d){
                    case UP_ARROW: return UP_ARROW;
                    case DOWN_ARROW: return DOWN_ARROW;
                    case RIGHT_ARROW: return RIGHT_ARROW;
                    case LEFT_ARROW: return LEFT_ARROW;
                    default:
                        break;
                }
            default:
                return ESC;
        }

    #else
        // if(buf=='\033') return ESC;
        // else if(buf=='\r') return ENTER;
        // else if(buf==BACK_SPACE)return BACK_SPACE;
        // else return buf;

        char buf[4]; 
        int num_read;
        
        while ((num_read = read(STDIN_FILENO, &buf[0], 1)) != 1) {
            if (num_read == -1) {
                perror("read");
                exit(1);
            }
        }
        if(buf[0]=='\033'){ 
            if(read(STDOUT_FILENO, &buf[1], 1) != 1) return '\033';
            if(read(STDOUT_FILENO, &buf[2], 1) != 1) return '\033';

            if(buf[1]=='['){
                switch(buf[2]){
                    case 'A': //up
                        return UP_ARROW; // \033[A
                    case 'B': //down
                        return DOWN_ARROW; // \033[B
                    case 'C': //right 
                        return RIGHT_ARROW;  // \033[C
                    case 'D': //left
                        return LEFT_ARROW; // \033[D
                }
                if(read(STDOUT_FILENO, &buf[3], 1)!=1) return '\033';
            }
            return '\033';
        }else{
            return '\033';
        }
    #endif
    
}

/*process Ctrl-f*/
void search_process(void){
    #ifdef _WIN32
        input_file_line();
        char *search_string=NULL;
        int len = 0;
        search_draw_msg_line("",0);
        while(1){
            int c;
            c = search_read_keypress();

            if(c==ESC) {
                input_file_line();
                break;
            }
            else if(c==ENTER){
                search_info * location = NULL;
                int size = 0;
                for(int i=0; i<file_row_length; ++i){
                    if(strstr(row_info[i].row, search_string)!=NULL) {
                        size++;
                        location = (search_info*)realloc(location, sizeof(search_info)*size);
                        location[size-1].row_location = i;
                        location[size-1].col_location = KMP(row_info[i].row , search_string, row_info[i].len, len);
                    }
                }
                if(size ==0){

                }else{
                    int search=0;
                    while_search_draw_line(location[0].row_location, location[0].col_location);

                    while(1){
                        int key = while_search_read_keypress();
                        if(key==DOWN_ARROW || key==RIGHT_ARROW){
                            if(search == size-1){
                                search=0;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }else{
                                search++;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }
                        }else if(key==UP_ARROW || key==LEFT_ARROW){
                            if(search==0){
                                search = size-1;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }else{
                                search--;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }
                        }else{
                            break;
                        }
                    }
                    free(location);
                }
                
            }else if(c==BACK_SPACE){
                if(len > 0){
                    // write(STDOUT_FILENO, "bs", strlen("bs"));
                    search_string = (char*)realloc(search_string, sizeof(char)*len-1);
                    len--;
                    search_draw_msg_line(search_string, len);
                }
            }else{
                len++;
                search_string = (char*)realloc(search_string, sizeof(char)*len);
                search_string[len-1] = c;
                
                search_draw_msg_line(search_string,len);
            }
        }

        free(search_string);
    #else
        // write(STDOUT_FILENO, "INTO Ctrl+f\r\n", strlen("INTO Ctrl+f\r\n"));
        input_file_line();
        char *search_string=NULL;
        int len = 0;
        search_draw_msg_line("",0);
        while(1){
            int c;
            c = search_read_keypress();

            if(c==ESC) {
                input_file_line();
                break;
            }
            else if(c=='\r'){
                search_info * location = NULL;
                int size = 0;
                for(int i=0; i<file_row_length; ++i){
                    if(strstr(row_info[i].row, search_string)!=NULL) {
                        size++;
                        location = (search_info*)realloc(location, sizeof(search_info)*size);
                        location[size-1].row_location = i;
                        location[size-1].col_location = KMP(row_info[i].row , search_string, row_info[i].len, len);
                    }
                }
                if(size == 0){
                    
                }else{
                    int search=0;
                    while_search_draw_line(location[0].row_location, location[0].col_location);
                    while(1){
                        int key = while_search_read_keypress();
                        if(key==DOWN_ARROW || key==RIGHT_ARROW){
                            if(search == size-1){
                                search=0;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }else{
                                search++;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }
                        }else if(key==UP_ARROW || key==LEFT_ARROW){
                            if(search==0){
                                search = size-1;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }else{
                                search--;
                                while_search_draw_line(location[search].row_location, location[search].col_location);
                            }
                        }else{
                            break;
                        }
                    }
                }
                    free(location);
            }else if(c==BACK_SPACE){
                if(len > 0){
                    // write(STDOUT_FILENO, "bs", strlen("bs"));
                    search_string = (char*)realloc(search_string, sizeof(char)*len-1);
                    len--;
                    search_draw_msg_line(search_string, len);
                }
            }else{
                len++;
                search_string = (char*)realloc(search_string, sizeof(char)*len);
                search_string[len-1] = c;
                
                search_draw_msg_line(search_string,len);
            }
        }

        free(search_string);
    #endif
    
}

void shortcut_key(void){
    #ifdef _WIN32
        int c;
        int d;
        // c = read_keypress();
        
        
        c = getch();
        
        switch(c){
            case 224:
                d = getch();      
                switch(d){
                    case UP_ARROW:
                    case DOWN_ARROW:
                    case RIGHT_ARROW:
                    case LEFT_ARROW:
                    case HOME:
                    case END:
                    case PAGE_UP:   
                    case PAGE_DOWN:
                        move_cursor(d, filename);
                        break;
                    default:
                        break;
                }
                break;
            case CTRL_Q:
                if(quit_status==0||quit_status==2){
                    system(CLEAR);
                    printf("\033[H");
                    exit(0);
                }else if(quit_status == 1){
                    //print message bar
                    // sprintf(clear, "\033[%dK", terminal_row_size);
                    printf("\033[%d;%dH", terminal_row_size, 0);
                    printf("\033[K");
                    printf("Force quit : Press Ctrl-f 1 more");
                    quit_status++;
                }
                break;
            case CTRL_S:
                save_file();
                break;
            case CTRL_F:
                search_process();
                break;
            case BACK_SPACE:
                backspace_process();
                quit_status=1;
                break;
            case ENTER:
                enter_process();
                quit_status=1;
                break;
            default:
                if(filename){
                    draw_character(c);
                    quit_status=1;
                }else{
                    draw_character_newfile(c);
                    quit_status=1;
                }
               break;
        }
    #else
        int c;
        c = read_keypress();

        switch(c){
            case CTRL_KEY('q'):
                if(quit_status==0||quit_status==2){
                    // char buf[30];
                    // sprintf(buf, "quit status : %d", quit_status);
                    // write(STDOUT_FILENO, buf, strlen(buf));
                    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios_quit);
                    system(CLEAR);
                    write(STDOUT_FILENO, "\033[H", strlen("\033[H"));
                    // for(int i = 0; i< terminal_row_size; ++i){
                    //     char buf[30];
                    //     sprintf(buf,"\033[%d;%dH", i, 0);
                    //     write(STDOUT_FILENO, "\033[K", strlen("\033[K"));
                    // }
                    // write(STDOUT_FILENO, "\033[H", strlen("\033[H"));
                    free(row_info);
                    exit(0);
                }else if(quit_status == 1){
                    //print message bar
                    char quit_msgbar[300];
                    char buf[30];
                    // sprintf(clear, "\033[%dK", terminal_row_size);

                    sprintf(buf,"\033[%dH", terminal_row_size);
                    write(STDOUT_FILENO, buf, strlen(buf));
                    write(STDOUT_FILENO, "\033[K", strlen("\033[K"));

                    sprintf(quit_msgbar,"Force quit : Press Ctrl-f 1 more");
                    write(STDOUT_FILENO, quit_msgbar, strlen(quit_msgbar));
                    quit_status++;
                }
                
            case UP_ARROW:
            case DOWN_ARROW:
            case RIGHT_ARROW:
            case LEFT_ARROW:
            case HOME:
            case END:
            case PAGE_UP:   
            case PAGE_DOWN:
                move_cursor(c, filename);
                break;
            case CTRL_KEY('s'): //save
                save_file();
                break;
            case CTRL_KEY('f'): //search
                search_process();
                break;
            case BACK_SPACE: //127bite
                backspace_process();
                quit_status=1;
                break;
            case ENTER:
                enter_process();
                quit_status=1;
                break;
            
            default:
                if(filename){
                    draw_character(c);
                    quit_status=1;
                }else{
                    draw_character_newfile(c);
                    quit_status=1;
                }
                break;
        }
    #endif
   
}


int main(int argc, char *argv[]){
    system(CLEAR);
    #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
        DWORD mode;


        //mode &= ~ENABLE_ECHO_INPUT;

        if (!GetConsoleScreenBufferInfo(hConsole, &csbiInfo)) {
            fprintf(stderr, "Failed to get console screen buffer info. Error code: %lu\n", GetLastError());
            return 1;
        }

        terminal_col_size = csbiInfo.srWindow.Right - csbiInfo.srWindow.Left + 1;
        terminal_row_size = csbiInfo.srWindow.Bottom - csbiInfo.srWindow.Top + 1;

        row_info=NULL;
        if(argc >=2){
            filename = argv[1];

            open_file(argv[1]);
            draw_file_line();
            printf("\033[%d;%dH", cursor_y, cursor_x);

            while(1){
                if(cursor_y + cursor_y_out < file_row_length-1){
                    down_update_file_line();
                }
                up_update_file_line();
                shortcut_key();

                printf("\033[%d;%dH", terminal_row_size-2, 0);
                draw_msg_line(terminal_row_size-2);
                printf("\x1B[0m\033[%d;%dH", cursor_y+1, cursor_x+1);
            }
            
            //printf("%d", file_row_length); 

           free(row_info);
        }else{
            open_new_terminal();
            row_info = (file_row_info *)realloc(row_info, sizeof(file_row_info)*(file_row_length+1));
            file_row_length++;
            row_info[0].row = (char *)malloc(sizeof(char));
            row_info[0].row[0] = '\0';
            row_info[0].len = 1;
            draw_msg_line(terminal_row_size-2);
            draw_msg_line(terminal_row_size-1);
            printf("\033[%d;%dH", 1, 1);
            while(1){
                shortcut_key();

                printf("\033[%d;%dH", terminal_row_size-2, 0);
                draw_msg_line(terminal_row_size-2);
                printf("\x1B[0m\033[%d;%dH", cursor_y+1, cursor_x+1);
            }
        }
    #else
        /*chage terminal setting*/
    tcgetattr(STDIN_FILENO, &orig_termios);
    tcgetattr(STDIN_FILENO, &orig_termios_quit);
    struct termios set = orig_termios;
    /*
    ICANON: Whenever the user presses a key, that keystroke is processed immediately.
    IXON: Ctrl-S is read as 19 bytes and Ctrl-Q is read as 17 bytes.
    IEXTEN: Disable default terminal Ctrl-O keys
    ICRNL: Make the Enter key read as 13 bytes
    */
    set.c_iflag &= ~(IXON | ICRNL);
    set.c_lflag &= ~(ECHO | ICANON | IEXTEN );

    /* Apply changed terminal properties */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &set);


    if (get_window_size(&terminal_row_size, &terminal_col_size) != 0) exit(1); 
    
    // char buf[150];
    // sprintf(buf, "termianl_col_size : %d\r\nterminal_row_size : %d", terminal_col_size, terminal_row_size);
    // write(STDOUT_FILENO, buf, strlen(buf));
    row_info = NULL;

    if(argc >= 2){
        // open_new_terminal();ㅠ
        filename = argv[1];
        open_file(argv[1]);

        // char buf[32];
        // sprintf(buf,"열 size : %d", file_row_length);
        // write(STDOUT_FILENO, buf, strlen(buf));
        draw_file_line();

        while(1){
            if(cursor_y + cursor_y_out <= file_row_length-1){
                down_update_file_line();
            }
            up_update_file_line();
            shortcut_key();


            //update message bar
            char buf[32];
            sprintf(buf,"\033[%d;%dH", terminal_row_size-1, 0);
            write(STDOUT_FILENO, buf, strlen(buf));
            draw_msg_line(terminal_row_size-2);
            // write(STDOUT_FILENO, "\033[?25l", strlen("\x1b[?25l"));
            sprintf(buf,"\x1B[0m\033[%d;%dH", cursor_y+1, cursor_x+1);
            write(STDOUT_FILENO, buf, strlen(buf));
        }
    }else{
        open_new_terminal();
        row_info = (file_row_info *)malloc(sizeof(file_row_info)*(file_row_length+1));
        file_row_length++;
        row_info[0].row = (char *)malloc(sizeof(char));
        row_info[0].row[0] = '\0';
        row_info[0].len = 1;

        while(1){
            shortcut_key();
            //update message bar
            char buf[32];
            sprintf(buf,"\033[%d;%dH", terminal_row_size-1,0);
            write(STDOUT_FILENO, buf, strlen(buf));
            draw_msg_line(terminal_row_size-2);  
            // // write(STDOUT_FILENO, "\033[?25l", strlen("\x1b[?25l"));
            sprintf(buf,"\x1B[0m\033[%d;%dH", cursor_y+1, cursor_x+1);
            write(STDOUT_FILENO, buf, strlen(buf));
        }   
    }
    #endif 
    
    return 0;
}



 