#include "file.h"
#include "defs.h"
#include "fcntl.h"
#include "fs.h"
#include "proc.h"

//This is a system-level open file table that holds open files of all process.
struct file filepool[FILEPOOLSIZE];

//Abstract the stdio into a file.
struct file *stdio_init(int fd)
{
	struct file *f = filealloc();
	f->type = FD_STDIO;
	f->ref = 1;
	f->readable = (fd == STDIN || fd == STDERR);
	f->writable = (fd == STDOUT || fd == STDERR);
	return f;
}

//The operation performed on the system-level open file table entry after some process closes a file.
void fileclose(struct file *f)
{
	if (f->ref < 1)
		panic("fileclose");
	if (--f->ref > 0) {
		return;
	}
	switch (f->type) {
	case FD_STDIO:
		// Do nothing
		break;
	case FD_INODE:
		iput(f->ip);
		break;
	default:
		panic("unknown file type %d\n", f->type);
	}

	f->off = 0;
	f->readable = 0;
	f->writable = 0;
	f->ref = 0;
	f->type = FD_NONE;
}

//Add a new system-level table entry for the open file table
struct file *filealloc()
{
	for (int i = 0; i < FILEPOOLSIZE; ++i) {
		if (filepool[i].ref == 0) {
			filepool[i].ref = 1;
			return &filepool[i];
		}
	}
	return 0;
}

//Show names of all files in the root_dir.
int show_all_files()
{
	return dirls(root_dir());
}

//Create a new empty file based on path and type and return its inode;
//if the file under the path exists, return its inode;
//returns 0 if the type of file to be created is not T_file
static struct inode *create(char *path, short type)
{
	struct inode *ip, *root;
	root = root_dir(); //Remember that the root_inode is open in this step,so it needs closing then.
	// ivalid(root);
	if ((ip = dirlookup(root, path, 0)) != 0) {
		warnf("create a exist file\n");
		iput(root); //Close the root_inode
		ivalid(ip);
		if (type == T_FILE && ip->type == T_FILE)
			return ip;
		iput(ip);
		return 0;
	}
	if ((ip = ialloc(root->dev, type)) == 0)
		panic("create: ialloc");

	tracef("create dinode and inode type = %d\n", type);

	ivalid(ip);
	iupdate(ip);
	if (dirlink(root, path, ip->inum) < 0)
		panic("create: dirlink");

	iput(root);
	return ip;
}

//A process creates or opens a file according to its path, returning the file descriptor of the created or opened file.
//If omode is O_CREATE, create a new file
//if omode if the others,open a created file.
int fileopen(char *path, uint64 omode)
{
	int fd;
	struct file *f;
	struct inode *ip;
	if (omode & O_CREATE) {
		ip = create(path, T_FILE);
		if (ip == 0) {
			return -1;
		}
	} else {
		if ((ip = namei(path)) == 0) {
			return -1;
		}
		ivalid(ip);
	}
	if (ip->type != T_FILE)
		panic("unsupported file inode type\n");
	if ((f = filealloc()) == 0 ||
	    (fd = fdalloc(f)) <
		    0) { //Assign a system-level table entry to a newly created or opened file
		//and then create a file descriptor that points to it
		if (f)
			fileclose(f);
		iput(ip);
		return -1;
	}
	// only support FD_INODE
	f->type = FD_INODE;
	f->off = 0;
	f->ip = ip;
	f->readable = !(omode & O_WRONLY);
	f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
	if ((omode & O_TRUNC) && ip->type == T_FILE) {
		itrunc(ip);
	}
	return fd;
}

// Write data to inode.
uint64 inodewrite(struct file *f, uint64 va, uint64 len)
{
	int r;
	ivalid(f->ip);
	if ((r = writei(f->ip, 1, va, f->off, len)) > 0)
		f->off += r;
	return r;
}

//Read data from inode.
uint64 inoderead(struct file *f, uint64 va, uint64 len)
{
	int r;
	ivalid(f->ip);
	if ((r = readi(f->ip, 1, va, f->off, len)) > 0)
		f->off += r;
	return r;
}

int linkat(uint64 oldroothpath, uint64 newpath) {
	struct proc *p = curr_proc();
	char oldname[200], newname[200];

	if (copyinstr(p->pagetable, oldname, oldroothpath, 200) < 0) return -1;
	if (copyinstr(p->pagetable, newname, newpath, 200) < 0) return -1;
	
	struct inode* root = root_dir();
	if (root == 0) return -1;
	struct inode* ip = dirlookup(root, oldname, 0);
	if (ip == 0) {
		iput(root);
		return -1;
	}

	ivalid(ip);
	// if isi6 directory or link target already exists, return -1
	if (ip->type == T_DIR || dirlink(root, newname, ip->inum) < 0) {
		iput(ip);
		iput(root);
		return -1;
	}

	ip->nlink += 1;
	iupdate(ip);
	iput(ip);
	iput(root);
	return 0;
}

// unlink a file/dir from path 
int unlinkat(uint64 path) {
	struct proc *p = curr_proc();
	char name[200];
	if (copyinstr(p->pagetable, name, path, 200) < 0) return -1;
	
	struct inode* root = root_dir();
	if (root == 0) return -1;
	struct inode* ip = dirlookup(root, name, 0);
	if (ip == 0) {
		iput(root);
		return -1;
	}

	ivalid(ip);
	if (dirunlink(root, name) < 0) {
		iput(ip);
		iput(root);
		return -1;
	}
	if (ip->nlink > 0) {
		ip->nlink -= 1;
		iupdate(ip);
	}
	iput(ip);
	iput(root);
	return 0;
}

// fd: file descriptor
// st: user pointer that the stat structure will be copied to
// return statistics of current file
int fstat(int fd, uint64 st) {
	// illegal st address
	if (st == 0) return -1;

	struct proc* p = curr_proc();

	// invalid fd
	if (fd < 0 || fd >= FD_BUFFER_SIZE) return -1;

	struct file* file = p->files[fd];
	if (file == NULL) return -1;
	if (file->type != FD_INODE || file->ip == 0) return -1;

	struct inode *ip = file->ip;
	ivalid(ip);

	struct Stat stat;
	// device
	stat.dev = ip->dev;
	// inode number
	stat.ino = ip->inum;
	// file / dir
	stat.mode = ip->type == T_FILE ? FILE : DIR;
	// link count
	stat.nlink = ip->nlink;
	if (copyout(p->pagetable, st, (void *) &stat, sizeof(stat)) < 0) {
		return -1;
	}
	return 0;
}