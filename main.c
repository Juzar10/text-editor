#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

struct termios original_termios;

// Error handling
void die(const char *s) {
    perror(s);
    exit(1);
}

void rollbackRawMode() {
    if ( tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios) == -1 ) die("tssetattr");
}

void enableRawMode() {
    // This gets the terminal structure, a data strucutre if you will.
    if ( tcgetattr(STDIN_FILENO, &original_termios) == -1 ) die( "tsgetattr");
    atexit(rollbackRawMode);

    struct termios raw = original_termios;
    /**
     * These are the different flags that we are turning off,
     * I don't know what all of these flags does yet
     * But they are all a way of making the terminal into the RAW mode,
     * which means having vim like features.
     */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int main() {

    enableRawMode();

    while (1) {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
        if (c == CTRL_KEY('q')) break;
    }
    return 0;
}
