//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"


// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *curproc = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd] == 0){
      curproc->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

int
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

int
sys_read(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return fileread(f, p, n);
}

int
sys_write(void)
{
  struct file *f;
  int n;
  char *p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
    return -1;
  return filewrite(f, p, n);
}

int
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

int
sys_fstat(void)
{
  struct file *f;
  struct stat *st;

  if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
int
sys_link(void)
{
  char name[DIRSIZ], *new, *old;
  struct inode *dp, *ip;

  if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

//PAGEBREAK!
int
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], *path;
  uint off;

  if(argstr(0, &path) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;
  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && ip->type == T_FILE)
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

int
sys_open(void)
{
  char *path;
  int fd, omode;
  struct file *f;
  struct inode *ip;

  if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  end_op();

  f->type = FD_INODE;
  f->ip = ip;
  f->off = 0;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
  return fd;
}

int
sys_mkdir(void)
{
  char *path;
  struct inode *ip;

  begin_op();
  if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_mknod(void)
{
  struct inode *ip;
  char *path;
  int major, minor;

  begin_op();
  if((argstr(0, &path)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEV, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

int
sys_chdir(void)
{
  char *path;
  struct inode *ip;
  struct proc *curproc = myproc();
  
  begin_op();
  if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  end_op();
  curproc->cwd = ip;
  return 0;
}

int
sys_exec(void)
{
  char *path, *argv[MAXARG];
  int i;
  uint uargv, uarg;

  if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv))
      return -1;
    if(fetchint(uargv+4*i, (int*)&uarg) < 0)
      return -1;
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    if(fetchstr(uarg, &argv[i]) < 0)
      return -1;
  }
  return exec(path, argv);
}

int
sys_pipe(void)
{
  int *fd;
  struct file *rf, *wf;
  int fd0, fd1;

  if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      myproc()->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  fd[0] = fd0;
  fd[1] = fd1;
  return 0;
}

static int
inode_copy(struct inode *src, struct inode *dst)
{
  char buf[512];
  uint off = 0;
  ilock(src);
  while(off < src->size){
    int toread = sizeof(buf);
    if(src->size - off < toread)
      toread = src->size - off;
    if(readi(src, buf, off, toread) != toread){
      iunlock(src);
      return 1;
    }
    if(writei(dst, buf, off, toread) != toread){
      iunlock(src);
      return 1;
    }
    off += toread;
  }
  iunlock(src);
  return 0;
}

static int
build_dst_path(char *dst, const char *src)
{
  int n = strlen(src);
  if(n + 5 >= MAXPATH)  
    return -1;

  safestrcpy(dst, src, MAXPATH);
  safestrcpy(dst + n, "_copy", MAXPATH - n);
  return 0;
}

int
sys_make_duplicate(void)
{
  char *src_path;
  char dst_path[MAXPATH];
  if(argstr(0, &src_path) < 0)
    return 1;
  if(build_dst_path(dst_path, src_path) < 0)
    return 1;
  begin_op();
  struct inode *src = namei(src_path);
  if(src == 0){
    end_op();
    return -1;
  }
  ilock(src);
  if(src->type != T_FILE){
    iunlock(src);
    iput(src);     
    end_op();
    return 1;
  }
  iunlock(src);
  struct inode *exist = namei(dst_path);
  if(exist){
    iput(exist);   
    iput(src);     
    end_op();
    return 1;
  }
  extern struct inode* create(char *path, short type, short major, short minor);
  struct inode *dst = create(dst_path, T_FILE, 0, 0);
  if(dst == 0){
    iput(src);     
    end_op();
    return 1;
  }
  int rc = inode_copy(src, dst);
  iunlockput(dst); 
  iput(src);     
  end_op();
  return rc ? 1 : 0;
}

static int k_strstr(const char *hay, int hlen, const char *needle) {
  int nlen = 0;
  while (needle[nlen]) nlen++;
  if (nlen == 0) return 0;
  for (int i = 0; i + nlen <= hlen; i++) {
    int ok = 1;
    for (int j = 0; j < nlen; j++) {
      if (hay[i+j] != needle[j]) { ok = 0; break; }
    }
    if (ok) return i;
  }
  return -1;
}

// simple substring search (no libc). returns start index or -1.
static int in_line(const char *hay, int hlen, const char *needle, int nlen) {
  if (nlen == 0) return 0;
  for (int i = 0; i + nlen <= hlen; i++) {
    int ok = 1;
    for (int j = 0; j < nlen; j++) {
      if (hay[i + j] != needle[j]) { ok = 0; break; }
    }
    if (ok) return i;
  }
  return -1;
}


int
sys_grep_syscall(void)
{
  char *keyword;
  char *filename;
  char *ubuf;
  int bufsize;

  if (argstr(0, &keyword)  < 0) return -1;
  if (argstr(1, &filename) < 0) return -1;
  if (argint(3, &bufsize)  < 0 || bufsize <= 1) return -1;
  if (argptr(2, &ubuf, bufsize) < 0) return -1;

  int klen = 0;
  while (keyword[klen]) klen++;

  begin_op();
  struct inode *ip = namei(filename);
  if (ip == 0) {
    end_op();
    return -1;
  }
  ilock(ip);

  // allocate a page-sized temp buffer for block reads
  char *kbuf = kalloc();
  if (kbuf == 0) {
    iunlockput(ip);
    end_op();
    return -1;
  }

  // build lines and search
  char line[512];
  int  llen = 0;
  int  off  = 0;
  int  found = 0;
  int  retlen = -1;

  while (off < ip->size) {
    int n = readi(ip, kbuf, off, PGSIZE);
    if (n <= 0) break;
    off += n;

    for (int i = 0; i < n; i++) {
      char c = kbuf[i];

      if (c == '\n' || llen == (int)sizeof(line) - 1) {
        if (in_line(line, llen, keyword, klen) >= 0) {
          int tocpy = (llen < bufsize - 1) ? llen : (bufsize - 1);
          if (copyout(myproc()->pgdir, (uint)ubuf, line, tocpy) < 0) {
            kfree(kbuf); iunlockput(ip); end_op(); return -1;
          }
          char nul = 0;
          copyout(myproc()->pgdir, (uint)(ubuf + tocpy), &nul, 1);
          cprintf("Matched line length: %d\n", llen);
          found = 1; retlen = tocpy;
          goto done;
        }
        // reset for next line
        llen = 0;
      } else {
        line[llen++] = c;
      }
    }
  }

  // handle last line if file doesn't end with '\n'
  if (!found && llen > 0) {
    if (in_line(line, llen, keyword, klen) >= 0) {
      int tocpy = (llen < bufsize - 1) ? llen : (bufsize - 1);
      if (copyout(myproc()->pgdir, (uint)ubuf, line, tocpy) < 0) {
        kfree(kbuf); iunlockput(ip); end_op(); return -1;
      }
      char nul = 0;
      copyout(myproc()->pgdir, (uint)(ubuf + tocpy), &nul, 1);
      cprintf("Matched line length: %d\n", llen);
      found = 1; retlen = tocpy;
    }
  }


done:
  kfree(kbuf);
  iunlockput(ip);
  end_op();
  return found ? retlen : -1;
}