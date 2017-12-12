// fs.cpp: File System

#include "sfs/fs.h"

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>

// Debug file system -----------------------------------------------------------

void FileSystem::debug(Disk *disk) {
    Block block;
    unsigned int inode_count;
    std::string direct;
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
    inode_count = block.Super.Inodes;
    disk->read(1, block.Data);

    for (unsigned int i = 0; i < inode_count; i++) {
        direct = "";
        for (unsigned int j = 0; j < POINTERS_PER_INODE; j++) {
            if (block.Inodes[i].Direct[j] != 0) {
                direct += std::to_string(block.Inodes[i].Direct[j]);
                direct += " ";
            }
        }
        if (!block.Inodes[i].Valid) {
            continue;
        } else {
            printf("Inode %u:\n", i);
            printf("    size: %u bytes\n"    , block.Inodes[i].Size);
            printf("    direct blocks: %s\n" , direct.c_str());
        }    
    }
}

// Format file system ----------------------------------------------------------

bool FileSystem::format(Disk *disk) {
    // Write superblock

    // Clear all other blocks
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
    disk->mount();
    // Copy metadata
    num_blocks = block.Super.Blocks;
    num_inode_blocks = block.Super.InodeBlocks;
    num_inodes = block.Super.Inodes;

    // Allocate free block bitmap
    free_bitmap = (uint32_t*)calloc(num_blocks,sizeof(uint32_t));

    //TODO: read inodes to determine which blocks are free?

    return true;
}

// Create inode ----------------------------------------------------------------

ssize_t FileSystem::create() {
    // Locate free inode in inode table

    // Record inode if found
    return 0;
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
    size_t block_number = inumber / INODES_PER_BLOCK;
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

    size_t block_number = inumber / INODES_PER_BLOCK;
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
