#include <stdlib.h>
#include <string.h>

#ifndef __H_CHR_DATA

#include "chr.h"

#endif

int get_font_data(struct chr_file *in_file) {

    // If the file is not a stroke file type (+) error out
    if (fgetc(in_file->file) != 0x2b) {
        return -1;
    }

    if(!fread(&in_file->num_char, WORD, 1, in_file->file) > 0) {
        return -1;
    }

    fseek(in_file->file, BYTE, SEEK_CUR); // Skip over 1 undefined byte
    in_file->first_char = fgetc(in_file->file);

    if(! fread(&in_file->stroke_offset, WORD, 1, in_file->file) > 0) {
        return -1;
    }

    in_file->scan_flag = fgetc(in_file->file); // Not sure what this is, doesn't seem to be used
    in_file->cap_height = fgetc(in_file->file);
    in_file->baseline_height = fgetc(in_file->file);
    in_file->dec_height = (signed char) fgetc(in_file->file);

    fseek(in_file->file, 5 * BYTE, SEEK_CUR); // Skip over 5 undefined bytes

    fgetpos(in_file->file, (fpos_t *) &in_file->char_offset_begin);
    in_file->width_offset_begin = (in_file->num_char * 2) + in_file->char_offset_begin;

#ifdef CHR_DEBUG
    printf("DEBUG:: Char Data Offset Begins: 0x%02x\n", in_file->char_offset_begin);
#endif

    return 0;
}

int get_header(struct chr_file *in_file) {
    const char file_sig[] = {'P', 'K', '\x08', '\x08'};
    int in_char, font_desc_length = 0;

    if(!fread(in_file->sig, SIG_LEN, 1, in_file->file) > 0) {
        return -1;
    }

    in_file->sig[SIG_LEN] = '\0';

    if (strncmp(in_file->sig, file_sig, SIG_LEN) != 0) {
        return -1;
    }

    fseek(in_file->file, 4 * BYTE, SEEK_CUR); // Skip over 'BGI '

    while ((in_char = fgetc(in_file->file)) != 0x1A && font_desc_length < (MAX_DESC_LEN - 1)) {
        in_file->desc[font_desc_length] = (char) in_char;
        font_desc_length++;
    }

    /* Continue to the end, even if not storing */
    if (font_desc_length == (MAX_DESC_LEN - 1)) {
        while (fgetc(in_file->file) != 0x1a);
    }

    in_file->desc[font_desc_length] = '\0';

    if(!fread(&in_file->head_size, WORD, 1, in_file->file) > 0) {
        return -1;
    }

    if(!fread(&in_file->font_name, SHORT_NAME_LEN, 1, in_file->file) > 0) {
        return -1;
    }

    in_file->font_name[SHORT_NAME_LEN] = '\0';

    if(!fread(&in_file->font_size, WORD, 1, in_file->file) > 0) {
        return -1;
    }

    in_file->maj_ver = fgetc(in_file->file);
    in_file->min_ver = fgetc(in_file->file);

    fseek(in_file->file, in_file->head_size, SEEK_SET); // Skip to end of the header padding

    return 0;
}

int get_char_data(struct chr_file *file, char ascii_code, struct chr_char *char_data) {
    int offset = file->char_offset_begin + (WORD * (ascii_code - file->first_char));
    int current_max = 1;
    int width_offset = file->width_offset_begin + (ascii_code - file->first_char);
    int char_offset = 0;

    /* init with no segments present */
    char_data->segment_count = 0;
    char_data->width = 0;
    char_data->segments = malloc(sizeof(struct chr_char_segment *) * current_max);

    if (char_data->segments == NULL) {
        return -1;
    }

    fseek(file->file, width_offset, SEEK_SET);
    char_data->width = fgetc(file->file);

    /* an initial segment data location */
    struct chr_char_segment *current_seg = malloc(sizeof(struct chr_char_segment));

    if (current_seg == NULL) {
        free(char_data->segments);
        return -1;
    }

    /* Lookup char data offset location*/
    fseek(file->file, offset, SEEK_SET);
    if(!fread(&char_offset, WORD, 1, file->file) > 0) {
        return -1;
    }
    /* Add the offset and move to that location, begin reading values
     * until an END block is found */
    char_offset += (file->num_char * 3) + file->char_offset_begin;
    fseek(file->file, char_offset, SEEK_SET);

#ifdef CHR_DEBUG
    printf("(DEBUG:: Char: %c - Offset: 0x%02x)\n", ascii_code, char_offset);
    printf("(DEBUG:: Char: %c - Width Offset: 0x%02x)\n", ascii_code, width_offset);
#endif

    while (get_char_segment(file, current_seg) != END) {
        char_data->segments[char_data->segment_count] = current_seg;
        char_data->segment_count++;

        current_max++;

        struct chr_char_segment **resized_data
                = realloc(char_data->segments, sizeof(struct chr_char_segment *) * current_max);

        if (resized_data == NULL) {
            return -1;
        }

        char_data->segments = resized_data;
        current_seg = malloc(sizeof(struct chr_char_segment));
    }

    free(current_seg); // free the last allocated segment, as it wasn't used

    return 0;
}

int get_char_segment(struct chr_file *file, struct chr_char_segment *data) {
    int op1, op2;
    int x, y;

    x = fgetc(file->file);
    y = fgetc(file->file);

    op1 = x >> 7;
    op2 = y >> 7;

    if (op1) {
        if (op2) {
            data->type = DRAW;
        } else {
            data->type = MOVE;
        }
    } else if (op2) {
        data->type = SCAN; // ? When is this used?
    } else {
        data->type = END;
        return data->type;
    }

    /* Remove the op flag from both */
    x &= ~(1 << 7);
    y &= ~(1 << 7);

    /* I'm sure there's a better way to get the
     * compliment of a 7bit number, but this
     * works, so I'm leaving it */
    if (x & (1 << 6)) {
        x = -(~x + 129);
    }

    if (y & (1 << 6)) {
        y = -(~y + 129);
    }

    data->coord.x = x;
    data->coord.y = y;

    return data->type;
}