#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SAMPLES 1024
#define MAX_FIELD_LENGTH 256

int main(int argc, char *argv[]) {
    int argn = atoi(argv[2]);
    char input[MAX_FIELD_LENGTH];
    int sample_number;
    char field[MAX_FIELD_LENGTH];
    double value;

    FILE *gnuplot_pipe = popen("gnuplot -persist", "w");
    if (gnuplot_pipe == NULL) {
        perror("Error opening gnuplot pipe");
        exit(1);
    }

    fprintf(gnuplot_pipe, "set title 'Sample Plot'\n");
    fprintf(gnuplot_pipe, "set xlabel 'Sample Number'\n");
    fprintf(gnuplot_pipe, "set ylabel 'Value'\n");
    fprintf(gnuplot_pipe, "plot 'data.txt' with lines\n");

    pclose(gnuplot_pipe);

    return 0;
}