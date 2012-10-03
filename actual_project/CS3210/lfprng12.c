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
#define PROC_FILENAME "bleep_entry"

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

	struct processInfo *next;
	struct task_struct* taskNext;
	if(DEBUG) printk("Entering addProcess \n");
	//check if there are any processes
	if (!headProcess){
			printk("Setting head process\n");
			headProcess = (struct processInfo*)vmalloc(sizeof(struct processInfo));
			//Initialize content
			if(DEBUG) printk("Initilializing head process\n");
			headProcess= (struct processInfo*)vmalloc(sizeof(struct processInfo));
			if(DEBUG) printk("vmallocing head process\n");
			headProcess->tgid = task->tgid;
			headProcess->seed = A_PRNG;
			headProcess->nextProcess = NULL;
			headProcess->headThread = NULL;
			headProcess->numThreads = 1;
			if(DEBUG) printk("Initilialized headprocess\n");
			
			taskNext = next_task(task);
			while (taskNext&&(taskNext->pid!=task->pid)){
					if(DEBUG) printk("In while loop for head process, numThreads:%d task->tgid:%d task->pid:%d taskNext->pid:%d\n",headProcess->numThreads,task->tgid,task->pid,taskNext->pid);
					headProcess->numThreads++;
					taskNext = next_task(taskNext);
			}
			if(DEBUG) printk("Exiting head process initialization\n");

			return (headProcess);
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
	while (taskNext&&taskNext->pid!=task->pid){
			if(DEBUG) printk("In while loop for task, numThreads:%d task->tgid:%d task->pid:%d taskNext->pid:%d\n", next->nextProcess->numThreads,task->tgid,task->pid,taskNext->pid);
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
	
	if(DEBUG) printk("entering addThread\n");
	//check if there are any processes
	if (!(parentProcess->headThread)){
			if(DEBUG) printk("checking if there are any processes\n");
			parentProcess->headThread = vmalloc(sizeof(struct threadInfo));
			if(DEBUG) printk("vmallocing parentProcess->headThread\n");
			//Initialize content
			if(DEBUG) printk("Initilializing content\n");
			parentProcess->headThread = vmalloc(sizeof(struct threadInfo));
			parentProcess->headThread->pid = task->pid;
			parentProcess->headThread->nextRandom = 0;
			parentProcess->headThread->nextThread = NULL;

			parentProcess->headThread->nextRandom = nextRandomGen(parentProcess->seed,1);
			if(DEBUG) printk("Exiting addThread \"headThread section\"\n");
			return (parentProcess->headThread);
	}
	//Error if fails
	if (!parentProcess->headThread){
			if(DEBUG) printk("initilizing process failed\n");
		return -EFAULT;
	}

	next=parentProcess->headThread; 
	while(next->nextThread){
		next = next->nextThread;	
		thread++;	
	}
	thread++;

	//Initialize content
	if(DEBUG) printk("Initilializing content\n");
	next->nextThread = vmalloc(sizeof(struct threadInfo));
	if(DEBUG) printk("vmallocing next->nextThread\n");
	next->nextThread->pid = task->pid;
	next->nextThread->nextRandom = 0;
	next->nextThread->nextThread = NULL;


	next->nextThread->nextRandom = nextRandomGen(parentProcess->seed, thread);

	return (next->nextThread);
}

struct processInfo* findProcess(struct task_struct* task){

	struct processInfo *next = NULL;
	
	if(DEBUG) printk("Entering findProcess\n");
	//Search through list of processess to find the matching process
	next=headProcess;
	while(next){
			if(DEBUG) printk("Entering while loop in findProcess\n");
		if (next->tgid==task->tgid){
			return next;
		}	
		next = next->nextProcess;
	}
	//if process is not found then return NULL
	return NULL;
	
}

struct threadInfo* findThread(struct processInfo* parentProcess, struct task_struct* task){
	
	struct threadInfo *next;
	
	if(DEBUG) printk("Entering findThread\n");
	//Search through the task linked list for matching thread 
	next=parentProcess->headThread;
	while(next){
		if(DEBUG) printk("findThread's while loop\n");
		if (next->pid==task->pid){
			return next;
		}	

		next = next->nextThread;
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

	if(DEBUG) printk("Entering read_proc\n");

	//Find if current process exists; if not create it
	curProcess = findProcess(current);
	if (DEBUG) curProcess = headProcess;
	if (curProcess==NULL){
		if(DEBUG) printk("current process does not exist\n");
		curProcess=addProcess(current);
		curProcess->seed = A_PRNG;
		if(DEBUG) printk("numThreads:%lu", curProcess->numThreads);		
	}else{
		printk("Process already exists. Process not being reseeded");
	}


	//Find if current thread exists; if not create it
	curThread = findThread(curProcess,current);
	if (curThread==NULL){
		if(DEBUG) printk("creating curret thread\n");
		curThread=addThread(curProcess,current);		
	}
	len = sprintf(outputBuffer, "%ld", curThread->nextRandom);
	if(DEBUG) printk("nextRandom: %lu\n",curThread->nextRandom);
	curThread->nextRandom = nextRandomGen(curThread->nextRandom, curProcess->numThreads);
	if (DEBUG) printk("nextnextRandom %lu\n", curThread->nextRandom);
	if (DEBUG) printk("thread %d", curThread->pid);
	memcpy(buf, outputBuffer, len);

	/*if()){*/
		/*if(DEBUG) printk("copy to user failed\n");*/
		/*return -EFAULT;	*/
	/*}*/

	if(DEBUG) printk("Leaving read_proc\n");
	return len;
}

int write_proc(struct file *file,const char *buf,int count,void *data )
{

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
		/*bufCopy[count] = '\0';*/

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

	if(DEBUG) 	printk("Exiting write_proc seed:%lu s\n",seed);
	return count;

}

void create_new_proc_entry(void)
{

	proc_write_entry = create_proc_entry(PROC_FILENAME,0666,NULL);
	if(!proc_write_entry) {
	    printk(KERN_INFO "Error creating proc entry");
	    return -ENOMEM;
	    }
	proc_write_entry->read_proc = read_proc ;
	proc_write_entry->write_proc = write_proc;
	printk(KERN_INFO "proc initialized\n");

}

int proc_init (void) {
	
	create_new_proc_entry();
	return 0;

}

void proc_cleanup(void) {

	printk(KERN_INFO " Inside cleanup_module\n");
	remove_proc_entry(PROC_FILENAME,NULL);

}

module_init(proc_init);
module_exit(proc_cleanup);

