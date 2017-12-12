// fs.cpp: File System

#include "sfs/fs.h"

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

// Debug file system -----------------------------------------------------------

void FileSystem::debug(Disk *disk) {
    Block block;
    Block block2;
    uint32_t indir;
    unsigned int inode_block_count;
    std::string direct;
    std::string indirect;
    // Read Superblock
    disk->read(0, block.Data);

    printf("SuperBlock:\n");
    if (block.Super.MagicNumber == MAGIC_NUMBER) {
        printf("    magic number is valid\n");
    } else {
        printf("    magic number is invalid\n");
    }
    printf("    %u blocks\n"         , block.Super.Blocks);
    printf("    %u inode blocks\n"   , block.Super.InodeBlocks);
    printf("    %u inodes\n"         , block.Super.Inodes);

    // Read Inode blocks
    inode_block_count = block.Super.InodeBlocks;

    for (unsigned int k = 0; k < inode_block_count; k++) {
        disk->read(1+k, block.Data);
        for (unsigned int i = 0; i < INODES_PER_BLOCK; i++) {
            direct = "";
            indirect = "";
            if (!block.Inodes[i].Valid) {
                continue;
            }
            for (unsigned int j = 0; j < POINTERS_PER_INODE; j++) {
                if (block.Inodes[i].Direct[j] != 0) {
                    direct += " ";
                    direct += std::to_string(block.Inodes[i].Direct[j]);
                }
            }
            indir = block.Inodes[i].Indirect;
            if (indir != 0) {
                disk->read(indir, block2.Data);
                for (unsigned int l = 0; l < POINTERS_PER_BLOCK; l++) {
                    if (block2.Pointers[l] != 0) {
                        indirect += " ";
                        indirect += std::to_string(block2.Pointers[l]);
                    }
                }
            }
            printf("Inode %u:\n", i);
            printf("    size: %u bytes\n"    , block.Inodes[i].Size);
            printf("    direct blocks:%s\n" , direct.c_str());
            if (indirect.length() > 0) {
                printf("    indirect block: %u\n", indir);
                printf("    indirect data blocks:%s\n", indirect.c_str());
            }
        }
    }
}

// Format file system ----------------------------------------------------------

bool FileSystem::format(Disk *disk) {
    // Check if mounted
    if (disk->mounted()) {
        return false;
    }
    // Write superblock
    Block block;
    block.Super.MagicNumber = MAGIC_NUMBER;
    block.Super.Blocks = disk->size();
    block.Super.InodeBlocks = (size_t)(((float)disk->size()*0.1)+0.5);
    block.Super.Inodes = INODES_PER_BLOCK*block.Super.InodeBlocks;
    disk->write(0, block.Data);

    // Clear all other blocks
    char clear[BUFSIZ] = {0};
    for (size_t i=1; i<block.Super.Blocks; i++) {
        disk->write(i, clear);
    }

    return true;
}

// Mount file system -----------------------------------------------------------

bool FileSystem::mount(Disk *disk) {

    // Make usre disk isn't mounted
    if (disk->mounted()) {
        return false;
    }

    // Read superblock
    Block block;
    disk->read(0, block.Data);

    // make sure inodes counts match
    if (block.Super.Inodes != block.Super.InodeBlocks * INODES_PER_BLOCK) {
        return false;
    }

    // check magic number
    if (block.Super.MagicNumber != MAGIC_NUMBER) {
        return false;
    }

    // make sure there are blocks
    if (block.Super.Blocks < 0) {
        return false;
    }

    // check inode proportion
    if (block.Super.InodeBlocks != ceil(.1 * block.Super.Blocks)) {
        return false;
    }

    disk->mount();

    // Copy metadata
    num_blocks = block.Super.Blocks;
    num_inode_blocks = block.Super.InodeBlocks;
    num_inodes = block.Super.Inodes;
    this->disk = disk;

    // Allocate free block bitmap
    free_bitmap = std::vector<int> (num_blocks,1);

    //set all blocks to free initially
    for (uint32_t i = 0; i < num_blocks; i++) {
        free_bitmap[i] = 1;
    }

    //TODO: read inodes to determine which blocks are free
    for (uint32_t inode_block = 0; inode_block < num_inode_blocks; inode_block++) {
        Block b;
        disk->read(1+inode_block,b.Data);
        // reads each inode
        for (uint32_t inode = 0; inode < INODES_PER_BLOCK; inode++) {
            // if it's not valid, it has no blocks
            if (!b.Inodes[inode].Valid) {
                continue;
            }
            uint32_t n_blocks = (uint32_t)ceil(b.Inodes[inode].Size/(double)disk->BLOCK_SIZE);
            // read all direct blocks
            for (uint32_t pointer = 0; pointer < POINTERS_PER_INODE && pointer < n_blocks; pointer++) {
                free_bitmap[b.Inodes[inode].Direct[pointer]] = 0;
            }

            //read indirect block if necessary
            if (n_blocks > POINTERS_PER_INODE) {
                Block indirect;
                disk->read(b.Inodes[inode].Indirect,indirect.Data);
                for (uint32_t pointer = 0; pointer < n_blocks - POINTERS_PER_INODE; pointer++) {
                    free_bitmap[indirect.Pointers[pointer]] = 0;
                }
            }
        }
    }

    return true;
}

// Create inode ----------------------------------------------------------------

ssize_t FileSystem::create() {

    // Locate free inode in inode table
    int ind = -1;
    for (uint32_t inode_block = 0; inode_block < num_inode_blocks; inode_block++) {
        Block b;
        disk->read(1+inode_block,b.Data);
        // reads each inode
        for (uint32_t inode = 0; inode < INODES_PER_BLOCK; inode++) {
            // if it's not valid, it's free to be written
            if (!b.Inodes[inode].Valid) {
                ind = inode + INODES_PER_BLOCK*inode_block;
                break;
            }
            if (ind != -1) {
                break;
            }
        }
    }

    // Record inode if found
    if (ind == -1) {
        return -1;
    }
    Inode i;
    i.Valid = true;
    i.Size = 0;
    for (unsigned int j = 0; j < POINTERS_PER_INODE; j++) {
        i.Direct[j] = 0;
    }
    i.Indirect = 0;
    save_inode(ind, &i);

    return ind;
}

// Remove inode ----------------------------------------------------------------

bool FileSystem::remove(size_t inumber) {
    Block block;
    Inode node;
    uint32_t indir;

    // Load inode information
    if (!load_inode(inumber, &node)) {
        return false;
    }
    if (node.Valid == 0) {
        return false;
    }

    indir = node.Indirect;

    // Free direct blocks
    for (unsigned int i = 0; i < POINTERS_PER_INODE; i++) {
        node.Direct[i] = 0;
    }

    // Free indirect blocks
    if (indir != 0) {
        disk->read(indir, block.Data);
        for (unsigned int j = 0; j < POINTERS_PER_BLOCK; j++) {
            block.Pointers[j] = 0;
        }
    }

    // Clear inode in inode table
    node.Indirect = 0;
    node.Valid = 0;
    node.Size = 0;
    if (!save_inode(inumber, &node)) {
        return false;
    };
    if (indir != 0) {
        disk->write(indir, block.Data);
    }
    free_bitmap[inumber] = 0;

    return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber) {
    // Load inode information
    Inode i;
    if (!load_inode(inumber,&i) || !i.Valid) {
        return -1;
    }

    return i.Size;
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset) {
    // Load inode information
    Inode inode;
    if (!load_inode(inumber, &inode) || offset > inode.Size) {
        return -1;
    }

    // Adjust length
    length = std::min(length, inode.Size - offset);

    uint32_t start_block = offset / disk->BLOCK_SIZE;

    // Read block and copy to data; use memcpy

    // Get indirect block number if it will need it
    Block indirect;
    if ((offset + length) / disk->BLOCK_SIZE > POINTERS_PER_INODE) {
        // make sure direct block is allocated
        if (inode.Indirect == 0) {
            return -1;
        }
        disk->read(inode.Indirect,indirect.Data);
    }

    size_t read = 0;
    for (uint32_t block_num = start_block; read < length; block_num++) {
        
        // figure out which block we're reading
        size_t block_to_read;
        if (block_num < POINTERS_PER_INODE) {
            block_to_read = inode.Direct[block_num];
        } else {
            block_to_read = indirect.Pointers[block_num-POINTERS_PER_INODE];
        }

        //make sure block is allocated
        if (block_to_read == 0) {
            return -1;
        }

        //get the block -- from either direct or indirect
        Block b;
        disk->read(block_to_read,b.Data);
        size_t read_offset;
        size_t read_length;

        // if it's the first block read, have to start from an offset
        // and read either until the end of the block, or the whole request
        if (read == 0) {
            read_offset = offset % disk->BLOCK_SIZE;
            read_length = std::min(disk->BLOCK_SIZE - read_offset, length);
        } else {
            // otherwise, start from the beginning, and read
            // either the whole block or the rest of the request
            read_offset = 0;
            read_length = std::min(disk->BLOCK_SIZE-0, length-read);
        }
        memcpy(data + read, b.Data + read_offset, read_length);
        read += read_length;
    }
    return read;
}

// Write to inode --------------------------------------------------------------

ssize_t FileSystem::write(size_t inumber, char *data, size_t length, size_t offset) {
    // Load inode
    
    // Write block and copy to data
    return 0;
}

// Load inode --------------------------------------------------------------
bool FileSystem::load_inode(size_t inumber, Inode *node) {
    size_t block_number = 1 + (inumber / INODES_PER_BLOCK);
    size_t inode_offset = inumber % INODES_PER_BLOCK;

    if (inumber >= num_inodes) {
        return false;
    }

    Block block;
    disk->read(block_number,block.Data);

    *node = block.Inodes[inode_offset];

    return true;
}


// Save inode --------------------------------------------------------------
bool FileSystem::save_inode(size_t inumber, Inode *node) {

    size_t block_number = 1 + inumber / INODES_PER_BLOCK;
    size_t inode_offset = inumber % INODES_PER_BLOCK;

    if (inumber >= num_inodes) {
        return false;
    }

    Block block;
    disk->read(block_number,block.Data);
    block.Inodes[inode_offset] = *node;
    disk->write(block_number,block.Data);

    return true;
}
// vim: set sts=4 sw=4 ts=8 expandtab ft=cpp:
