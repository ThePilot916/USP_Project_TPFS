#include "tpfs.h"

static int *tp_init(struct fuse_conn_info *conn, struct fuse_config *cfg){
	
	#ifdef DEBUG
		printf("Initialising ThePilotFileSystem\n");
	#endif
	//all the initialise calls should happen here
}


static int tp_getattr(const char *path, struct stat *buf, struc fuse_file_info *fi){
	
	#ifdef DEBUG
		printf("GetAttr => %s\n",path);
	#endif
	
	//check if its directory or a file and then proceed
	DIRENT *temp;
	memset(stbuf, 0, sizeof(struct stat));

	int dir_check = get_dirent(temp,path);

	if(dir_check == 1){
		
		#ifdef DEBUGx2
			printf("Get<F6>attr-> Directory\n");
		#endif
		
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}
	else if(dir_check == 0){
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = 4096;
	}
	else{
		return -ENOENT;
	}
}

static int tp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi, enum fuse_readdir_flags flags){
	
	#ifdef DEBUG
		printf("ReadDir => %s\n",path);
	#endif
	
	(void) offset;
	(void) fi;
	(void) flags;
	DIRENT *temp;
	if(get_dirent(temp, path) != 0){
		return -ENOENT;
	}
	else{
		filler(buf,".",NULL,0,0);
		filler(buf,"..",NULL,0,0);
		for(int i = 0; i < temp->dirent_c; i++){
			filler(buf,dirent_g[temp->dirent_l[i]].file_name,NULL,0,0);
		}
	}
}

//max_dir_ent hasnt been taken care of
static int tp_mkdir(const char *path, mode_t mode){
	
	#ifdef DEBUG
		printf("mkdir: %s\n",path);
	#endif
	
		DIRENT *parent_dir;
		char *new_name = get_dirent_parent(parent_dir,path);					//return ENOTDIR if dir_check = 0
		
		int avail_dirent = get_dirent_free();
		int avail_inode = get_inode_free();

		if(avail_dirent >= 0  && avail_inode >= 0){
			int new_c = parent_dir->dirent_c + 1;
			
			INODE *new_inode;
			new_inode = &inode_g[avail_inode];
			new_inode->is_dir = true;
			new_inode->block_off = avail_dirent;
			new_inode->block_n = 1;
			
			DIRENT *new_dir;
			new_dir = &dirent_g[avail_dirent];
			strcpy(new_dir->file_name,new_name);
			new_dir->inode_num = avail_inode;
			new_dir->dirent_c = 0;
			
			parent_dir->dirent_c = new_c;
			parent_dir->dirent_l[new_c] = avail_dirent;
			
			freemap_g->inode_free[avail_inode] = 0;											//remove free for avail_inode
			freemap_g->dirent_free[avail_dirent] = 0;											//remove free for avail_dirent
		}
		else{
			return -ENOSPC;																							//no more dirents or inodes avail
		}
}

/*
 * Almost same implementation as for mkdir, only change is that isDir is set to false
 * and a free data_block is associated to the inode
 */

//max_dir_ent hasnt been taken care of
static int tp_mknod(const char *path, mode_t mode, dev_t d){
	
	#ifdef DEBUG
		printf("mknod: %s\n",path);
	#endif
	
	DIRENT *parent_dir;
	char *new_name = get_dirent_parent(parent_dir,path);

	int avail_dirent = get_dirent_free();
	int avail_inode = get_inode_free();
	int avail_datablk = get_datablk_free();

	if(avail_dirent >= 0 && avail_inode >= 0 && avail_datablk >= 0){
		int new_c = parent_dir->dirent_c + 1;
		
		INODE *new_inode;
		new_inode = &inode_g[avail_inode];
		new_inode->is_dir = false;
		new_inode->block_off = avail_datablk;
		new_inode->block_n = 1;
		new_inode->size = 0;

		DIRENT *new_dir;
		new_dir = &dirent_g[avail_dirent];
		strcpy(new_dir->file_name, new_name);
		new_dir->inode_num = avail_inode;
		new_dir ->dirent_c = 0;

		parent_dir->dirent_c = new_c;
		parent_dir->dirent_l[new_c] = avail_dirent;

		freemap_g->inode_free[avail_inode] = 0;
		freemap_g->dirent_free[avail_dirent] = 0;
		freemap_g->datablk_free[avail_datablk] = 0;
	}
	else{
		return -ENOSPC;																								//no more space avail
	}
}


static int tp_open(const char *path, struct fuse_file_info *fi){
	
	#ifdef DEBUG
		printf("open: %s\n",path);
	#endif
	
	INODE *temp;
	int ino_res = get_inode(temp,path);
	if(ino_res != 0){
		return -1;
	}
	else{
		
		#ifdef DEBUG
			printf("Opened file successfully\n");
		#endif
		
		return 0;
	}

}

/*
 * reads only from one block, multi block store is not implemented yet
 */

static int tp_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	
	#ifdef DEBUG
		printf("read: %s\n",path);
	#endif

	(void) fi;
	INODE *inode;
	int ino_res = get_inode(inode,path);
	if(ino_res != 0){
		return -ENOENT;		
	}
	else{
		size_t act_len;
		if(off < act_len){
			act_len = inode->size;
			if(off+size > act_len){
				size = act_len - off;
			}
			DATA_BLOCK *datablk = &datablk_g[inode->block_off];
			memcpy(buf,(datablk)+off,size);
		}
		else{
			size = 0;
		}
	}
	return size;
}

static int tp_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	
	#ifdef DEBUG
		printf("write: %s\n",path);
	#endif
	(void) fi;
	INODE *inode;
	int ino_res = get_inode(inode,path);
	if(ino_res == 1){
		return -EISDIR;
	}
	if(ino_res == -1){
		return -ENOENT;
	}
	else{
		size_t ip_size = sizeof(char)*strlen(buf);
		if(off < BLOCK_SIZE){
			size_t wr_size_avail = BLOCK_SIZE-off;
			if(ip_size > wr_size_avial){
				size = wr_size_avail;
			}
			else{
				size = ip_size;
			}
			DATA_BLOCK *datablk = &datablk_g[inode->block_off];
			memcpy((datablk)+off,buf,size);
		}
		else{
			size = 0;
		}
	}
	return size;
}


static struct fuse_operations tp_operations = {
	.init = tp_init,
	.getattr = tp_getattr,
	.readdir = tp_readdir,
	.mkdir = tp_mkdir,
	.mknod = tp_mknod,
	.open = tp_open,
	.read = tp_read,
	.write = tp_write,
};


int main(int argc, char *argv[]){
	
	TPFS = calloc(1,TPFS_SIZE);

	freemap_g = (FREEMAP *)TPFS;
	inode_g = (INODE *)(TPFS+INODE_OFF);
	dirent_g = (DIRENT *)(TPFS+DIRENT_OFF);
	datablk_g = (DATA_BLOCK *)(TPFS+DATABLK_OFF);
	
	FILE *pers_file = fopen("pers_tpfs","r+");

	freemap_initialise(pers_file);
	inode_initialise(pers_file);
	dirent_initialise(pers_file);
	datablk_initialise(pers_file);

	
}















