#ifndef TPFS
#define TPFS

//#define DEBUG inorder to debug

/*
 * Change to 30 if this doesn't work
 */

#define USE_FUSE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>


/*
 *
 *
 *Notation info: 
 *	_l -> list of its prefix
 *	_c -> count of its prefix
 *	_g -> prefix is of type global
 *
 *
 */


#define BLOCK_SIZE 4096

#define DIRENT_PER_DIR 10										//entries per directory
#define DIRENT_MAX	50											//Max number of dirents allowed
#define INODE_MAX (DIRENT*DIRENT_PER_DIR)		//Max number of GLOBAL inodes
#define DATA_BLOCKS (INODE_MAX)							//Total number of data_blocks available
#define FILE_NAME_MAX 256										//Dirent_filename max size

char *TPFS;																	//Whole of the filesystem

typedef struct inode{

	bool is_dir;
	int block_off;												//offset value to data_block from datablock begin, begin will be different if its a directory, it'll give data_block to the dirent instead.
	int block_n;													//number of data_blocks being used by it, currently useless
	size_t size;													//only used if its not a directory
}INODE;

typedef struct dirent{
	char file_name[FILE_NAME_MAX];				//file/directory name
	int dirent_l[DIRENT_PER_DIR];					//list of entries assosciated to their direct_g index
	int dirent_c;													//number of entries present;
	int inode_num;												//inode associated to the current dirent
}DIRENT;

typedef struct data_block{
	char data[BLOCK_SIZE];								//size of each data block
}DATA_BLOCK;

typedef struct freemap{
	int inode_free[INODE_MAX];
	int dirent_free[DIRENT_MAX];
	int datablk_free[DATA_BLOCKS];
}FREEMAP;


FREEMAP *freemap_g;
INODE *inode_g;
DIRENT *dirent_g;
DATA_BLOCK *datablk_g;



/*
 *
 * THE PERSISTENCE FILE IS CALLED pers_tpfs and it'll present in same directory from where the code executes.
 * 
 *
 * values defined here are for persistence file partitioning
 * File begins with freemaps, inode entry(entries equal to max number of inodes), followed by dirents
 * and then by the actuall data_blocks for files contents and stuff
 * Freemap entry has 0 offset from the begining of the file
 * INODE_OFF will give offset to the first inode entry
 * DIRENT_OFFSET will give offset to the first dirent in the file
 * DATABLK_OFF will give offset to the first datablk in the file
 *
 ****NOTE: None of these offsets are to be mistaken for first free location for any of the entities
 ****
 *
 * all of these will be checked for bounds 
 */ 


#define FREEMAP_OFF 0
#define INODE_OFF (sizeof(FREEMAP))
#define DIRENT_OFF (sizeof(INODE)*INODE_MAX+INODE_OFF)
#define DATABLK_OFF (sizeof(DIRENT)*DIRENT_MAX+DIRENT_OFF)


#define TPFS_SIZE (sizeof(INODE)*INODE_MAX+sizeof(FREEMAP)+sizeof(DIRENT)*DIRENT_MAX+DATA_BLOCKS*BLOCK_SIZE)			// SIZEOF the whole filesystem being created

/* 
 * Prototypes for the calls being implemented
 * Close prototpe not avail in FUSE.h
 */

static int tp_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int tp_open(const char *path, struct fuse_file_info *fi);
static int tp_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi);
static int tp_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi);

static int tp_getattr(const char *path, struct stat *buf, struct fuse_file_info *fi);
static int tp_opendir( const char *path, struct fuse_file_info *fi);
static int tp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
static int tp_mknod(const char *path, mode_t mode, dev_t rdev);
static int tp_mkdir(const char *path, mode_t mode);

static int *tp_init(struct fuse_conn_info *conn, struct fuse_config *cfg);


/*
 * Helper Functions
 */

int get_dirent(DIRENT *dirent, char *path);
int get_inode(INODE *inode, char *path);
char* get_dirent_parent(DIRENT *dirent, char *path);
int get_inode_free();
int get_dirent_free();
int get_datablk_free();

/*
 *
 * Persistence Helper function prototypes
 *
 */

int inode_initialise(FILE *fp);
int dirent_initialise(FILE *fp);
int datablk_initialise(FILE *fp);
int freemap_initialise(FILE *fp);

int inode_write(FILE *fp,off_t off,INODE *buff);
int dirent_write(FILE *fp,off_t off,DIRENT *buff);
int datablk_write(FILE *fp,off_t off,char *buff);
int freemap_write(FILE *fp,off_t off,FREEMAP *buff);

#endif
