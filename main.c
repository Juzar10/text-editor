#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/** data **/

struct EditorConfig {
    struct termios original_termios;
};

struct EditorConfig E;

// Error handling
void die(const char *s) {
    perror(s);
    exit(1);
}

/** terminal **/
void rollbackRawMode() {
    if ( tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1 ) die("tssetattr");
}

void enableRawMode() {
    // This gets the terminal structure, a data strucutre if you will.
    if ( tcgetattr(STDIN_FILENO, &E.original_termios) == -1 ) die( "tsgetattr");
    atexit(rollbackRawMode);

    struct termios raw = E.original_termios;
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

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/** input **/
void editorProcessKey() {
    const char c = editorReadKey();

    switch(c) {
        case (CTRL_KEY('q')):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        default:
            break;
    }
}

/** output **/
void editorDrawRows() {
    for (int y = 0; y < 24; y++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);


    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}


/** main **/
int main() {

    enableRawMode();

    while (1) {
        editorRefreshScreen();
        editorProcessKey();
    }
    return 0;
}
