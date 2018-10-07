//
// Created by philip on 05/09/17.
//

#include <stdio.h>
#include <stdlib.h>

#include "dict.h"

void delete_dict(struct chr_dict *character_data) {
    for (int i = 0; i < character_data->size; i++) {
        for (int j = 0; j < character_data->dict[i]->character_data->segment_count; j++) {
            free(character_data->dict[i]->character_data->segments[j]);
        }

        free(character_data->dict[i]->character_data->segments);
        free(character_data->dict[i]->character_data);
        free(character_data->dict[i]);
    }

    free(character_data->dict);
    free(character_data);
}

struct chr_dict_entry* add_dict_entry(char passed_char, struct chr_dict *character_data, struct chr_file *in_file) {
    struct chr_char *current_char = malloc(sizeof(struct chr_char));

    if (current_char == NULL) {
        return NULL;
    }

    if (get_char_data(in_file, passed_char, current_char) == 0) {
        struct chr_dict_entry *new_entry = malloc(sizeof(struct chr_dict_entry));

        if (new_entry == NULL) {
            free(current_char);
            return NULL;
        }

        new_entry->ascii_char = passed_char;
        new_entry->character_data = current_char;

        struct chr_dict_entry **resized = realloc(character_data->dict,
                                                  sizeof(struct chr_dict_entry *) * (character_data->size + 1));

        if (!resized) {
            free(new_entry);
            free(current_char);
            return NULL;
        }

        character_data->dict = resized;
        character_data->size++;

        character_data->dict[character_data->size - 1] = new_entry;

        return new_entry;
    } else {
        return NULL;
    }
}

struct chr_char *get_dict_char(char find_char, struct chr_dict *dict) {
    for (int j = 0; j < dict->size; j++) {
        if (find_char == dict->dict[j]->ascii_char) {
            return dict->dict[j]->character_data;
        }
    }

    return NULL;
}