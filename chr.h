#include <stdio.h>

#define __H_CHR_DATA

#define INTERNAL __attribute__ ((visibility ("hidden")))

enum block_type {
    END = 0, MOVE, DRAW, SCAN
};

#define SIG_LEN 4
#define SHORT_NAME_LEN 4
#define MAX_DESC_LEN 250
#define WORD 2
#define BYTE 1

struct chr_file {
    FILE *file;
    char sig[SIG_LEN + 1];              // File-type signature
    char desc[MAX_DESC_LEN];            // The desc given
    int head_size;                      // The size of the whole header (From 0x0)
    char font_name[SHORT_NAME_LEN + 1]; // The internal 4 letter name of the font
    int font_size;                      // The size of the font file - header size
    int maj_ver;                        // Major version
    int min_ver;                        // Minor version

    int num_char;                       // How many chars in this font file
    int first_char;                     // ASCII value of the first char in the font (usually ' ')

    int stroke_offset;                  // Offset to stroke definitions
    int scan_flag;                      // Normally 0?

    int cap_height;                     // Distance from origin to top of capital
    int baseline_height;                // Distance from origin to baseline
    int dec_height;                     // Distance from origin to bottom descender

    int char_offset_begin;              // Offsets to character definitions
    int width_offset_begin;             // Width table for the characters
};

struct coord {
    float x;
    float y;
};

struct chr_char_segment {
    struct coord coord;
    enum block_type type;
};

struct chr_char {
    int width;
    struct chr_char_segment **segments;
    int segment_count;
};

extern int get_char_data(struct chr_file*, char, struct chr_char*);
extern int get_header(struct chr_file*);
extern int get_font_data(struct chr_file* in_file);

INTERNAL int get_char_segment(struct chr_file*, struct chr_char_segment*);