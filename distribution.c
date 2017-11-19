#include <stdbool.h>
#include <stdio.h>
#include "distribution.h"
#include "datablock.h"

/**********************************************************
 **********************************************************
       Implement your distribution system in this file.
      You may add additional private functions as needed.
 **********************************************************
 **********************************************************
 */

/**
 * Debug/testing function to "reset" the distribution system back to default
 * --- Implement This Function As Needed ---
 */

#define CMD_TYPE_MASK 0x7
#define INDEX_MASK 0x3F8
#define DATA_MASK 0x3FFFFFFFC00
#define INDEX_POSITION 3
#define DATA_POSITION 10

static int index_map[NUM_DATA_BLOCKS];

void parse_command(uint64_t command, uint64_t *data, uint8_t *index, uint8_t *command_type);
status_e map_index(int index, uint8_t command_type);
void clear_map();

void reset(void)
{
    //reset the data blocks
    //leave this here. Implement additional reset code below.
    reset_data_blocks();
    
    //do any other reset needed here
    clear_map(); 
}

/**
 * Process the requested command
 * --- Implement This Function ---
 * @param command Pointer to the bit-packed command data containing the command type (bit 0-2), index (bit 3-9), and data (bit 10-41)
 * @return Status of the comamnd request.
 * return SUCCESS on successful write to or read from a data_block_t
 * return BLOCK_FULL for a write command if all blocks writes have been "used up"
 * return DATA_INVALID for reads to an index that have not been written, or any other read/write failures
 * On read SUCCESS, data should be packed into the correct location in the command pointer (bits 10-41).
 */
status_e process_command(uint64_t *command)
{
    uint8_t command_type;
    uint64_t data;
    uint8_t index;
    status_e status = DATA_INVALID;
    status_e map_status;

    parse_command(*command, &data, &index, &command_type);
    //attempt to map the index provided by user
    if((map_status = map_index((int)index, command_type)) != SUCCESS)
    {
        return map_status;
    }
    
    //read command type
    if(command_type == (uint8_t)CMD_READ) 
    { 
        //check for a valid block
        if(get_write_counter(index_map[index]) > 0) 
        {
            uint32_t temp_data;
            status = read_data_block(index_map[index], &temp_data);
            data = (uint64_t)temp_data;

            //get rid of garbage values in command variable 
            *command =  ~(DATA_MASK) & *command;
            //load data bits in command variable
            *command = (data << DATA_POSITION) | *command;
        }
    }
    //write command type
    else if(command_type == (uint8_t)CMD_WRITE) 
    {
        //attempt to write to data block
        status = write_data_block(index_map[index], data);
    }

    return status;
}

/**
 * Function to do any initialization required
 * --- Implement This Function --- 
 */
void initialize(void)
{
    //leave this here. Implement additional initialization code below.
    reset();
    
    //do any other initialization needed here
    clear_map();
}

/**
 * This function parses the command provided by the user
 */
void parse_command(uint64_t command, uint64_t *data, uint8_t *index, uint8_t *command_type)
{
    *command_type = (uint8_t)command & CMD_TYPE_MASK;
    *index = (uint8_t) ((command & INDEX_MASK) >> INDEX_POSITION);
    *data = (command & DATA_MASK) >> DATA_POSITION;
}

/**
 * This function maps the index provided by the user to the index
 * utilized to read and write data to the data blocks
 */
status_e map_index(int index, uint8_t command_type) 
{
    if(index > NUM_DATA_BLOCKS)
    {
        return DATA_INVALID;
    }
    //check if the index is new
    if(index_map[index] == -1) 
    {
        index_map[index] = index;
        return SUCCESS;
    }

    unsigned short i;
    uint32_t count_writes = get_write_counter(index_map[index]);
    if(count_writes >= BLOCK_MAX_WRITES && command_type == CMD_WRITE) 
    {
        /*current block is full and user needs to write
         *so we need to search for an empty block if it exists
         */
        for(i = 0; i < NUM_DATA_BLOCKS; i++) 
        {
            if(get_write_counter(i) == 0) 
            {
                index_map[index] = i;
                return SUCCESS;
            }
        }
        //no empty block was found
        return BLOCK_FULL;
    }
    return SUCCESS;
}

/**
 * This function initializes all the values of the index map to -1
 * -1 means the current index has not being utilized to index an item in the data block
 */
void clear_map()
{
    unsigned short i;
    for(i = 0; i < NUM_DATA_BLOCKS; i++) 
    {
        index_map[i] = -1;
    }
}
