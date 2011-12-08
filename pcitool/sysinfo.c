#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#define MEMINFO_FILE "/proc/meminfo"
#define MTAB_FILE "/etc/mtab"

#define BAD_OPEN_MESSAGE					\
"Error: /proc must be mounted\n"				\
"  To mount /proc at boot you need an /etc/fstab line like:\n"	\
"      /proc   /proc   proc    defaults\n"			\
"  In the meantime, run \"mount /proc /proc -t proc\"\n"

/* This macro opens filename only if necessary and seeks to 0 so
 * that successive calls to the functions are more efficient.
 * It also reads the current contents of the file into the global buf.
 */
#define FILE_TO_BUF(filename) do{				\
    static int fd, local_n;					\
    if ((fd = open(filename, O_RDONLY)) == -1) {		\
	fputs(BAD_OPEN_MESSAGE, stderr);			\
	fflush(NULL);						\
	_exit(102);						\
    }								\
    lseek(fd, 0L, SEEK_SET);					\
    if ((local_n = read(fd, buf, sizeof buf - 1)) < 0) {	\
	perror(filename);					\
	fflush(NULL);						\
	_exit(103);						\
    }								\
    buf[local_n] = '\0';					\
    close(fd);							\
}while(0)


typedef struct mem_table_struct {
  const char *name;     /* memory type name */
  unsigned long *slot; /* slot in return struct */
} mem_table_struct;

static int compare_mem_table_structs(const void *a, const void *b){
  return strcmp(((const mem_table_struct*)a)->name,((const mem_table_struct*)b)->name);
}

size_t get_free_memory(void){
  char buf[4096];
  unsigned long kb_main_buffers, kb_main_cached, kb_main_free;
  char namebuf[16]; /* big enough to hold any row name */
  mem_table_struct findme = { namebuf, NULL};
  mem_table_struct *found;
  char *head;
  char *tail;

  const mem_table_struct mem_table[] = {
    {"Buffers",      &kb_main_buffers}, // important
    {"Cached",       &kb_main_cached},  // important
    {"MemFree",      &kb_main_free},    // important
  };
  const int mem_table_count = sizeof(mem_table)/sizeof(mem_table_struct);

  FILE_TO_BUF(MEMINFO_FILE);

  head = buf;
  for(;;){
    tail = strchr(head, ':');
    if(!tail) break;
    *tail = '\0';
    if(strlen(head) >= sizeof(namebuf)){
      head = tail+1;
      goto nextline;
    }
    strcpy(namebuf,head);
    found = bsearch(&findme, mem_table, mem_table_count,
        sizeof(mem_table_struct), compare_mem_table_structs
    );
    head = tail+1;
    if(!found) goto nextline;
    *(found->slot) = strtoul(head,&tail,10);
nextline:
    tail = strchr(head, '\n');
    if(!tail) break;
    head = tail+1;
  }
  
  return (kb_main_buffers + kb_main_cached + kb_main_free) * 1024;
}


int get_file_fs(const char *fname, size_t size, char *fs) {
  int err = 0;
  char buf[4096];
  char *fn;

  char *head;
  char *tail;

  size_t len, max = 0;
  struct stat st;
  
  if ((!fname)||(!fs)||(size < 3)) return -1;
  
  if (*fn == '/') {
    fn = (char*)fname;
  } else {
    if (!getcwd(buf, 4095)) return -1;
    fn = malloc(strlen(fname) + strlen(buf) + 2);
    if (!fn) return -1;
    sprintf(fn, "%s/%s", buf, fname);
  }
  
  if (!stat(fn, &st)) {
    if (S_ISBLK(st.st_mode)) {
	strcpy(fs, "raw");
	goto clean;
    }
  }
  
  FILE_TO_BUF(MTAB_FILE);

  head = buf;
  for(;;){
    head = strchr(head, ' ');
    if(!head) break;

    head += 1;
    tail = strchr(head, ' ');
    if(!tail) break;
    
    *tail = '\0';

    len = strlen(head);
    if((len <= max)||(strncmp(head, fn, len))) {
      head = tail+1;
      goto nextline;
    }
    
    head = tail + 1;
    tail = strchr(head, ' ');
    if(!tail) break;

    *tail = '\0';

    if (!strncasecmp(head,"root",4)) {
	head = tail+1;
	goto nextline;
    }
    
    max = len;

    if (strlen(head) >= size) err = -1;
    else {
	err = 0;
	strcpy(fs, head);
    }
    
    head = tail+1;
nextline:
    tail = strchr(head, '\n');
    if(!tail) break;
    head = tail+1;
  }

clean:  
  if (fn != fname) free(fn);

puts(fs);
  return err;
}
