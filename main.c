#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <locale.h>

#ifndef __H_CHR_DATA

#include "chr.h"

#endif

#ifndef __H_CHR_MAIN

#include "main.h"

#endif

#ifndef __H_CHR_DICT
#include "chr_dict.h"
#endif

int main(int argc, char *argv[]) {
    int c;
    char *filename = NULL;
    char *passed_string = NULL;
    int passed_string_len;

    setlocale(LC_ALL, "");

    struct chr_dict *character_data = NULL;
    struct chr_file *in_file = NULL;

    struct gcode_settings settings;
    setting_defaults(&settings);

    while ((c = getopt(argc, argv, "n:f:r:x:y:h:w:z:u:p:d:F:a:A:t:o")) != -1) {
        switch (c) {
            case 'f':
                filename = optarg;
                break;
            case 'r':
                settings.rotation = strtof(optarg, NULL);
                break;
            case 'n':
                settings.sub_id = optarg;
                break;
            case 'x':
                settings.offset_x = strtof(optarg, NULL);
                break;
            case 'y':
                settings.offset_y = strtof(optarg, NULL);
                break;
            case 'w': /* Max sentence length */
                settings.x_length = strtof(optarg, NULL);
                break;
            case 'h': /* Max height of all lines */
                settings.y_length = strtof(optarg, NULL);
                break;
            case 'd': /* Max depth of cut */
                settings.cut_to = strtof(optarg, NULL);
                break;
            case 'u': /* Max retract height */
                settings.rapid_z = strtof(optarg, NULL);
                break;
            case 'p': /* Plunge from height */
                settings.plunge_from = strtof(optarg, NULL);
                break;
            case 'z': /* Rate of feed from Z-Up to cut-depth */
                settings.plunge_rate = (int) strtol(optarg, NULL, 10);
                break;
            case 'F': /* Rate of feed in cut */
                settings.feed_rate = (int) strtol(optarg, NULL, 10);
                break;
            case 'a': /* Horizontal alignment (0 = Left, 1 = Middle, 2 = Right */
                settings.x_offset_align = (enum x_align) strtol(optarg, NULL, 10);
                break;
            case 'A': /* Vertical alignment (0 = Bottom, 1 = Centre, 2 = Top */
                settings.y_offset_align = (enum y_align) strtol(optarg, NULL, 10);
                break;
            case 'o': /* Produced sub expects to have global tool variables available */
                settings.sub_options = 1;
                break;
            case 't':
                settings.align_x = (enum x_align) strtol(optarg, NULL, 10);
                break;
            default:
                exit(-1);
        }
    }

    if (filename == NULL) {
        printf("Error: a valid font path must be passed with the -f switch\n");
        return -1;
    }

    if (optind >= argc) {
        printf("Error: no text to produced passed\n");
        return -1;
    } else {
        passed_string = strdup(argv[optind++]);
        passed_string_len = (int) strlen(passed_string);
    }

    in_file = calloc(1, sizeof(struct chr_file));

    if (!in_file) {
        printf("Error: couldn't allocate memory for CHR file structure\n");
        free(passed_string);
        return -1;
    }

    /* Open font file for reading */
    in_file->file = fopen(filename, "rb");

    if (in_file->file == NULL) {
        printf("Error: Could not open file %s\n", filename);

        free(in_file);
        free(passed_string);
        return -1;
    }

    /* Get font header information */
    if (get_header(in_file)) {
        printf("Error: file %s does not appear to be a correctly formatted CHR file\n", filename);

        delete_chr(in_file);
        free(passed_string);

        return -1;
    }

    /* Get font details data + spacing and offsets */
    if (get_font_data(in_file)) {
        printf("Error: file %s does not appear to be a stroke file type\n", filename);

        delete_chr(in_file);
        free(passed_string);

        return -1;
    }

    character_data = malloc(sizeof(struct chr_dict));

    if (character_data == NULL) {
        printf("Error: couldn't allocate memory for the character dictionary\n");

        delete_chr(in_file);
        free(passed_string);

        return -1;
    }

    character_data->size = 0;
    character_data->dict = malloc(sizeof(struct chr_dict_entry *));

    if (character_data->dict == NULL) {
        printf("Error: couldn't allocate memory for the character dictionary\n");

        delete_chr(in_file);
        free(passed_string);
        free(character_data);

        return -1;
    }

    /* ---------------------------------------- */
    /* Init line output */

    struct output *lines;
    struct chr_char *current_char = NULL;
    struct chr_line *current_line = calloc(1, sizeof(struct chr_line));

    lines = malloc(sizeof(struct output));

    if (init_output(lines) == -1) {
        printf("Error: couldn't allocate memory for a new line\n");

        delete_chr(in_file);
        delete_dict(character_data);
        free(current_line);
        free(passed_string);
        free(character_data);

        return -1;
    }

    add_line(lines, current_line);

    for (int i = 0; i < passed_string_len; i++) {
        if (passed_string[i] > 127 || (passed_string[i] < in_file->first_char &&
                passed_string[i] != 10 && passed_string[i] != 13)) {
            printf("Error: Invalid Character passed (%i)\n", passed_string[i]);

            delete_chr(in_file);
            delete_dict(character_data);
            free(lines->line);
            free(lines);
            free(current_line->string);
            free(current_line);
            free(passed_string);

            return -1;
        }

        if (passed_string[i] == '\n' || (passed_string[i] == '\\' && passed_string[i + 1] == 'n')) {
            i += (passed_string[i] != '\n');

            current_line = calloc(1, sizeof(struct chr_line));

            if (current_line == NULL) {
                printf("Error: Couldn't allocate memory for a new line\n");

                delete_chr(in_file);
                delete_dict(character_data);
                free(current_line);
                free(passed_string);

                return -1;
            }

            if (add_line(lines, current_line) < 0) {
                delete_chr(in_file);
                delete_dict(character_data);
                free(current_line);
                free(passed_string);
                free(character_data);

                return -1;
            }
            continue;
        } else if (get_dict_char(passed_string[i], character_data) == NULL) {
            if (add_dict_entry(passed_string[i], character_data, in_file) == NULL) {
                printf("Error: Couldn't add entry for %c to dictionary\n", passed_string[i]);

                delete_chr(in_file);
                delete_dict(character_data);
                free(passed_string);

                return -1;
            }
        }

        current_char = get_dict_char(passed_string[i], character_data);
        add_to_line(current_line, current_char, passed_string[i]);
    }

    fclose(in_file->file);

    set_scale(in_file, lines, &settings);

    produce_gcode(lines, &settings);

    free(in_file);
    delete_lines(lines);
    delete_dict(character_data);
    free(passed_string);
    return 0;
}

void set_scale(struct chr_file *in_file, struct output *lines, struct gcode_settings *settings) {
    lines->line_height = (in_file->cap_height - in_file->dec_height);
    lines->total_height = get_max_height(lines->line[0]);

    if (lines->line_count > 1) {
        lines->total_height += (lines->line_height * (lines->line_count - 2)) - in_file->dec_height;
        lines->total_height += (in_file->cap_height - get_min_height(lines->line[lines->line_count - 1]));
    } else {
        lines->total_height -= get_min_height(lines->line[0]);
    }

    /* Get the longest line */
    for (int i = 0; i < lines->line_count; i++) {
        get_line_width(lines->line[i]);

        if (lines->line[i]->line_width > lines->max_width) {
            lines->max_width = lines->line[i]->line_width;
        }
    }

    if (settings->x_length) {
        float x_scale = settings->x_length / lines->max_width;

        if (settings->y_length && ((lines->total_height * x_scale) > settings->y_length)) {
            settings->scale = settings->y_length / lines->total_height;
        } else {
            settings->scale = x_scale;
        }
    } else if (settings->y_length) {
        settings->scale = settings->y_length / lines->total_height;
    }
}
/**
 * Produce a GCode routine to engrave text
 * @param lines
 * @param settings
 * @return
 */
int produce_gcode(struct output *lines, struct gcode_settings *settings) {
    float x, y;
    float sin_rot, cos_rot;
    float x_new, y_new;
    float line_height = lines->line_height * settings->scale;
    struct coord cur_loc, align_offset;

    cur_loc.x = align_offset.x = x = 0;
    cur_loc.y = align_offset.y = y = 0;
    sin_rot = cos_rot = 1;

    if (settings->rotation) {
        float f_rad = (settings->rotation * ((float) M_PI / 180));
        sin_rot = sinf(f_rad);
        cos_rot = cosf(f_rad);
    }

    if (settings->y_offset_align == MIDDLE) {
        align_offset.y = (lines->total_height / 2);
    } else if (settings->y_offset_align == BOTTOM) {
        align_offset.y = lines->total_height;
    }

    align_offset.y -= (get_max_height(lines->line[0]));

    if (settings->x_offset_align == CENTER) {
        align_offset.x -= (lines->max_width / 2);
    } else if (settings->x_offset_align == RIGHT) {
        align_offset.x -= lines->max_width;
    }

    align_offset.x *= settings->scale;
    align_offset.y *= settings->scale;

    if(settings->sub_id != NULL) {
        printf("o<%s_engrave> sub\n", settings->sub_id);
    }

    for (int i = 0; i < lines->line_count; i++) {

        if (settings->align_x == CENTER) {
            cur_loc.x = ((lines->max_width / 2) - (lines->line[i]->line_width / 2)) * settings->scale;
        } else if (settings->align_x == RIGHT) {
            cur_loc.x = ((lines->max_width) - (lines->line[i]->line_width)) * settings->scale;
        } else {
            cur_loc.x = 0;
        }

        /* Add a new line on each subsequent line */
        cur_loc.y = -(line_height * i);

        for (int j = 0; j < lines->line[i]->string_length; j++) {
            /* Removed due to inability to escape chars in comments */
            //printf("\n(Char: '%c')\n", lines->line[i]->string[j]);

            struct chr_char *current_char = lines->line[i]->characters[j];

            /* Insures the initial move after a G0 is prefeced with G1 */
            int initial_move = 1;

            for (int k = 0; k < current_char->segment_count; k++) {
                int plunging = 0;
                int ending = (k == current_char->segment_count - 1);
                x = ((current_char->segments[k]->coord.x) * settings->scale) + cur_loc.x;
                y = ((current_char->segments[k]->coord.y) * settings->scale) + cur_loc.y;

                if (current_char->segments[k]->type == MOVE) {
                    if (k != 0 && current_char->segments[k - 1]->type == DRAW) { /* Was down, retract */
                        if(settings->sub_options) {
                            printf("G0 Z#<_z_clear>\n");
                        } else {
                            printf("G0 Z%.3f\n", settings->plunge_from);
                        }
                    }
                } else if (current_char->segments[k]->type == DRAW) {
                    if (k != 0 && current_char->segments[k - 1]->type == MOVE) { /* Was Up, Plunge */

                        if(settings->sub_options) {
                            printf("G0 Z#<_z_clear>\n");
                            printf("G1 Z%.3f F#<_feed_vertical>\n", settings->cut_to);
                        } else {
                            printf("G0 Z%.3f\n", settings->plunge_from);
                            printf("G1 Z%.3f F%i\n", settings->cut_to, settings->plunge_rate);
                        }

                        plunging = 1;
                    }
                }

                if (!ending) {
                    x += align_offset.x;
                    y += align_offset.y;

                    if (settings->rotation) {
                        x_new = cos_rot * x - sin_rot * y;
                        y_new = sin_rot * x + cos_rot * y;

                        x = x_new;
                        y = y_new;
                    }

                    x += settings->offset_x;
                    y += settings->offset_y;

                    if(initial_move) {
                        printf("G%i X%.3f Y%.3f", current_char->segments[k]->type != MOVE, x, y);
                        initial_move = 0;
                    } else {
                        printf("X%.3f Y%.3f", x, y);
                    }

                    if (plunging) {
                        if(settings->sub_options) {
                            printf(" F#<_feed_normal>");
                        } else {
                            printf(" F%i", settings->feed_rate);
                        }
                    }

                    printf("\n");
                } else {
                    if(settings->sub_options) {
                        printf("G0 Z#<_rapid_z>\n");
                    } else {
                        printf("G0 Z%.3f\n", settings->rapid_z);
                    }
                }
            }

            cur_loc.x = x;
            cur_loc.y = y;
        }
    }

    if(settings->sub_id != NULL) {
        printf("o<%s_engrave> endsub\n", settings->sub_id);
    }
    return 0;
}

void delete_chr(struct chr_file *data) {
    fclose(data->file);
    data->file = NULL;

    free(data);
}

void delete_lines(struct output *lines) {
    for (int i = 0; i < lines->line_count; i++) {
        free(lines->line[i]->characters);
        free(lines->line[i]->string);
        free(lines->line[i]);
    }

    free(lines->line);

    free(lines);
}

float get_max_height(struct chr_line *line) {
    float max_y = 0;

    for (int i = 0; i < line->string_length; i++) {
        for (int j = 0; j < line->characters[i]->segment_count; j++) {

            if (line->characters[i]->segments[j]->coord.y > max_y) {
                max_y = line->characters[i]->segments[j]->coord.y;
            }
        }
    }

    return max_y;
}

float get_min_height(struct chr_line *line) {
    float min_y = 0;

    for (int i = 0; i < line->string_length; i++) {
        for (int j = 0; j < line->characters[i]->segment_count; j++) {

            if (line->characters[i]->segments[j]->coord.y < min_y) {
                min_y = line->characters[i]->segments[j]->coord.y;
            }
        }
    }

    return min_y;
}

void get_line_width(struct chr_line *line) {
    float max_x = 0;
    if(line->string_length != 0) {
        struct chr_char *final_char = line->characters[line->string_length - 1];

        for (int i = 0; i < line->string_length - 1; i++) {
            line->line_width += line->characters[i]->width;
        }

        for (int i = 0; i < final_char->segment_count - 1; i++) {
            if (final_char->segments[i]->coord.x > max_x) {
                max_x = final_char->segments[i]->coord.x;
            }
        }

        line->line_width += max_x;
    }
}

int add_to_line(struct chr_line *line, struct chr_char *char_data, char passed_char) {
    if (line->string_length == 0) {
        line->characters = malloc(sizeof(struct chr_char *));

        if (line->characters == NULL) {
            return -1;
        }

        line->characters[0] = char_data;
        line->string_length++;
    } else {
        /* expand the array */
        line->string_length++;
        struct chr_char **resize = realloc(line->characters, sizeof(struct chr_char *) * line->string_length);

        if (resize == NULL) {
            return -1;
        }

        line->characters = resize;
        line->characters[line->string_length - 1] = char_data;
    }

    char *old_string = line->string;

    if (!asprintf(&line->string, "%s%c", line->string, passed_char)) {
        return -1;
    }
    free(old_string);

    return 0;
}

int init_output(struct output *output) {

    output->line = malloc(sizeof(struct chr_line *));

    if (output->line == NULL) {
        free(output);

        return -1;
    }

    output->line_count = 0;
    output->total_height = output->max_width = 0;

    return 0;
}

int add_line(struct output *output, struct chr_line *new_line) {
    struct chr_line **resize = realloc(output->line, sizeof(struct chr_line *) * (output->line_count + 1));

    if (resize == NULL) {
        return -1;
    }

    output->line_height = 0;
    output->total_height = 0;
    output->line = resize;
    output->line_count++;
    output->line[output->line_count - 1] = new_line;

    new_line->string = malloc(sizeof(char*));
    new_line->string[0] = '\0';

    return 0;
}

void setting_defaults(struct gcode_settings *settings) {
    memset(settings, 0, sizeof(struct gcode_settings));

    settings->scale = 1.0;
    settings->plunge_rate = 100;
    settings->feed_rate = 250;
    settings->rapid_z = 2.5;
    settings->plunge_from = 0.25;
    settings->cut_to = (float) -0.2;

    settings->x_length = 0;
    settings->y_length = 0;

    settings->offset_x = 0;
    settings->offset_y = 0;
    settings->x_offset_align = CENTER;
    settings->y_offset_align = MIDDLE;

    settings->sub_options = 0;

    settings->align_x = CENTER;
}