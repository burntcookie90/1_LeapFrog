#include <linux/module.h> 
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#define MAX_PROC_SIZE 1000
#define DEBUG 1

static unsigned long long A_PRNG = 764261123;
/*static unsigned long long B_PRNG = 0;*/
static unsigned long long C_PRNG = 2147483647;

//For Testing
static char proc_data[MAX_PROC_SIZE];
static struct proc_dir_entry *proc_write_entry;

long nextRandomGen(long prevRandom, int numThreads);
/*void create_new_proc_entry();*/

struct threadInfo{
	pid_t pid;
	long nextRandom;
	struct threadInfo* nextThread;
};

struct processInfo{
	pid_t tgid;
	int numThreads;
	long seed;
	struct processInfo* nextProcess;
	struct threadInfo *headThread;
};

struct processInfo *headProcess;

struct processInfo* addProcess(struct task_struct* task){

	if(DEBUG) printk("Entering addProcess \n");
	struct processInfo *next;
	struct task_struct* taskNext;
	//check if there are any processes
	if (!headProcess){
			printk("Setting head process\n");
		headProcess = (struct processInfo*)vmalloc(sizeof(struct processInfo));
	}
	//Error if fails
	if (!headProcess){
			printk("Still no head process, sending EFAULT\n");
		return -EFAULT;
	}

	next=headProcess;
	while(next->nextProcess){
		next = next->nextProcess;	
	}

	

	//Initialize content
	if(DEBUG) printk("Initilializing content\n");
	next->nextProcess = (struct processInfo*)vmalloc(sizeof(struct processInfo));
	if(DEBUG) printk("vmallocing\n");
	next->nextProcess->tgid = task->tgid;
	next->nextProcess->seed = A_PRNG;
	next->nextProcess->nextProcess = NULL;
	next->nextProcess->headThread = NULL;
	next->nextProcess->numThreads = 1;
	if(DEBUG) printk("Initilialized content\n");
	
	taskNext = next_task(task);
	while (taskNext&&taskNext!=next){
		next->nextProcess->numThreads++;
		taskNext = next_task(taskNext);
	}
	if(DEBUG) printk("Exiting addProcess\n");

	return (next->nextProcess);
	
}

struct threadInfo* addThread(struct processInfo* parentProcess, struct task_struct* task){
	
	struct threadInfo *next;
	int threadNum = 1;
	int thread = 0;

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
		thread++;	
	}
	thread++;

	//Initialize content
	next->nextThread = vmalloc(sizeof(struct threadInfo));
	next->nextThread->pid = task->tgid;
	next->nextThread->nextRandom = 0;
	next->nextThread->nextThread = NULL;


	next->nextThread->nextRandom = nextRandomGen(parentProcess->seed, thread);

	return (next->nextThread);
}

struct processInfo* findProcess(struct task_struct* task){

	struct processInfo *next = NULL;
	
	//Search through list of processess to find the matching process
	next=headProcess;
	while(next){
		if (next->tgid==task->tgid){
			return next;
		}	
	}
	//if process is not found then return NULL
	return NULL;
	
}

struct threadInfo* findThread(struct processInfo* parentProcess, struct task_struct* task){
	
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

	char outputBuffer[1001];
	int len = 0;

	struct processInfo* curProcess;
	struct threadInfo* curThread;

	//Find if current process exists; if not create it
	curProcess = findProcess(current);
	if (curProcess==NULL){
		curProcess=addProcess(current);
		curProcess->seed = A_PRNG;		
	}else{
		printk("Process already exists. Process not being reseeded.");
	}


	//Find if current thread exists; if not create it
	curThread = findThread(curProcess,current);
	if (curThread==NULL){
		curThread=addThread(curProcess,current);		
	}
	curThread->nextRandom = nextRandomGen(curThread->nextRandom, curProcess->numThreads);
	len = sprintf(outputBuffer, "%ld", curThread->nextRandom);

	if(copy_to_user(buf, outputBuffer, len)){
		return -EFAULT;	
	}

	return len;
}

int write_proc(struct file *file,const char *buf,int count,void *data )
{
	if(DEBUG) 	printk("Entering write_proc\n");

	char bufCopy[1001];
	long seed = 0;
	/*int numThreads = 0;*/
	/*int len = 0;*/
	struct processInfo* curProcess;
	struct threadInfo* curThread;

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
	sscanf(bufCopy, "%lu",&seed);
	//If no seed chosen	
	if (seed==0){
		seed = A_PRNG;
	}

	//Find if current process exists; if not create it
	curProcess = findProcess(current);
	if (curProcess==NULL){
		curProcess=addProcess(current);
		curProcess->seed = seed;		
	}else{
		printk("Process already exists. Process not being reseeded.");
	}

	//Find if current thread exists; if not create it
	curThread = findThread(curProcess,current);
	if (curThread==NULL){
		curThread=addThread(curProcess,current);		
	}

	if(DEBUG) printk("Exiting write proc\n");
	memcpy(proc_data,bufCopy,count);

	return count;

}

void create_new_proc_entry(void)
{

	proc_write_entry = create_proc_entry("doop_entry",0666,NULL);
	if(!proc_write_entry) {
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

module_init(proc_init);
module_exit(proc_cleanup);
