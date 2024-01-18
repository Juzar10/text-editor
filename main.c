#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/** data **/

struct EditorConfig {
    int screenrows;
    int screencols;
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
    for (int y = 0; y < E.screenrows; y++) {
        write(STDOUT_FILENO, "~", 1);
        if (y < E.screenrows - 1) {
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }
}

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);


    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
    return 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        editorReadKey();
        return getCursorPosition(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void initWindowSize() {
    if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
        die("getWindowSize");
    }
}

/** main **/
int main() {

    enableRawMode();
    initWindowSize();

    // It's endless loop because we are listening for CTRL + Q character to end the program.
    while (1) {
        editorRefreshScreen();
        editorProcessKey();
    }
    return 0;
}
