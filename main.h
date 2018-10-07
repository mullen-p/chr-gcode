#ifndef __H_CHR_DATA
#include "chr-data.h"
#endif

//#define CHR_DEBUG

#define __H_CHR_MAIN

#ifndef __H_CHR_DICT
#include "dict.h"
#endif

#ifndef __H_CHR_GCODE
#include "gcode.h"
#endif

struct chr_line {
    struct chr_char **characters;
    int string_length;
    float line_width;
    char *string;
};

struct output {
    struct chr_line **line;
    int line_count;
    float max_width;
    float total_height;
    float line_height;
};

enum x_align {
    LEFT = 0, CENTER, RIGHT
};

enum y_align {
    TOP = 0, MIDDLE, BOTTOM
};

struct gcode_settings {
    float offset_x;
    float offset_y;

    float x_length;
    float y_length;

    float rotation;
    float scale;

    int feed_rate;
    int plunge_rate;
    int sub_options;

    float rapid_z;
    float plunge_from;
    float cut_to;

    enum x_align align_x;

    enum x_align x_offset_align;
    enum y_align y_offset_align;

    char* sub_id;
};

void set_scale(struct chr_file*,struct output*, struct gcode_settings*);
void setting_defaults(struct gcode_settings*);
int produce_gcode(struct output *, struct gcode_settings*);

void delete_chr(struct chr_file*);
void delete_lines(struct output*);

int add_to_line(struct chr_line*, struct chr_char*, char );
int add_line(struct output*, struct chr_line*);

int init_output(struct output *output);

float get_min_height(struct chr_line*);
float get_max_height(struct chr_line*);
void get_line_width(struct chr_line*);