CSE.30341.FA17: Project 06
==========================

This is the documentation for [Project 06] of [CSE.30341.FA17].

Members
-------

1. Domer McDomerson (dmcdomer@nd.edu)
2. Belle Fleur (bfleur@nd.edu)

Design
------

> 1. To implement `Filesystem::debug`, you will need to load the file system
>    data structures and report the **superblock** and **inodes**.
>
>       - How will you read the superblock?
>       - How will you traverse all the inodes?
>       - How will you determine all the information related to an inode?
>       - How will you determine all the blocks related to an inode?

To read the superblock, we will call read(0,data) to read the data in the first
block, then cast that data pointer to the Block union. Then, we can read the 
superblock fields through the "Super" member.
To traverse all the inodes, we will first read the number of inodes from the
SuperBlock. Then, we can read the inodes by, starting at block 1, iterating
through the inodes by using the Inodes[] member of the union.
We will determine all the information related to an inode simply by reading
the fields from the struct Inode in the Inodes array.
All of the blocks related to the inode are the ones in the Inode.Direct array,
as well as any in the block pointed to by the Inode.Indirect field.

> 2. To implement `FileSystem::format`, you will need to write the superblock
>    and clear the remaining blocks in the file system.
>
>       - What pre-condition must be true before this operation can succeed?
>       - What information must be written into the superblock?
>       - How would you clear all the remaining blocks?

Before the operation can succeed, the disk must not be mounted already.
When formatting, the magic number, number of blocks, number of inode blocks,
and number of inodes must be written.
We would clear all the remaining blocks by iterating across all of the inodes
and setting the Valid member to zero (i.e. false)

> 3. To implement `FileSystem::mount`, you will need to prepare a filesystem
>    for use by reading the superblock and allocating the free block bitmap.
>
>       - What pre-condition must be true before this operation can succeed?
>       - What sanity checks must you perform?
>       - How will you record that you mounted a disk?
>       - How will you determine which blocks are free?

Before the operation can succeed, the disk must not be mounted already.
We must check the following: that the magic number is correct, that the number of blocks
described by the superblock is the same as the number of disks on the disk, that the
number of inode blocks is reasonable, and that the number of inodes matches the total
number of inodes that would be in that many blocks.
We will record that we mounted a disk by incrementing the "Mounts" member variable
in the disk.
We will determine which blocks are free by scanning the inodes and seeing which
blocks are pointed to by valid inodes.

> 4. To implement `FileSystem::create`, you will need to locate a free inode
>    and save a new inode into the inode table.
>
>       - How will you locate a free inode?
>       - What information would you see in a new inode?
>       - How will you record this new inode?

We will locate a free inode by performing a linear scan along the inodes and
finding the first one that is free.
We would see in a free inode Valid set to zero. The other fields may be garbage.
We would record the new inode during create by seting Valid to 1, and setting
each other field of the Inode struct to its proper value (Size, filling Direct
with as many pointers as needed, and setting Indirect to a block if necessary,
zero otherwise).


> 5. To implement `FileSystem::remove`, you will need to locate the inode and
>    then free its associated blocks.
>
>       - How will you determine if the specified inode is valid?
>       - How will you free the direct blocks?
>       - How will you free the indirect blocks?
>       - How will you update the inode table?

We will determine if the specified inode is valid by checking its Valid member.
We will free the direct blocks by zeroing all of the blocks pointed to by each
entry of the Direct array.
We will free the indirect blocks by going to the block that is pointed to by the
Indirect member, and iterating through all of the pointers it contains, zeroing
the data in the blocks that each pointer points to.
Finally, we can clear the data members of the inode struct and set Valid to zero.

> 6. To implement `FileSystem::stat`, you will need to locate the inode and
>    return its size.
>
>       - How will you determine if the specified inode is valid?
>       - How will you determine the inode's size?

We will determine if the specified inode is valid by checking the Valid member
of the inode struct.
We will determine the inode's size by reading the Size field of the inode struct.

> 7. To implement `FileSystem::read`, you will need to locate the inode and
>    copy data from appropriate blocks to the user-specified data buffer.
>
>       - How will you determine if the specified inode is valid?
>       - How will you determine which block to read from?
>       - How will you handle the offset?
>       - How will you copy from a block to the data buffer?

We will determine if the specified inode is valid by checking the Valid member
of the inode struct.
We will determine which block(s) to read from by finding how many blocks in
the specified offset would be. If that block offset is less than the number
of blocks in Direct, we can simply read from the block pointed to by that entry
of Direct. Otherwise, we need to scan through the indirect block for the data
pointer.
Once we find the data block to read from, we will handle the offset by starting
the read from (offset % BLOCK_SIZE) bytes in.
We will copy from a block to the data buffer using memcpy.


> 8. To implement `FileSystem::write`, you will need to locate the inode and
>    copy data the user-specified data buffer to data blocks in the file
>    system.
>
>       - How will you determine if the specified inode is valid?
>       - How will you determine which block to write to?
>       - How will you handle the offset?
>       - How will you know if you need a new block?
>       - How will you manage allocating a new block if you need another one?
>       - How will you copy from a block to the data buffer?
>       - How will you update the inode?

We will determine if the specified inode is valid by checking the Valid member
of the inode struct.
We will determine which block(s) to write to by finding how many blocks in
the specified offset would be. If that block offset is less than the number
of blocks in Direct, we can simply write to the block pointed to by that entry
of Direct. Otherwise, we need to scan through the indirect block for the data
pointer.
Once we find the data block to write to, we will handle the offset by starting
the read from (offset % BLOCK_SIZE) bytes in.
To allocate a new block if we need one, we will add a new pointer to the Direct
array if it has room, or to the indirect block if it doesn't.
We will copy from a block to the data buffer using memcpy.
We will update the size field of the inode if we grew the data at all (instead
of overwriting), and update the Direct array if we allocated a new direct block,
and the Indirect pointer if we allocated the indirect block

Errata
------

> Describe any known errors, bugs, or deviations from the requirements.

Extra Credit
------------

> Describe what extra credit (if any) that you implemented.

[Project 06]:       https://www3.nd.edu/~pbui/teaching/cse.30341.fa17/project06.html
[CSE.30341.FA17]:   https://www3.nd.edu/~pbui/teaching/cse.30341.fa17/
[Google Drive]:     https://drive.google.com
