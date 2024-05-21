
/*
S_IRWXU
    Read, write, and search, or execute, for the file owner; S_IRWXG is the bitwise inclusive-OR of S_IRUSR, S_IWUSR, and S_IXUSR.
S_IRUSR
    Read permission for the file owner.
S_IWUSR
    Write permission for the file owner.
S_IXUSR
    Search permission (for a directory) or execute permission (for a file) for the file owner.
///
S_IRWXG
    Read, write, and search or execute permission for the file's group. S_IRWXG is the bitwise inclusive-OR of S_IRGRP, S_IWGRP, and S_IXGRP.
S_IWGRP
    Write permission for the file's group.
S_IRGRP
    Read permission for the file's group.
S_IXGRP
    Search permission (for a directory) or execute permission (for a file) for the file's group.
///
S_IROTH
    Read permission for users other than the file owner.
*/
int iofile (process *p, tok_t arg[]) {
     int idx = 1; 
     int out, in;
     while(idx < MAXTOKS && arg[idx]) {
        switch(arg[idx][0]) {
            case '>':
            //Open for writing only.
            out = open(arg[++idx], O_WRONLY|O_CREAT, S_IRWXU|S_IRWXG|S_IROTH);
            if(out != -1) {
                p->stdout = out;
                arg[idx-1] = arg[idx] = NULL;
                break;
            }
            else{
                fprintf(stderr, "failed to open\n");
                return -1;
            }
            case '<':
            //Open for reading only.
            in = open(arg[++idx], O_RDONLY);
            if(in != -1) {
                p->stdout = in;
                arg[idx-1] = arg[idx] = NULL;
                break;
            }
            else{
                fprintf(stderr, "failed to open\n");
                return -1;
            }
            }
        idx++;
     }
     return 0;
}

// first  arg[IN]:  executable file
// second arg[OUT]: valid path
int path_resolution(const char* name, char pathname[], int size) {
   int pos;
   int i, j;
   char * cur_path; 
   if(name == NULL || name[0] == '.' || name[0] == '/') 
	    return 1;	
   char * path_env = getenv("PATH");
   for(pos = 0; path_env[pos]; pos++) {
        for(i = 0; i < size && path_env[pos+i] && path_env[pos+i] != ':'; i++) 
			pathname[i] = path_env[pos+i];
	    if(path_env[pos+i] == '\0')
		    break;
	    else
		    pos += i;
	    if(pathname[i-1] != '/') pathname[i++] = '/';
        for(j = 0; i < size && name[j]; i++, j++)
            pathname[i] = name[j];
        if(i < size) 
            pathname[i] = '\0';
        else {
            fprintf(stderr, "size: %d, i: %d\n", size, i);
            perror("path_resolution failed: pathname over bound");
        }
        if(access(pathname, X_OK) == 0) 
            return 0;
   }
   return 1; 
}




int cmd_exec(tok_t arg[]) {
   int wstatus;
   int pid;
   char pathname[MAX_FILE_SIZE+1];
   if(arg == NULL || arg[0] == NULL) {
	perror("cmd_exec: invalide argument\n");
	return -1;
   }
   process* cur_process = (process*)malloc(sizeof(process));
   assert(cur_process != NULL);
   cur_process->argv = arg;
   cur_process->completed = cur_process->stopped = 0;
   cur_process->stdin = cur_process->stdout = -1;
   cur_process->tmodes = shell_tmodes;
   (cur_process->tmodes).c_lflag |= (IEXTEN | ISIG | ICANON ); 
   cur_process->background = process_background_sign(arg); // foreground process by default

   if(path_resolution(arg[0], pathname, MAX_FILE_SIZE) != 0) 
      strncpy(pathname, arg[0], MAX_FILE_SIZE); 
   if(io_redirection(cur_process, arg) != 0) {
        fprintf(stderr, "Failed to process input/output redirection\n");
        exit(-1);
   }
   add_process(cur_process);
   pid = fork();
   if(pid == 0) {
        cur_process->pid = getpid();
        launch_process(cur_process);
        if(execv(pathname, arg) == -1) { 
            fprintf(stderr, "Failed to exec: %s\n", arg[0]);
            exit(-2);
        }
        perror("Child process unexpected trace");
        exit(-3);
   }
   else if(pid < 0) {
        fprintf(stderr, "Failed to exec: %s\n", arg[0]);
        return -1;
   }
   // parent process
   else {
        // sync pgid info in parent child
        cur_process->pid = pid;
        setpgid(pid, pid);
        if(!cur_process->background) 
            put_process_in_foreground(cur_process, 0);
        else 
            put_process_in_background(cur_process, 0);
   }

   return 1;		
}











// first  arg[IN]:  executable file
// second arg[OUT]: valid path
int path_resolution(const char* file, char path[], int size) {
   int idx;
   int i, j;
   if(file == NULL || file[0] == '.' || file[0] == '/') 
	    return 1;	
   char * env = getenv("PATH");
   for(idx = 0; env[idx]; idx++) {
        for(i = 0; i < size && env[idx+i] && env[idx+i] != ':'; i++) 
			path[i] = env[idx+i];
	    if(env[idx+i] == '\0')
		    break;
	    else
		    idx += i;
	    if(path[i-1] != '/') //end with '/'
             path[i++] = '/';
        for(j = 0; i < size && file[j]; i++, j++)
            path[i] = file[j];
        if(i < size) 
            path[i] = '\0';
        // if(access(path, X_OK) == 0) 
        //     return 0;
   }
   return 1; 
}

int cmd_exec(tok_t arg[]) {
   int wstatus;
   int pid;
   char pathname[MAX_FILE_SIZE+1];
   if(arg == NULL || arg[0] == NULL) 
	return -1;
   process* cur_process = (process*)malloc(sizeof(process));
   assert(cur_process != NULL);
   cur_process->argv = arg;
   cur_process->completed = cur_process->stopped = 0;
   cur_process->stdin = cur_process->stdout = -1;
   cur_process->tmodes = shell_tmodes;
   (cur_process->tmodes).c_lflag |= (IEXTEN | ISIG | ICANON ); 
   if(path_resolution(arg[0], pathname, MAX_FILE_SIZE) != 0) 
      strncpy(pathname, arg[0], MAX_FILE_SIZE); 
   add_process(cur_process);
   pid = fork();
   if(pid == 0) {
        cur_process->pid = getpid();
        launch_process(cur_process);
   }
   else if(pid < 0) 
        return -1;
   // parent process
   else {
        // sync pgid info in parent child
        cur_process->pid = pid;
        setpgid(pid, pid);
   }

   return 1;		
}