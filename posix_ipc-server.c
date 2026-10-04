#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

float my_atof(const char *str, int *consumed) {
    float res = 0.0f;
    float frac = 0.1f;
    bool in_frac = false;
    int i = 0;

while (str[i] == ' ') i++;

int start = i;
while (str[i] != '\0' && str[i] != ' ' && str[i] != '\n') {
        if (str[i] == '.') {
            in_frac = true;
        } else if (str[i] >= '0' && str[i] <= '9') {
            if (!in_frac) {
                res = res * 10.0f + (str[i] - '0');
            } else {
                res = res + (str[i] - '0') * frac;
                frac *= 0.1f;
            }
        }
        i++;
    }
    *consumed = i;
    return (i == start) ? -1.0f : res;
}

int my_ftoa(float n, char *buf) {
    int pos = 0;
    int ipart = (int)n;
    float fpart = n - (float)ipart;

    int temp = ipart;
    int digits = 0;
    if (temp == 0) digits = 1;
    while (temp > 0) { digits++; temp /= 10; }

    for (int i = digits - 1; i >= 0; i--) {
        buf[pos + i] = (ipart % 10) + '0';
        ipart /= 10;
    }
    pos += digits;
    buf[pos++] = '.';

    for (int i = 0; i < 6; i++) {
        fpart *= 10.0f;
        int fdigit = (int)fpart;
        buf[pos++] = fdigit + '0';
        fpart -= fdigit;
    }
    buf[pos++] = '\n';
    buf[pos] = '\0';
    return pos;
}

int main(int argc, char **argv) {
    char buf[4096];
    ssize_t bytes;

    if (argc < 2) {
        const char msg[] = "error: file name not provided\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    int file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open file\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';

        float result = 0.0f;
//      int consumed = 0;
//      int total_offset = 0;
        bool first_num = true;
        bool div_by_zero = false;

        char *ptr = buf;
        while (*ptr != '\0' && *ptr != '\n') {
            while (*ptr == ' ') ptr++;
            if (*ptr == '\0' || *ptr == '\n') break;

            int step = 0;
            float num = my_atof(ptr, &step);
            if (step == 0) break;
            ptr += step;

            if (first_num) {
                result = num;
                first_num = false;
            } else {
                if (num == 0.0f) {
                    div_by_zero = true;
                    break;
                }
                result /= num;
            }
        }

        if (div_by_zero) {
            const char err_msg[] = "ERROR: Division by zero!\n";
            write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
            write(STDOUT_FILENO, "ZERO\n", 5);
            close(file);
            exit(2);
        }

        char out_buf[64];
        int out_len = my_ftoa(result, out_buf);
        write(file, out_buf, out_len);

        write(STDOUT_FILENO, "OK\n", 3);
    }

    close(file);
    return 0;
}