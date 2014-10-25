#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

// debug macros {{{
// will have effect if macro EBUG is defined
// the cool thing is that defining macro by gcc compiler looks like this:
// gcc -Dmacro
// therefore, defining macro EBUG looks like this:
// gcc -DEBUG
// which is cool :)
#define dbg if(0)
#ifdef EBUG
    #undef dbg
    #define dbg if(1)
#endif

#define epf(...) fprintf(stderr, __VA_ARGS__)
#define dpf(...) dbg epf(__VA_ARGS__)
//}}}

const int DEFAULT_IN_PORT = 12345;
const int DEFAULT_OUT_PORT = 12345;
const int MESSAGE_LENGTH = 700;

// socket descriptors
int in_fd;
int out_fd;
// terminal settings
struct termios term_init;
struct termios term; // actual settings
// descriptor set
fd_set set;

void cleaning_the_mess();
void err_assert(const int err_status, char *message);


void err_assert(const int err_status, char *message){
    // check whether `err_status` is below 0. If so, then 
    // prints out error message and ends program.
    if(message == NULL) message = "";
    
    if(err_status < 0){
        perror(message);
        cleaning_the_mess();
        exit(1);
    }
}

void print_usage(){
    fprintf(stderr, "Usage: ./talker.bin ip_address [port] [in_port]\n");
    fflush(stderr);
}

void create_sockets(char *_ip, char *_port, char *_in_port){
    // binds descriptors `in_fd` and `out_fd`.
    assert(_ip != NULL);
    int err_status;
    
    struct sockaddr_in out;
    socklen_t out_len = sizeof(out);
    // setting out_fd with (almost) all possible fail checks
    {
        out.sin_family = AF_INET;
        //out.sin_port = htons(DEFAULT_OUT_PORT);
        err_status = inet_pton(AF_INET, _ip, &(out.sin_addr)); // inet_addr is deprecated
        
        if(err_status == 0){
            fprintf(stderr, "Error: First argument isn't valid IP address!\n");
            print_usage();
            exit(1);
        }
        else if(err_status == -1){
            perror("Error init_out()");
            exit(1);
        }
        
        int out_port;
        if(_port != NULL){
            out_port = atoi(_port);
            if(out_port <= 0){
                fprintf(stderr, "Error: Second argument isn't valid port!\n");
                print_usage();
                exit(1);
            }
        }
        else{
            out_port = DEFAULT_OUT_PORT;
        }

        out.sin_port = htons(out_port); // errors here?
        
        out_fd = socket(AF_INET, SOCK_DGRAM, 0);
        err_status = connect(out_fd, (struct sockaddr*) &out, out_len);
        err_assert(err_status, "Error while setting out_fd");
    }

    struct sockaddr_in in;
    socklen_t in_len = sizeof(in);
    // setting in_fd with (almost) all possible checks
    {
        in.sin_family = AF_INET;
        in.sin_addr.s_addr = INADDR_ANY;
        
        int in_port;
        if(_in_port != NULL){
            in_port = atoi(_in_port);
            if(in_port <= 0){
                fprintf(stderr, "Error: Third argument isn't valid port!\n");
                fprintf(stderr, "%s -> %d\n", _in_port, in_port);
                print_usage();
                exit(1);
            }
        }
        else{
            in_port = DEFAULT_IN_PORT;
        }
        
        in.sin_port = htons(in_port); // errors here?

        in_fd = socket(AF_INET, SOCK_DGRAM, 0);
        err_status = bind(in_fd, (struct sockaddr*)&in, in_len);
        err_assert(err_status, "Error while setting in_fd");
    }
}

void save_terminal_settings(){
    // saves initial terminal settings to `term_init`.
    int err_status = tcgetattr(0, &term_init);
    err_assert(err_status, "Failed to save initial terminal setttings");

    err_status = tcgetattr(0, &term);
    err_assert(err_status, NULL);
}

void restore_terminal_settings(){
    // restores initial terminal settings from `term_init`.
    int err_status = tcsetattr(0, TCSANOW, &term_init);
    err_assert(err_status, "Failed to restore initial terminal settings");
}

void set_canon_on(){
    // Toggle canonical mode on.
    term.c_lflag |= ICANON;

    int err_status = tcsetattr(0, TCSANOW, &term);
    err_assert(err_status, "");
}

void set_canon_off(){
    // Toggle canonical mode off.
    term.c_lflag &= ~ICANON;
    int err_status = tcsetattr(0, TCSANOW, &term);
    err_assert(err_status, "");
}

int read_from_input(char *message){
    // reads input from stdin and saves it in `message`.
    // maximum length is `MESSAGE_LENGTH`.
    // which means: if message is longer, then it will be interpreted
    // as several messages (probably without proper EOL symbols).
    int c = read(0, message, MESSAGE_LENGTH);
    err_assert(c, "Failed to read from keyboard");
    message[c] = '\0';
    return c+1;
}

void read_from_network(char *message){
    // reads input from network and saves it in `message`.
    // maximum length is `MESSAGE_LENGTH`.
    // which means: if incoming message is longer, then it will be shown
    // as several messages (probably without proper EOL symbols).
    int c = read(in_fd, message, MESSAGE_LENGTH);
    err_assert(c, "Failed to read data from network");
    message[c] = '\0';
}

void show_message(char *message){
    // prints the `message` to stdin.
    int err_status = printf(">> %s", message);
    err_assert(err_status, "");
}

void send_message(char *message, int l){
    // send message to `out_fd`.
    if(l <= 2) return; // empty string won't be sent (\n, \0, ?)     

    int c = 0;
    int sent = 0;

    while(sent < l){
        c = send(out_fd, message + sent, l - sent, 0);
        err_assert(c, "Failed to send data");
        sent += c;
    }
}

void exit_handler(int sig){
    // ends program.
    assert(sig == SIGINT);
    cleaning_the_mess();
    printf("Exiting...\n");
    exit(0);
}

void set_signal_handler(){
    // Ctrl + C for exit
    void (*prev_handler) (int);
    prev_handler = signal(SIGINT, exit_handler);
    err_assert(prev_handler == SIG_ERR ? -1 : 47, "Failed to set signal handler");
}

void cleaning_the_mess(){
    printf("\nCleaning the mess...\n");
    restore_terminal_settings();
    close(in_fd);
    close(out_fd);
}

int main(int argc, char **args){
    printf("Talker is starting...\n");
    
    save_terminal_settings();
    
    switch(argc){
        case 2: create_sockets(args[1], NULL, NULL);
                break;
        case 3: create_sockets(args[1], args[2], NULL);
                break;
        case 4: create_sockets(args[1], args[2], args[3]);
                break;
        default:print_usage();
                exit(1);
    }
    // sockets are in_fd, out_fd
    
    set_signal_handler();
    printf("Ctrl + C to exit.\n");
    
    // main cycle
    set_canon_off();
    
    int is_writing = 0;
    
    while(1){
        FD_ZERO(&set);
        FD_SET(0, &set);
        if(!is_writing){
            // to prevent 100% CPU load on endless while-loop (because select will end immediatedly)
            FD_SET(in_fd, &set);
        }

        int err_status = select(in_fd + 1, &set, NULL, NULL, NULL);
        err_assert(err_status, "Failed to set select");
        
        if(FD_ISSET(0, &set) && !is_writing){
            is_writing = 1;
            set_canon_on();
        }
        else if(FD_ISSET(0, &set) && is_writing){
            set_canon_off();
            is_writing = 0;
            
            char message[MESSAGE_LENGTH+47]; // this won't kill any babies, but will surely prevent overflow.
                                             // on the other hand, this may look like coder doesn't know
                                             // anything about C strings. This is not true :)
            int l = read_from_input(message);
            send_message(message, l);
        }
        else if(FD_ISSET(in_fd, &set) && !is_writing){
            char message[MESSAGE_LENGTH+47];
            read_from_network(message);
            show_message(message);
        }
    }

    cleaning_the_mess();
    return 0;
}
