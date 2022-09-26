#include "keyboard.h"
#include "io.h"
#include "ioqueue.h"
#include "console.h"
#include "interrupt.h"

#define DATA_PORT 0x60
#define COMMAND_PORT 0x64

#define ESC 27
#define ENTER 0x0A  // NOTE:原本为0x0D(回车)，全部转为0x0A(换行)
#define L_CTRL_MAKE 0x1D
#define L_CTRL_BREAK 0x9D
#define L_SHFT_MAKE 0x2A
#define L_SHFT_BREAK 0xAA
#define R_SHFT_MAKE 0x36
#define R_SHFT_BREAK 0xB6
#define L_ALT_MAKE  0x38
#define L_ALT_BREAK  0xB8
#define CAPS_MAKE   0x3A

/* 
 * 把键盘扫描码转成ASCII码
 * TODO 处理剩余按键
 */
char keymap[256][2] = {{0, 0}, {ESC, ESC}, {'1', '!'}, {'2', '@'}, {'3', '#'}, 
                    {'4', '$'}, {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'},
                    {'9', '('}, {'0', ')'}, {'-', '_'}, {'=', '+'}, {'\b', '\b'},
                    {'\t', '\t'}, {'q', 'Q'}, {'w', 'W'}, {'e', 'E'}, {'r', 'R'}, 
                    {'t', 'T'}, {'y', 'Y'}, {'u', 'U'}, {'i', 'I'}, {'o', 'O'}, 
                    {'p', 'P'}, {'[', '{'}, {']', '}'}, {ENTER, ENTER}, {L_CTRL_MAKE, L_CTRL_MAKE},
                    {'a', 'A'}, {'s', 'S'}, {'d', 'D'}, {'f', 'F'}, {'g', 'G'}, 
                    {'h', 'H'}, {'j', 'J'}, {'k', 'K'}, {'l', 'L'}, {';', ':'}, 
                    {'\'', '\"'}, {'`', '~'}, {L_SHFT_MAKE, L_SHFT_MAKE}, {'\\', '|'}, {'z', 'Z'},
                    {'x', 'X'}, {'c', 'C'}, {'v', 'V'}, {'b', 'B'}, {'n', 'N'},
                    {'m', 'M'}, {',', '<'}, {'.', '>'}, {'/', '?'}, {R_SHFT_MAKE, R_SHFT_MAKE},
                    {'*', '*'}, {L_ALT_MAKE, L_ALT_MAKE}, {' ', ' '}, {CAPS_MAKE, CAPS_MAKE}};

static struct ioqueue keyboard_buf;

bool shift_status = false;
bool ctrl_status = false;
bool alt_status = false;
bool caps_status = false;

// 中断处理程序太长了
static void keyboard_handler() {
    uint8_t code = inb(DATA_PORT);

    if (code == 0xE0) {
        return; /* TODO 以后添加处理 */
    }

    if (code & 0x80) {
        switch (code) {
            case L_SHFT_BREAK : shift_status = false; break;
            case R_SHFT_BREAK : shift_status = false; break;
            case L_CTRL_BREAK : ctrl_status = false; break;
            case L_ALT_BREAK  : alt_status = false; break;
            default: break; /* TODO 其他断码以后处理 */
        }
        return; 
    }

    switch (code) {
        case L_SHFT_MAKE : shift_status = true; break;
        case R_SHFT_MAKE : shift_status = true; break;
        case L_CTRL_MAKE : ctrl_status = true; break;
        case L_ALT_MAKE  : alt_status = true; break;
        case CAPS_MAKE  : caps_status = !caps_status; break;
        default: {
            ASSERT(code <= 0x3a);
            char c = keymap[code][shift_status];
            if (caps_status) {
                if (c >= 'A' && c <= 'Z') {
                    c += 'a' - 'A';
                } else if (c >= 'a' && c <= 'z') {
                    c -= 'a' - 'A';
                }
            }
            if (c == '\b') {
                ioqueue_pop_back_char(&keyboard_buf);
            } else {
                ioqueue_putchar(&keyboard_buf, c);
            }
            console_printk("%c", c); // 回显
        }
    }  
    // TODO ctrl+c怎么办 
}

// TODO 全部改为行缓冲
char keyboard_read() {
    return ioqueue_getchar(&keyboard_buf);
}

// 传入-1代表读到换行符
int keyboard_readline(char* dest, int count) {
    return ioqueue_getline(&keyboard_buf, dest, count);
}


void keyboard_init() {
    printk("keyboard init start\n");
    ioqueue_init(&keyboard_buf, 100);
    register_handler(0x21, keyboard_handler);
    printk("keyboard init end\n");
}