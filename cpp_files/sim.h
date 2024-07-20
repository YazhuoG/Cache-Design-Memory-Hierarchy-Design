#ifndef SIM_CACHE_H
#define SIM_CACHE_H

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <stdint.h>

typedef struct cache_params_t {
	uint32_t block_size;
	uint32_t L1_size;
	uint32_t L1_assoc;
	uint32_t L2_size;
	uint32_t L2_assoc;
	uint32_t pref_n;
	uint32_t pref_m;
} cache_params_t;

typedef struct Cache {
	uint32_t block_size;
	uint32_t num_sets;
	uint32_t assoc;
	uint32_t **tag_matrix;
	int **valid_flag_matrix;
	int **dirty_flag_matrix;
	uint32_t **LRU;
	uint32_t read_hit;
	uint32_t read_count;
	uint32_t read_miss;
	uint32_t write_count;
	uint32_t write_hit;
	uint32_t write_miss;
	uint32_t write_back;
	struct Cache * next_level_cache;
	struct Cache * stream_buffer;
} Cache;


void print_order(Cache*);
int check_valid_block(Cache*, uint32_t);
uint32_t get_tag_value(Cache*, uint32_t);
uint32_t get_index_value(Cache*, uint32_t);
uint32_t LRU(Cache*, uint32_t);
void update_LRU(Cache*, uint32_t, int);
void cache_write(Cache*, uint32_t);
void cache_read(Cache*, uint32_t);
void Cache_init(Cache*, uint32_t, uint32_t, uint32_t);

#endif