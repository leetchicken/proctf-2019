#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>

#define BUFLEN 100
#define BUFFMT "%99s"

#define MAXFRAMES 10
#define MAXFRAMEPOLYS 25
#define MAXPOLYPOINTS 50

#define POINTS_IN_LINE 80
#define LINES 25

#define MAXPOLYSALLOCATED 50


struct point {
    unsigned char x;
    unsigned char y;
};

typedef struct point* polyline;
typedef polyline frame[MAXFRAMEPOLYS];

int flags_dir_fd = 0;
char* username = 0;
static unsigned int polys_allocated = 0;


void print_prompt() {
    struct dirent* entry = 0;
    char buf[BUFLEN] = {0};
    int readed = 0;

    printf("Welcome!\n");

    printf("Available logins:\n");
    if(flags_dir_fd >= 0) {
        int logins_fd = openat(flags_dir_fd, "logins.txt", O_RDONLY);
        if (logins_fd >= 0) {
            while ((readed = read(logins_fd, buf, BUFLEN)) > 0) {
                write(1, buf, readed);
            }
        } else {
            printf("none\n");
        }
    } else {
        printf("web\n");
    }
}

int char_is_good(char c) {
    if (c >= 'a' && c <= 'z') {
        return 1;
    } else if (c >= 'A' && c <= 'Z') {
        return 1;
    } else if (c >= '0' && c <= '9') {
        return 1;
    } else if (c == '_' || c == '=') {
        return 1;
    } else {
        return 0;
    }
}

int line_is_good(char *line) {
    int i;
    for(i = 0; i < BUFLEN && line[i] != 0; i += 1) {
        if (!char_is_good(line[i])) {
            return 0;
        }
    }
    return i < BUFLEN;
}

int get_good_word(FILE * file, char *buf) {
    fscanf(file, BUFFMT, buf);
    return line_is_good(buf);
}

int try_log_in(char* login, int fd) {
    char good_password[BUFLEN];
    char password[BUFLEN];

    FILE *password_file = fdopen(fd, "r");
    fscanf(password_file, BUFFMT, good_password);
    fclose(password_file);

    printf("password: ");
    if ((!get_good_word(stdin, password)) || strcmp(password, good_password) != 0) {
        printf("bad password\n");
        return 0;        
    }
    return 1;
}

int try_register(char *login, int fd) {
    char secret[BUFLEN];
    char new_password[BUFLEN];
    int login_list_fd;

    printf("new password: ");
    if (!get_good_word(stdin, new_password)) {
        printf("bad new password\n");
        return 0;
    }

    printf("new secret: ");
    if (!get_good_word(stdin, secret)) {
        printf("bad new secret\n");
        return 0;
    }

    login_list_fd = openat(flags_dir_fd, "logins.txt", O_WRONLY|O_CREAT|O_APPEND, 0600);
    if (login_list_fd < 0) {
        printf("unable to save your login\n");
        return 0;
    }
    FILE *password_file = fdopen(fd, "w");
    fprintf(password_file, "%s %s\n", new_password, secret);
    fclose(password_file);

    FILE *logins_file = fdopen(login_list_fd, "a");
    fprintf(logins_file, "%s\n", login);
    fclose(logins_file);

    return 1;
}

char* authenticate() {
    char login[BUFLEN];

    printf("Please log in or register\n");
    printf("login: ");

    if (!get_good_word(stdin, login)) {
        printf("bad login\n");
        return 0;
    }

    if (flags_dir_fd >= 0) {
        int fd = openat(flags_dir_fd, login, O_RDONLY);
        if (fd >= 0) {
            if(!try_log_in(login, fd)) {
                return 0;
            }
        } else {
            fd = openat(flags_dir_fd, login, O_WRONLY|O_CREAT, 0600);
            if(fd >= 0) {
                if(!try_register(login, fd)) {
                    return 0;
                }
            } else {
                printf("unable to save your password and secret\n");
                return 0;
            }
        }
    }

    char* good_login = malloc(BUFLEN);
    strcpy(good_login, login);
    return good_login;
}


int frame_is_empty(polyline* f) {
    for(unsigned int i = 0; i < MAXFRAMEPOLYS-1; i += 1) {
        if(f[i] != 0) {
            return 0;
        }
    }
    return 1;
}

int count_points(polyline f) {
    int ans = 0;
    #pragma clang loop unroll(disable)
    while(ans < MAXPOLYPOINTS && !(f[ans].x == 0 && f[ans].y == 0)) {
        ans += 1;
    }
    return ans;
}

void handle_info(frame* frames) {
    #pragma clang loop unroll(disable)
    for(unsigned frame_idx = 0; frame_idx < MAXFRAMES; frame_idx += 1) {
        if(frame_is_empty(frames[frame_idx])) {
            continue;
        }
        printf("frame %d:\n", frame_idx);
        #pragma clang loop unroll(disable)
        for(unsigned poly_idx = 0; poly_idx < MAXFRAMEPOLYS-1; poly_idx += 1) {
            if (frames[frame_idx][poly_idx] == 0 ) {
                continue;
            }
            printf(" poly %d, points %d\n", poly_idx, count_points(frames[frame_idx][poly_idx]));
        }
    }
}

void handle_new_poly(frame *frames, unsigned int frame_idx) {
    #pragma clang loop unroll(disable)
    for(unsigned int idx = 0; idx < MAXFRAMEPOLYS; idx += 1) {
        if(frames[frame_idx][idx] == 0) {
            if(polys_allocated >= MAXPOLYSALLOCATED) {
                printf("err, too much polys\n");
                return;
            } 

            frames[frame_idx][idx] = malloc(BUFLEN);
            polys_allocated += 1;

            printf("ok, idx=%d\n", idx);
            return;
        }
    }
    printf("no more space\n");
}

void del_poly_if_unused(frame *frames, polyline f) {
    #pragma clang loop unroll(disable)
    for (unsigned int frame_idx=0; frame_idx < MAXFRAMES; frame_idx += 1) {
        if(frame_is_empty(frames[frame_idx])) {
            continue;
        }
        #pragma clang loop unroll(disable)
        for (unsigned int poly_idx=0; 
             poly_idx < MAXFRAMEPOLYS && frames[frame_idx][poly_idx] != 0; 
             poly_idx += 1) {
            if (frames[frame_idx][poly_idx] == f) {
                printf("ok\n");
                return;
            }
        }
    }
    printf("ok, poly is not on needed anymore, deleting it\n");
    free(f);
    polys_allocated -= 1;
}

void handle_del_poly(frame *frames, unsigned int frame_idx) {
    unsigned int poly_idx;
    scanf("%d", &poly_idx);

    if(poly_idx >= MAXFRAMEPOLYS) {
        printf("bad poly index\n");
        return;
    }

    polyline p = frames[frame_idx][poly_idx];
    if(p == 0) {
        printf("ok, was empty\n");
        return;
    }
    frames[frame_idx][poly_idx] = 0;
    del_poly_if_unused(frames, p);
}

void handle_copy_poly(frame *frames, unsigned int frame_idx) {
    unsigned int poly_from_idx;
    unsigned int dest_frame_idx;

    scanf("%d %d", &poly_from_idx, &dest_frame_idx);

    if(dest_frame_idx >= MAXFRAMES) {
        printf("bad frame index\n");
        return;
    }

    if(poly_from_idx >= MAXFRAMEPOLYS) {
        printf("bad poly index\n");
        return;
    }

    polyline p = frames[frame_idx][poly_from_idx];
    if (p == 0) {
        printf("no such poly\n");
        return;
    }

    #pragma clang loop unroll(disable)
    for(unsigned int idx = 0; idx < MAXFRAMEPOLYS; idx += 1) {
        if(frames[dest_frame_idx][idx] == 0) {
            frames[dest_frame_idx][idx] = p;
            printf("ok, idx=%d\n", idx);
            return;
        }
    }
    printf("no more space\n");
}

void clear_frame(frame *frames, unsigned int frame_idx) {
    #pragma clang loop unroll(disable)    
    for(unsigned int idx = 0; idx < MAXFRAMEPOLYS; idx += 1) {
        polyline p = frames[frame_idx][idx];
        if(p != 0) {
            frames[frame_idx][idx] = 0;
            del_poly_if_unused(frames, p);
        }
    }
}

void handle_clear_frame(frame *frames, unsigned int frame_idx) {
    clear_frame(frames, frame_idx);
    printf("ok\n");
}


void handle_copy_frame(frame *frames, unsigned int frame_idx) {
    unsigned int dest_frame_idx;
    scanf("%d", &dest_frame_idx);

    if(dest_frame_idx >= MAXFRAMES) {
        printf("bad frame index\n");
        return;
    }

    clear_frame(frames, dest_frame_idx);

    #pragma clang loop unroll(disable)    
    for(unsigned int idx = 0; idx < MAXFRAMEPOLYS; idx += 1) {
        frames[dest_frame_idx][idx] = frames[frame_idx][idx];
    }
    printf("ok\n");
}

void handle_add_point(frame *frames, unsigned int frame_idx) {
    unsigned int poly_idx;
    unsigned int pos;
    unsigned int x;
    unsigned int y;
    scanf("%d %d %d %d", &poly_idx, &pos, &x, &y);

    if(poly_idx >= MAXFRAMEPOLYS) {
        printf("bad poly index\n");
        return;
    }

    if(pos >= MAXPOLYPOINTS) {
        printf("bad pos\n");
        return;
    }

    if(x >= 256 || y >= 256 || (x == 0 && y == 0)) {
        printf("bad coordinates\n");
        return;
    }

    polyline p = frames[frame_idx][poly_idx];
    if(p == 0) {
        printf("no such polyline\n");
        return;
    }

    p[pos].x = x;
    p[pos].y = y;

    printf("ok\n");
}

void handle_get_point(frame *frames, unsigned int frame_idx) {
    unsigned int poly_idx;
    unsigned int pos;
    scanf("%d %d", &poly_idx, &pos);

    if(poly_idx >= MAXFRAMEPOLYS) {
        printf("bad poly index\n");
        return;
    }

    if(pos >= MAXPOLYPOINTS) {
        printf("bad pos\n");
        return;
    }

    polyline p = frames[frame_idx][poly_idx];
    if(p == 0) {
        printf("no such poly\n");
        return;
    }

    if (p[pos].x == 0 && p[pos].y == 0) {
        printf("no such point\n");
        return;
    }
    printf("ok, x=%d y=%d\n", p[pos].x, p[pos].y);
}

void handle_del_point(frame *frames, unsigned int frame_idx) {
    unsigned int poly_idx;
    unsigned int pos;
    scanf("%d %d", &poly_idx, &pos);

    if(poly_idx >= MAXFRAMEPOLYS) {
        printf("bad poly index\n");
        return;
    }

    if(pos >= MAXPOLYPOINTS) {
        printf("bad pos\n");
        return;
    }

    polyline p = frames[frame_idx][poly_idx];
    if(p == 0) {
        printf("no such poly\n");
        return;
    }

    p[pos].x = 0;
    p[pos].y = 0;
    printf("ok\n");
}

void draw_line_low(unsigned char *d, unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2) {
    int dx = x2 - x1;
    int dy = abs(y2 - y1);
    int sy = (y2 - y1) < 0 ? -1: 1;
    int D = 2*dy - dx;
    int y = y1;
    #pragma clang loop unroll(disable)
    for(int x = x1; x <= x2; x += 1) {
        if(y >= 0 && y<LINES && x >= 0 && x < POINTS_IN_LINE) {
            d[y*POINTS_IN_LINE + x] = 1;
        }
        if(D > 0) {
            y = y + sy;
            D = D - 2*dx;
        }
        D = D + 2*dy;
    }
}

void draw_line_high(unsigned char *d, unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2) {
    int dx = abs(x2 - x1);
    int dy = y2 - y1;
    int sx = (x2 - x1) < 0 ? -1 : 1;
    int D = 2*dx - dy;
    int x = x1;
    #pragma clang loop unroll(disable)
    for(int y = y1; y <= y2; y += 1) {
        if(y >= 0 && y<LINES && x >= 0 && x < POINTS_IN_LINE) {
            d[y*POINTS_IN_LINE + x] = 1;
        }
        if(D > 0) {
            x = x + sx;
            D = D - 2*dy;
        }
        D = D + 2*dx;
    }
}

void draw_line(unsigned char *d, unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2) {
    if (abs(y2 - y1) < abs(x2 - x1)) {
        if (x1 > x2) {
            draw_line_low(d, x2, y2, x1, y1);
        } else {
            draw_line_low(d, x1, y1, x2, y2);
        }
    } else {
        if (y1 > y2) {
            draw_line_high(d, x2, y2, x1, y1);
        } else {
            draw_line_high(d, x1, y1, x2, y2);
        }
    }
}

void draw_poly(struct point* points, unsigned char *draw_buf) {
    memset(draw_buf, 0, LINES*POINTS_IN_LINE);

    if(!(points[0].x == 0 && points[0].y == 0)) {
        #pragma clang loop unroll(disable)
        for(int l = 0; l + 1 < MAXPOLYPOINTS; l += 1) {
            if (points[l+1].x == 0 && points[l+1].y == 0) {
                draw_line(draw_buf, points[l].x, points[l].y, points[0].x, points[0].y);
                break;
            } else {
                draw_line(draw_buf, points[l].x, points[l].y, points[l+1].x, points[l+1].y);
            }
        }
    }

    int replace_made;
    do {
        replace_made = 0;

        #pragma clang loop unroll(disable)
        for(int y=0; y<LINES; y+=1) {
            #pragma clang loop unroll(disable)
            for(int x=0; x<POINTS_IN_LINE; x+=1) {
                if (draw_buf[y*POINTS_IN_LINE+x] != 0) {
                    continue;
                }
                if (x == 0 || draw_buf[y*POINTS_IN_LINE+(x-1)] == 2) {
                    draw_buf[y*POINTS_IN_LINE+x] = 2;
                    replace_made = 1;
                }
                if (x == (POINTS_IN_LINE-1) || draw_buf[y*POINTS_IN_LINE+(x+1)] == 2) {
                    draw_buf[y*POINTS_IN_LINE+x] = 2;
                    replace_made = 1;
                }
                if (y == 0 || draw_buf[(y-1)*POINTS_IN_LINE+x] == 2) {
                    draw_buf[y*POINTS_IN_LINE+x] = 2;
                    replace_made = 1;
                }
                if (y == (LINES-1) || draw_buf[(y+1)*POINTS_IN_LINE+x] == 2) {
                    draw_buf[y*POINTS_IN_LINE+x] = 2;
                    replace_made = 1;
                }
            }
        }
    } while(replace_made);

    #pragma clang loop unroll(disable)    
    for(int y=0; y<LINES; y+=1) {
        #pragma clang loop unroll(disable)
        for(int x=0; x<POINTS_IN_LINE; x+=1) {
            draw_buf[y*POINTS_IN_LINE+x] = (draw_buf[y*POINTS_IN_LINE+x] == 0);
        }
    }
}


void render(frame *frames, unsigned int frame_idx) {
    unsigned char rendered[LINES*POINTS_IN_LINE] = {0};
    unsigned char draw_buf[LINES*POINTS_IN_LINE];

    #pragma clang loop unroll(disable)
    for(int poly=0; poly < MAXFRAMEPOLYS; poly += 1) {
        if(frames[frame_idx][poly] == 0) {
            continue;
        }
        draw_poly(frames[frame_idx][poly], draw_buf);
        
        #pragma clang loop unroll(disable)
        for(int y=0; y<LINES; y+=1) {
            #pragma clang loop unroll(disable)
            for(int x=0; x<POINTS_IN_LINE; x+=1) {
                if (draw_buf[y*POINTS_IN_LINE+x] != 0) {
                    rendered[y*POINTS_IN_LINE+x] = 'A' + poly;
                }
            }
        }
    }

    #pragma clang loop unroll(disable)
    for(int y=0; y<LINES; y+=1) {
        #pragma clang loop unroll(disable)
        for(int x=0; x<POINTS_IN_LINE; x+=1) {
            if(rendered[y*POINTS_IN_LINE+x] == 0) {
                printf(" ");
            } else {
                printf("%c", rendered[y*POINTS_IN_LINE+x]);
            }
        }
        printf("\n");
    }    
}

void handle_render(frame *frames, unsigned int frame_idx, char* username) {
    char password[BUFLEN] = "the_real_password_is_unknown";
    char secret[BUFLEN] = "the_real_secret_is_unknown";
    if (flags_dir_fd >= 0) {
        int fd = openat(flags_dir_fd, username, O_RDONLY);
        if(fd >= 0) {
            FILE *password_file = fdopen(fd, "r");
            fscanf(password_file, BUFFMT BUFFMT, password, secret);
            fclose(password_file);
        } 
    }
    printf("secret: %s\n", secret);
    render(frames, frame_idx);
}

void handle_help() {
    printf("set_frame <frame_idx>\n");
    printf("info\n");
    printf("\n");
    printf("new_poly\n");
    printf("del_poly <poly_idx>\n");
    printf("copy_poly <poly_idx> <dest_frame_idx>\n");
    printf("\n");
    printf("clear_frame\n");
    printf("copy_frame <dest_frame_idx>\n");
    printf("\n");
    printf("add_point <poly_idx> <pos> <x> <y>\n");
    printf("get_point <poly_idx> <pos>\n");
    printf("del_point <poly_idx> <pos>\n");
    printf("\n");
    printf("render\n");
}

void handle_command(char* username, frame* frames, unsigned int frame_idx, char* command) {
    if(strcmp(command, "help") == 0) {
        handle_help();
    } else if(strcmp(command, "info") == 0) {
        handle_info(frames);
    } else if(strcmp(command, "new_poly") == 0) {
        handle_new_poly(frames, frame_idx);
    } else if(strcmp(command, "del_poly") == 0) {
        handle_del_poly(frames, frame_idx);
    } else if(strcmp(command, "copy_poly") == 0) {
        handle_copy_poly(frames, frame_idx);
    } else if(strcmp(command, "clear_frame") == 0) {
        handle_clear_frame(frames, frame_idx);
    } else if(strcmp(command, "copy_frame") == 0) {
        handle_copy_frame(frames, frame_idx);
    } else if(strcmp(command, "add_point") == 0) {
        handle_add_point(frames, frame_idx);
    } else if(strcmp(command, "get_point") == 0) {
        handle_get_point(frames, frame_idx);
    } else if(strcmp(command, "del_point") == 0) {
        handle_del_point(frames, frame_idx);
    } else if(strcmp(command, "render") == 0) {
        handle_render(frames, frame_idx, username);
    } else {
        printf("unknown command, try 'help'\n");
    }
}

void handle_client(char *username) {
    char command[BUFLEN] = {0};
    unsigned int frame_idx = 0;

    frame* frames = malloc(MAXFRAMES*sizeof(frame));
    memset(frames, 0, MAXFRAMES*sizeof(frame));
    
    while(!feof(stdin)) {
        printf("> ");
    
        if (!get_good_word(stdin, command)) {
            printf("bad command, try 'help'\n");
            continue;
        }

        if (strcmp(command, "set_frame") == 0) {
            unsigned int new_frame_idx;
            scanf("%d", &new_frame_idx);

            if(new_frame_idx >= MAXFRAMES) {
                printf("bad frame index\n");
                continue;
            }
            frame_idx = new_frame_idx;
            printf("ok\n");
            continue;
        } else if (strcmp(command, "quit") == 0) {
            printf("good bye\n");
            break;
        }
        handle_command(username, frames, frame_idx, command);
    }
}


int main() {
    setbuf(stdout, NULL);

    flags_dir_fd = open("flags", O_RDONLY|O_DIRECTORY);
    print_prompt();

    username = authenticate();
    if (username == 0) {
        return 1;
    }

    handle_client(username);
    return 0;
}
