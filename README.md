# linux-daemons
## 1. an analogue of the cron daemon.  
the daemon uses input config file to specify what tasks to run and when  
  
### signals:  
* __SIGINT__  - daemon starts to execute commands from the very start of the config file  
* __SIGALRM__ - is thrown by alarm timer to execute a command and set a new one  
* __SIGTERM__ - termination of the daemon  
  
### use:
* __compile__:```shellscript $ make daemon```  
* __run__:```shellscript $ ./daemon input_config.txt```  
* or both:```shellscript $ ./run.sh```  
