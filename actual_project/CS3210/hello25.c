#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#define MAX_PROC_SIZE 1000


static unsigned long long A_PRNG = 764261123;
static unsigned long long B_PRNG = 0;
static unsigned long long C_PRNG = 2147483647;

//For Testing
static char proc_data[MAX_PROC_SIZE];
static struct proc_dir_entry *proc_write_entry;


struct threadInfo{
	pid_t pid;
	long prevRandom;
	struct threadInfo* nextThread;
};

struct processInfo{
	pid_t tgid;
	int numThreads;
	long long seed;
	struct processInfo* nextProcess;
	struct threadInfo *headThread;
};

struct processInfo *headProcess;

struct processInfo* addProcess(struct task_struct* task){

	struct processInfo *next;
	
	//check if there are any processes
	if (!headProcess){
		headProcess = (struct processInfo*)vmalloc(sizeof(struct processInfo));
	}
	//Error if fails
	if (!headProcess){
		return -EFAULT;
	}

	next=headProcess;
	while(next->nextProcess){
		next = next->nextProcess;	
	}

	//Initialize content
	next->nextProcess = (struct processInfo*)vmalloc(sizeof(struct processInfo));
	next->nextProcess->tgid = task->tgid;
	next->nextProcess->numThreads = 1;
	next->nextProcess->seed = A_PRNG;
	next->nextProcess->nextProcess = NULL;
	next->nextProcess->headThread = NULL;

	return (next->nextProcess);
	
}

struct threadInfo addThread(struct processInfo* parentProcess, struct task_struct* task){
	
	struct threadInfo *next;
	
	//check if there are any processes
	if (!(parentProcess->headThread)){
		parentProcess->headThread = vmalloc(sizeof(struct threadInfo));
	}
	//Error if fails
	if (!parentProcess->headThread){
		return -EFAULT;
	}

	next=parentProcess->headThread; 
	while(next->nextThread){
		next = next->nextThread;	
	}

	//Initialize content
	next->nextThread = vmalloc(sizeof(struct threadInfo));
	next->nextProcess->pid = task->tgid;
	next->nextProcess->prevRandom = parentProcess->;
	next->nextProcess->nextThread = NULL;

	return (next->nextThread);
}

struct processInfo findProcess(struct task_struct* task){

	struct processInfo *next = NULL;
	
	//Search through list of processess to find the matching process
	next=headProcess;
	while(next){
		if (next->tgid==task->tgid){
			return next;
		}	
	}
	//if process is not found then return NULL
	return (NULL);
	
}

struct threadInfo findThread(struct processInfo* parentProcess, struct task_struct* task){
	
	struct threadInfo *next;
	
	//Search through the task linked list for matching thread 
	next=parentProcess->headThread;
	while(next){
		if (next->pid==task->pid){
			return next;
		}	
	}
	//if not found then null
	return (NULL);
}



long nextRandomGen(long prevRandom, int numThreads){
	
	int i =0;
	long nextRandom = prevRandom;

	//Find the next random number
	for (i=0;i<numThreads;i++){
		nextRandom = (A_PRNG*nextRandom)%C_PRNG;
	}	

	return nextRandom;
}


int read_proc(char *buf,char **start,off_t offset,int count,int *eof,void *data )
{
	int len=0;
	len = sprintf(buf,"\n %s\n ",proc_data);		//Print out the data

	return len;
}

int write_proc(struct file *file,const char *buf,int count,void *data )
{

	char bufCopy[1001];
	long seed = 0;
	int numThreads = 0;
	int len = 0;
	struct processInfo curProcess = NULL;
	struct threadInfo curThread = NULL;	

	//Nothing to write
	if (count==0){
		return 0;
	}

	//Set upper limit on size
	if(count > MAX_PROC_SIZE)
	    count = MAX_PROC_SIZE;

	//Copy from user to kernal space
	if(copy_from_user(bufCopy,buf, count))
		return -EFAULT;

	//Set terminating character
		bufCopy[count] = '\0';

	//Pull out numbers from user
	sscanf(bufCopy, "%ld",&seed);
	//If no seed chosen	
	if (seed=0){
		seed = A_PRNG;
	}

	//Find if current process exists; if not create it
	curProcess = findProcess(current);
	if (curProcess!=NULL){
		curProcess=addProcess(current);
		curProcess->seed = seed;		
	}else{
		printk("Process already exists. Process not being reseeded.")
	}

	//Find if current thread exists; if not create it
	curProcess = findThread(current);
	if (curProcess!=NULL){
		curProcess=addThread(curProcess,current);		
	}

	return count;

}

void create_new_proc_entry()
{

	proc_write_entry = create_proc_entry("proc_entry",0666,NULL);
	if(!proc_write_entry)
	      {
	    printk(KERN_INFO "Error creating proc entry");
	    return -ENOMEM;
	    }
	proc_write_entry->read_proc = read_proc ;
	proc_write_entry->write_proc = write_proc;
	printk(KERN_INFO "proc initialized");

}

int proc_init (void) {
	
	create_new_proc_entry();
	return 0;

}

void proc_cleanup(void) {

	printk(KERN_INFO " Inside cleanup_module\n");
	remove_proc_entry("proc_entry",NULL);

}

MODULE_LICENSE("GPL");   
module_init(proc_init);
module_exit(proc_cleanup);

