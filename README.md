# linux-daemons
## 1. an analogue of the cron daemon.  
the daemon uses input config file to specify what tasks to run and when  
[man cron.8](http://man7.org/linux/man-pages/man8/cron.8.html)
  
### signals:  
* __SIGINT__  - daemon starts to execute commands from the very start of the config file  
* __SIGALRM__ - is thrown by alarm timer to execute a command and set a new one  
* __SIGTERM__ - termination of the daemon  
  
### use:
* __compile__: $ make daemon  
* __run__: $ ./daemon input_config.txt  
* __or both__: $ ./run.sh  
