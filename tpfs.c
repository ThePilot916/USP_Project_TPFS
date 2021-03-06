#include "tpfs.h"

static int tp_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi){
	fi = (void*) fi;
	#ifdef DEBUG
		printf("GetAttr => %s\n",path);
	#endif
	
	//check if its directory or a file and then proceed
	DIRENT *temp;
	memset(stbuf, 0, sizeof(struct stat));

	int dir_check = get_dirent(&temp,path);
	printf("dir_check exited");

	if(dir_check == 1){
		
		#ifdef DEBUGx2
			printf("Getattr-> Directory\n");
		#endif
		
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		printf("exited getattr as directory\n");
		return 0;
	}
	else if(dir_check == 0){
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = 4096;
		printf("exited getattr as file\n");

	}
	else{
		return -ENOENT;
	}
	return 0;
}

static int tp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi){
	printf("ReadDir => %s\n",path);
	
	(void) off;
	(void) fi;
	DIRENT *temp;
	temp = NULL;
	printf("Is there an error here?\n");
	int get_dirent_check = get_dirent(&temp, path);
//	printf("Exited get_dirent in readdir %d\n", temp->dirent_c);
	if(get_dirent_check == 0){
//		printf("Checking read if");
		return -ENOENT;
	}
	else{
//		printf("Check_read_else: %d\n",temp->dirent_c);

		filler(buf,".",NULL,0);
		printf("Still in read_else\n");
		filler(buf,"..",NULL,0);
		for(int i = 0; i < temp->dirent_c; i++){
			filler(buf,dirent_g[temp->dirent_l[i]].file_name,NULL,0);
			printf("path_name: %s\n",dirent_g[temp->dirent_l[i]].file_name);
		}
	}
	return 0;
}


//max_dir_ent hasnt been taken care of
static int tp_mkdir(const char *path, mode_t mode){
	#ifdef DEBUG
		printf("mkdir: %s\n",path);
	#endif
	
		DIRENT *parent_dir;
//		printf("Before the dirent call\n");
		char *new_name = get_dirent_parent(&parent_dir,path);					//return ENOTDIR if dir_check = 0
//		printf("file name: %s\n",new_name);

		int avail_dirent = get_dirent_free();
		int avail_inode = get_inode_free();
		printf("%s\n", new_name);
		if(avail_dirent >= 0 && avail_inode >= 0){
			int new_c = parent_dir->dirent_c + 1;
			
			INODE *new_inode;
			new_inode = &(inode_g[avail_inode]);
			new_inode->inode_num = avail_inode;
			new_inode->is_dir = true;
			new_inode->block_off = avail_dirent;
			new_inode->block_n = 1;
			
			DIRENT *new_dir;
			new_dir = &(dirent_g[avail_dirent]);
			strcpy(new_dir->file_name,new_name);
			new_dir->inode_num = avail_inode;
			new_dir->dirent_c = 0;
			new_dir->dirent_num = avail_dirent;

			parent_dir->dirent_l[new_c-1] = avail_dirent;
			parent_dir->dirent_c = new_c;
			
			
			freemap_g->inode_free[avail_inode] = 0;											//remove free for avail_inode
			freemap_g->dirent_free[avail_dirent] = 0;											//remove free for avail_dirent
		}
		else{
			return -ENOSPC;																							//no more dirents or inodes avail
		}	
	printf("%s\n", path);
	return 0;
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
	char *new_name = get_dirent_parent(&parent_dir,path);

	int avail_dirent = get_dirent_free();
	int avail_inode = get_inode_free();
	int avail_datablk = get_datablk_free();

	if(avail_dirent >= 0 && avail_inode >= 0 && avail_datablk >= 0){
		int new_c = parent_dir->dirent_c + 1;
		
		INODE *new_inode;
		new_inode = &inode_g[avail_inode];
		new_inode->inode_num = avail_inode;
		new_inode->is_dir = false;
		new_inode->block_off = avail_datablk;
		new_inode->block_n = 1;
		new_inode->size = 0;

		DIRENT *new_dir;
		new_dir = &dirent_g[avail_dirent];
		strcpy(new_dir->file_name, new_name);
		new_dir->inode_num = avail_inode;
		new_dir->dirent_num = avail_dirent;
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
	return 0;
}


static int tp_open(const char *path, struct fuse_file_info *fi){
	
	#ifdef DEBUG
		printf("open: %s\n",path);
	#endif
	
	INODE *temp;
	int ino_res = get_inode(&temp,path);
	if(ino_res != 0){
		return -1;
	}
	else{
		
		#ifdef DEBUG
			printf("Opened file successfully\n");
		#endif
		
		return 0;
	}
	return 0;

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
	int ino_res = get_inode(&inode,path);
	if(ino_res != 0){
		return -ENOENT;		
	}
	else{
		size_t act_len;
		act_len = inode->size;
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

int tp_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	
	#ifdef DEBUG
		printf("write: %s\n",path);
	#endif
	(void) fi;
	INODE *inode;
	int ino_res = get_inode(&inode,path);
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
			if(ip_size > wr_size_avail){
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
	.getattr = tp_getattr,
	.open = tp_open,
	.readdir = tp_readdir,
	//.init = tp_init,
	.mkdir = tp_mkdir,
	.mknod = tp_mknod,
	.read = tp_read,
	.write = tp_write,
};


int main(int argc, char *argv[]){
		
	freemap_g = (FREEMAP *)malloc(sizeof(FREEMAP));
	inode_g = (INODE *)malloc(sizeof(INODE)*INODE_MAX);
	dirent_g = (DIRENT *)malloc(sizeof(DIRENT)*DIRENT_MAX);
	datablk_g = (DATA_BLOCK *)malloc(sizeof(DATA_BLOCK)*DATA_BLOCKS);
	
	
	FILE *pers_file = fopen("pers_tpfs","r+");
	freemap_initialise(pers_file);
	inode_initialise(pers_file);
	dirent_initialise(pers_file);
	datablk_initialise(pers_file);
	if(pers_file == NULL){		
		printf("Creating a new file");							//create a new file if its not present(fresh init)
		pers_file = fopen("pers_tpfs","w+");
		DIRENT *root_dir = &dirent_g[0];
		INODE *root_inode = &inode_g[0];

		root_dir->file_name[0] = '/';
		root_dir->dirent_c = 0;
		root_dir->dirent_num = 0;
		root_dir->inode_num = 0;

		root_inode->inode_num = 0;
		root_inode->is_dir = true;

		freemap_g->inode_free[0] = 0;
		freemap_g->dirent_free[0] = 0;
	}
	else
	{
		printf("Not creating a new file");
	}
	return fuse_main(argc,argv,&tp_operations,NULL);
}















