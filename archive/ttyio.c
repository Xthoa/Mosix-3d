#include "types.h"
#include "string.h"
#include "sprintf.h"
#include "baseapi.h"

void dlentry(){
    
}

Export int tty_putchar(char c){
    return writefd(1, &c, 1);
}
Export int tty_puts(char* s){
    return writefd(1, s, strlen(s));
}
Export int tty_getchar(){
    char c;
    readfd(0, &c, 1);
    return c;
}
Export int tty_getchar_wb(){
    char c = tty_getchar();
    tty_putchar(c);
    return c;
}
Export int tty_gets(char* dst, size_t max){
    for(int i = 0; i <= max; i++){
        char c = tty_getchar_wb();
        if(c == '\b'){
            if(i >= 1){
                dst[i - 1] = 0;
                i -= 2;
            }
        }
        else dst[i] = c;
        if(c == '\n'){
            dst[++i] = '\0';
            return i;
        }
    }
    return max;
}
Export int tty_gets_nowb(char* dst, size_t max){
    for(int i = 0; i <= max; i++){
        char c = tty_getchar();
        if(c == '\b'){
            if(i >= 1){
                dst[i - 1] = 0;
                i -= 2;
            }
        }
        else dst[i] = c;
        if(c == '\n'){
            dst[++i] = '\0';
            return i;
        }
    }
    return max;
}

Export int tty_printf(char* fmt, ...){
    char* buf = kmalloc(256);
    va_list args;
    int n;
    va_start(args, fmt);
    n = vsprintf(buf, fmt, args);
    va_end(args);
    tty_puts(buf);
    kfree(buf);
    return n;
}
