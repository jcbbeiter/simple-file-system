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
                    direct += std::to_string(block.Inodes[i].Direct[j]);
                    direct += " ";
                }
            }
            indir = block.Inodes[i].Indirect;
            if (indir != 0) {
                disk->read(indir, block2.Data);
                for (unsigned int l = 0; l < POINTERS_PER_BLOCK; l++) {
                    if (block2.Pointers[l] != 0) {
                        indirect += std::to_string(block2.Pointers[l]);
                        indirect += " ";
                    }
                }
            }
            printf("Inode %u:\n", i);
            printf("    size: %u bytes\n"    , block.Inodes[i].Size);
            if (direct.length() > 0) {
                direct.pop_back();
            }
            printf("    direct blocks: %s\n" , direct.c_str());
            if (indirect.length() > 0) {
                indirect.pop_back();
                printf("    indirect block: %u\n", indir);
                printf("    indirect data blocks: %s\n", indirect.c_str());
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
    // Read superblock
    Block block;
    disk->read(0, block.Data);

    // Set device and mount
    if (disk->mounted()) {
        return false;
    }

    if (block.Super.Inodes != block.Super.InodeBlocks * INODES_PER_BLOCK) {
        return false;
    }

    if (block.Super.MagicNumber != MAGIC_NUMBER) {
        return false;
    }

    if (block.Super.Blocks < 0) {
        return false;
    }

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
    free_bitmap = (uint32_t*)calloc(num_blocks,sizeof(uint32_t));

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
    // Load inode information

    // Free direct blocks

    // Free indirect blocks

    // Clear inode in inode table
    return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t FileSystem::stat(size_t inumber) {
    // Load inode information
    return 0;
}

// Read from inode -------------------------------------------------------------

ssize_t FileSystem::read(size_t inumber, char *data, size_t length, size_t offset) {
    // Load inode information

    // Adjust length

    // Read block and copy to data
    return 0;
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
