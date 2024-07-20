#include "sim.h"
using namespace std;

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/

int L1_prefetch = 0, L2_prefetch = 0;

int main(int argc, char *argv[])
{	
	FILE *fp;			// File pointer.
    char *trace_file;		// This variable holds the trace file name.
    cache_params_t params;	// Look at the sim.h header file for the definition of struct cache_params_t.
    char rw;			// This variable holds the request's type (read or write) obtained from the trace.
    uint32_t addr;		// This variable holds the request's address obtained from the trace.
                    // The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

    // Exit with an error if the number of command-line arguments is incorrect.
    if (argc != 9) {
        printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
        exit(EXIT_FAILURE);
    }

    // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
	params.block_size = (uint32_t) atoi(argv[1]);
	params.L1_size    = (uint32_t) atoi(argv[2]);
	params.L1_assoc   = (uint32_t) atoi(argv[3]);
	params.L2_size    = (uint32_t) atoi(argv[4]);
	params.L2_assoc   = (uint32_t) atoi(argv[5]);
	params.pref_n     = (uint32_t) atoi(argv[6]);
	params.pref_m     = (uint32_t) atoi(argv[7]);
	trace_file        = argv[8];

    // Open the trace file for reading.
	fp = fopen(trace_file, "r");
    if(fp == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }

	// Print simulator configuration.
	printf("===== Simulator configuration =====\n");
	printf("BLOCKSIZE:  %u\n", params.block_size);
	printf("L1_SIZE:    %u\n", params.L1_size);
	printf("L1_ASSOC:   %u\n", params.L1_assoc);
	printf("L2_SIZE:    %u\n", params.L2_size);
	printf("L2_ASSOC:   %u\n", params.L2_assoc);
	printf("PREF_N:     %u\n", params.pref_n);
	printf("PREF_M:     %u\n", params.pref_m);
	printf("trace_file: %s\n", trace_file);
	printf("\n");

	// Create L1 cache
	Cache *L1_cache = NULL;
	L1_cache = (struct Cache *)calloc(1, sizeof(Cache));
	Cache_init(L1_cache , params.block_size, params.L1_size, params.L1_assoc);

	// If L2 cache is available, create L2 cache
	Cache *L2_cache = NULL;
	if (params.L2_size!=0)
	{
		L2_cache = (struct Cache *)calloc(1, sizeof(Cache));
		Cache_init(L2_cache, params.block_size, params.L2_size, params.L2_assoc);
		L1_cache->next_level_cache = L2_cache;
		L2_cache->next_level_cache = NULL;
	}
	else
	{
		L1_cache->next_level_cache = NULL;
	}

	// If prefetch configuration is available, create stream buffer
	if (params.pref_n > 0 && params.pref_m > 0)
	{
		L1_cache->stream_buffer = (struct Cache *)calloc(1, sizeof(Cache));
		Cache_init(L1_cache->stream_buffer, params.block_size, (params.block_size*params.pref_m), params.pref_m);
		L1_cache->stream_buffer->num_sets = params.pref_n;
		if (L2_cache)
		{
			L2_cache->stream_buffer = (struct Cache *)calloc(1, sizeof(Cache));
			Cache_init(L2_cache->stream_buffer, params.block_size, (params.block_size*params.pref_m), params.pref_m);
			L2_cache->stream_buffer->num_sets = params.pref_n;
			cout << "Continue" << endl;
		}
	}
	else 
	{
		L1_cache->stream_buffer = NULL;
		if (L2_cache)
		{
			L2_cache->stream_buffer = NULL;
		}
	}

	// Read requests from the trace file and echo them back.
	while (fscanf(fp, "%c %x\n", &rw, &addr) == 2)	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
	{
		if (rw == 'r')
		{
			cache_read(L1_cache, addr);
		}
		else if (rw == 'w') {
			cache_write(L1_cache, addr);
		}
	}

	// Print cache contents
	if (L1_cache)
	{
		cout << "===== L1 contents =====" << endl;
		print_order(L1_cache);
		printf("\n");
	}

	if (L1_cache && L1_cache->stream_buffer)
	{
		cout << "===== Stream Buffer(s) contents =====" << endl;
		print_order(L1_cache->stream_buffer);
		printf("\n");
	}

	if (L2_cache)
	{
		cout << "===== L2 contents =====" << endl;
		print_order(L2_cache);
		printf("\n");
	}

	// Print cache simulator results
	cout << "===== Measurements =====" << endl;

	int L1_read = 0, L1_readmiss = 0, L1_write = 0, L1_writemiss = 0, L1_writeback = 0;
	int L2_read = 0, L2_readmiss = 0, L2_write = 0, L2_writemiss = 0, L2_writeback = 0;
	double L1_missrate = 0.0, L2_missrate = 0.0;

	if (L1_cache)
	{
		L1_read = L1_cache->read_count;
		L1_readmiss = L1_cache->read_miss;
		L1_write = L1_cache->write_count;
		L1_writemiss = L1_cache->write_miss;
		if (L1_read | L1_write) 
		{
			L1_missrate = (double)(L1_readmiss + L1_writemiss) / (L1_read + L1_write);
		}
		L1_writeback = L1_cache->write_back;
	}

	if (L2_cache)
	{
		L2_read = L2_cache->read_count;
		L2_readmiss = L2_cache->read_miss;
		L2_write = L2_cache->write_count;
		L2_writemiss = L2_cache->write_miss;
		L2_writeback = L2_cache->write_back;
		if (L2_read)
		{
			L2_missrate = (double)L2_readmiss / L2_read;
		}
	}


	cout << "a. L1 reads:       	   	" << L1_read << endl;
	cout << "b. L1 read misses:  	   	" << L1_readmiss << endl;
	cout << "c. L1 writes:        	  	" << L1_write << endl;
	cout << "d. L1 write misses:  	  	" << L1_writemiss << endl;
	cout << "e. L1 miss rate:          	" << setprecision(4) << fixed << L1_missrate << endl;
	cout << "f. L1 writebacks:   		" << L1_writeback << endl;
	cout << "g. L1 prefetches:         	" << L1_prefetch << endl;
	cout << "h. L2 reads (demand):      	" << L2_read << endl;
	cout << "i. L2 read misses (demand):	" << L2_readmiss << endl;
	cout << "j. L2 reads (prefetch):      	" << L2_prefetch << endl;
	cout << "k. L2 read misses (prefetch):	" << L2_prefetch << endl;
	cout << "l. L2 writes:        	   	" << L2_write << endl;
	cout << "m. L2 write misses:  	   	" << L2_writemiss << endl;
	cout << "n. L2 miss rate:          	" << setprecision(4) << fixed << L2_missrate << endl;
	cout << "o. L2 writebacks:         	" << L2_writeback << endl;
	cout << "p. L2 prefetches:         	" << L2_prefetch << endl;
	if (L2_cache) {
		cout << "q. memory traffic:        	" << L2_readmiss + L2_writemiss + L2_writeback + L2_prefetch << endl;
	}
	else {
		cout << "q. memory traffic:        	" << L1_readmiss + L1_writemiss + L1_writeback + L1_prefetch << endl;
	}

}


// Function to print cache contents based on LRU
void print_order(Cache *cache)
{
	uint32_t count;
	int i, j;

	for (j = 0; (unsigned)j < cache->num_sets; j++)
	{
		printf("set%7d:  ", j);
		count = 0;
		while (count < cache->assoc)
		{
			for (i = 0; (unsigned)i < cache->assoc; i++)
			{
				if (cache->valid_flag_matrix[j][i] && (cache->LRU[j][i] == count))
				{
					if (cache->tag_matrix[j][i])
					{
						printf("%7x ", cache->tag_matrix[j][i]);
					}
					if (cache->dirty_flag_matrix[j][i])
					{
						printf("D ");
					}
					else 
					{
						printf("  ");
					}
					break;
				}
			}
			count++;
		}
		printf("\n");
	}
}

// Check if any block is not valid 
int check_valid_block(Cache *cache, uint32_t row)
{
	for (int j = 0; (unsigned)j < cache->assoc; j++)
	{
		if (cache->valid_flag_matrix[row][j] == 0)
		{
			return j;
		}
	}

	return 0;
}

// Calculate and return tag value for the given cache and address
uint32_t get_tag_value(Cache *cache, uint32_t add)
{
	uint32_t tag_value, tag_bits, temp;
	tag_bits = 32- log2(cache->block_size) - log2(cache->num_sets);
	temp = pow(2, tag_bits) - 1;
	tag_value = ((add >> (32 - tag_bits)) & temp);
	return tag_value;
}

// Calculate and return index value for the given cache and address
uint32_t get_index_value(Cache *cache, uint32_t add)
{
	uint32_t index_value, index_bits, temp;
	index_bits = log2(cache->num_sets);
	temp = pow(2, index_bits) - 1;
	index_value = ((add >> (int)log2(cache->block_size)) & temp);
	return index_value;
}

// Find LRU for the given cache and index
uint32_t LRU(Cache *cache, uint32_t index)
{
	for (int j = 0; (unsigned)j < cache->assoc; j++)
	{
		if (cache->LRU[index][j] == (cache->assoc - 1))
		{
			return j;
		}
	}

	return cache->assoc;
}

// Update the LRU for the given cache and block
void update_LRU(Cache *cache, uint32_t row, int col)
{
	for (int j = 0; (unsigned)j < cache->assoc; j++)
	{
		if (cache->LRU[row][j] < cache->LRU[row][col])
		{
			cache->LRU[row][j] = cache->LRU[row][j] + 1;
		}
	}

	//update MRU to 0
	cache->LRU[row][col] = 0;
}

// Write operation of cache
void cache_write(Cache *cache, uint32_t add)
{
	uint32_t index, tag;
	int i, empty_b, lru_b = 0;
	index = get_index_value(cache, add);
	tag = get_tag_value(cache, add);

	cache->write_count++;

	// Check if the address hits cache
	for (i = 0; (unsigned)i < cache->assoc; i++)
	{
		if (cache->valid_flag_matrix[index][i] && cache->tag_matrix[index][i] == tag)
		{
			update_LRU(cache, index, i);
			cache->write_hit++;
			cache->dirty_flag_matrix[index][i] = 1;
			return;
		}
	}
	
	// If steam buffer is available, check if the address hits stream buffer
	if (cache->stream_buffer)
	{
		int k, m, n, hit = 0;
		uint32_t r_tag, s_tag;

		r_tag = get_tag_value(cache->stream_buffer, add);
		s_tag = r_tag + 1;

		for (k = 0; (unsigned)k < cache->stream_buffer->assoc; k++)
		{
			if (cache->stream_buffer->tag_matrix[0][k] == r_tag)
			{
				for (m = 0; (unsigned)m < cache->stream_buffer->assoc; m++)
				{
					cache->stream_buffer->tag_matrix[0][m] = s_tag + m;
					L1_prefetch++;
				}
				cache->stream_buffer->valid_flag_matrix[0][k] = 1;
				update_LRU(cache->stream_buffer, 0, k);
				L1_prefetch = L1_prefetch - k - 1;
				hit = 1;
				cache->write_hit++;
				break;
			}
		}

		// If not hit, create new stream buffer contents based on the missed address
		if (hit == 0) 
		{
			for (n = 0; (unsigned)n < cache->stream_buffer->assoc; n++)
			{
				cache->stream_buffer->tag_matrix[0][n] = s_tag + n;
				L1_prefetch++;
			}
			cache->stream_buffer->valid_flag_matrix[0][k] = 1;
			update_LRU(cache->stream_buffer, 0, 0);
			cache->write_miss++;
		}
	}
	else {
		// If not hit, update miss
		cache->write_miss++;
	}

	// Check if there is empty block
	empty_b = check_valid_block(cache, index);
	
	if (empty_b > 0) // Has empty blocks
	{
		if (cache->next_level_cache) // If current cache has next level cache
		{
			// Apply read operation to the next level cache
			cache_read(cache->next_level_cache, add);
		}
		// Update tag, valid flag, and dirty flag
		cache->tag_matrix[index][empty_b] = tag;
		cache->valid_flag_matrix[index][empty_b] = 1;
		cache->dirty_flag_matrix[index][empty_b] = 1;
		update_LRU(cache, index, empty_b);
		return;
	}
	else // No empty blocks
	{
		lru_b = LRU(cache, index);

		if (cache->next_level_cache) // If current cache has next level cache
		{
			if (cache->dirty_flag_matrix[index][lru_b]) // If the block is dirty, evict it and write back
			{
				uint32_t lru_address, lru_tag;
				lru_tag = cache->tag_matrix[index][lru_b];
				lru_address = ((lru_tag << (int)(log2(cache->num_sets))) + (int)index);
				lru_address = lru_address << (int)log2(cache->block_size);
				cache_write(cache->next_level_cache, lru_address);
				cache->dirty_flag_matrix[index][lru_b] = 0;
				cache->write_back++;
			}
			// Apply read operation to the next level cache and update current cache
			cache_read(cache->next_level_cache, add);
			cache->tag_matrix[index][lru_b] = get_tag_value(cache, add);
			cache->valid_flag_matrix[index][lru_b] = 1;
			cache->dirty_flag_matrix[index][lru_b] = 1;
			update_LRU(cache, index, lru_b);
			return;
		}
	
		else {
			if (cache->dirty_flag_matrix[index][lru_b]) // If the block is dirty, evict it and write back
			{
				cache->write_back++;
				cache->dirty_flag_matrix[index][lru_b] = 0;
			}
			// Update current cache
			cache->tag_matrix[index][lru_b] = get_tag_value(cache, add);
			cache->valid_flag_matrix[index][lru_b] = 1;
			cache->dirty_flag_matrix[index][lru_b] = 1;
			update_LRU(cache, index, lru_b);
			return;
		}
	}
}

// Read operation of cache
void cache_read(Cache *cache, uint32_t add) {
	uint32_t index, tag;
	int i, empty_b, lru_b;
	index = get_index_value(cache, add);
	tag = get_tag_value(cache, add);
	
	cache->read_count++;

	// Check if the address hits cache
	for (i = 0; (unsigned)i < cache->assoc; i++)
	{
		if (cache->valid_flag_matrix[index][i] && cache->tag_matrix[index][i] == tag)
		{
			update_LRU(cache, index, i);
			cache->read_hit++;
			return;
		}
	}

	// If steam buffer is available, check if the address hits stream buffer
	if (cache->stream_buffer)
	{
		int k, m, n, hit = 0;
		uint32_t r_tag, s_tag;

		r_tag = get_tag_value(cache->stream_buffer, add);
		s_tag = r_tag + 1;

		for (k = 0; (unsigned)k < cache->stream_buffer->assoc; k++)
		{
			if (cache->stream_buffer->tag_matrix[0][k] == r_tag)
			{
				for (m = 0; (unsigned)m < cache->stream_buffer->assoc; m++)
				{
					cache->stream_buffer->tag_matrix[0][m] = s_tag + m;
					L1_prefetch++;
				}
				cache->stream_buffer->valid_flag_matrix[0][k] = 1;
				update_LRU(cache->stream_buffer, 0, k);
				L1_prefetch = L1_prefetch - k - 1;
				hit = 1;
				cache->read_hit++;
				break;
			}
		}

		// If not hit, create new stream buffer contents based on the missed address
		if (hit == 0)
		{
			for (n = 0; (unsigned)n < cache->stream_buffer->assoc; n++)
			{
				cache->stream_buffer->tag_matrix[0][n] = s_tag + n;
				L1_prefetch++;
			}
			cache->stream_buffer->valid_flag_matrix[0][k] = 1;
			update_LRU(cache->stream_buffer, 0, 0);
			cache->read_miss++;
		}

	}
	else {
		// If not hit, update miss
		cache->read_miss++;
	}

	// Check if there is empty block
	empty_b = check_valid_block(cache, index);

	if (empty_b > 0) // Has empty blocks
	{
		if (cache->next_level_cache) // If current cache has next level cache
		{
			// Apply read operation to the next level cache
			cache_read(cache->next_level_cache, add);
			
		}
		// Update tag, valid flag, and dirty fla
		cache->tag_matrix[index][empty_b] = tag;
		cache->valid_flag_matrix[index][empty_b] = 1;
		cache->dirty_flag_matrix[index][empty_b] = 0;
		update_LRU(cache, index, empty_b);
		return;
	}

	else // No empty blocks
	{
		lru_b = LRU(cache, index);

		if (cache->next_level_cache)
		{
			if (cache->dirty_flag_matrix[index][lru_b]) // If the block is dirty, evict it and write back
			{
				uint32_t lru_address, lru_tag;
				lru_tag = cache->tag_matrix[index][lru_b];
				lru_address = ((lru_tag << (int)(log2(cache->num_sets))) + (int)index);
				lru_address =  lru_address<< (int)log2(cache->block_size);
				cache_write(cache->next_level_cache, lru_address);
				cache->dirty_flag_matrix[index][lru_b] = 0;
				cache->write_back++;
			}
			// Apply read operation to the next level cache and update current cache
			cache_read(cache->next_level_cache, add);
			cache->tag_matrix[index][lru_b] = get_tag_value(cache, add);
			cache->valid_flag_matrix[index][lru_b] = 1;
			update_LRU(cache,index,lru_b);
			return;
		}
		else {
			if (cache->dirty_flag_matrix[index][lru_b]) // If the block is dirty, evict it and write back
			{
				cache->write_back++;
				cache->dirty_flag_matrix[index][lru_b] = 0;				
			}
			// Update current cache
			cache->tag_matrix[index][lru_b] = get_tag_value(cache, add);
			cache->valid_flag_matrix[index][lru_b] = 1;
			update_LRU(cache, index, lru_b);
			return;
		}
	}
}

void Cache_init(Cache * cache, uint32_t blocksize, uint32_t size, uint32_t assoc) 
{
	int i, j;
	cache->num_sets = size / (blocksize * assoc);
	cache->assoc = assoc;
	cache->block_size = blocksize;
	
	// Create tag matrix to store tags
	cache->tag_matrix = (uint32_t **)calloc(cache->num_sets, sizeof(uint32_t *));

	for (i = 0; (unsigned)i < cache->num_sets; i++)
	{
		cache->tag_matrix[i] = (uint32_t *)calloc(assoc, sizeof(uint32_t));

	}

	// Create LRU matrix for the tag
	cache->LRU = (uint32_t **)calloc(cache->num_sets, sizeof(uint32_t *));
	for (i = 0; (unsigned)i < cache->num_sets; i++)
	{
		cache->LRU[i] = (uint32_t *)calloc(assoc, sizeof(uint32_t));
		for (j = 0; (unsigned)j < assoc; j++)
		{
			cache->LRU[i][j] = j;
		}
	}
	
	// Create validity flag matrix for the tag
	cache->valid_flag_matrix = (int **)calloc(cache->num_sets, sizeof(int *));
	for (i = 0; (unsigned)i < cache->num_sets; i++)
	{
		cache->valid_flag_matrix[i] = (int *)calloc(assoc, sizeof(int));
	}

	cache->dirty_flag_matrix = (int **)calloc(cache->num_sets, sizeof(int *));
	for (i = 0; (unsigned)i < cache->num_sets; i++)
	{
		cache->dirty_flag_matrix[i] = (int *)calloc(assoc, sizeof(int));
	}
}


