//
// Created by philip on 05/09/17.
//

#ifndef __H_CHR_DICT
#define __H_CHR_DICT

#endif //__H_CHR_DICT

#ifndef __H_CHR_DATA
#include "chr.h"
#endif

struct chr_dict_entry {
    char ascii_char;
    struct chr_char *character_data;
};

struct chr_dict {
    int size;
    struct chr_dict_entry **dict;
};

void delete_dict(struct chr_dict*);
struct chr_dict_entry* add_dict_entry(char, struct chr_dict*, struct chr_file*);
struct chr_char* get_dict_char(char, struct chr_dict*);