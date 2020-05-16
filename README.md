# linux-daemons
an analogue of the cron daemon.  
the daemon uses input config file to specify what tasks to run and when  
  
signals:  
SIGINT - daemon starts to execute commands from the very start of the config file  
SIGALRM - is thrown by alarm timer to execute a command and set a new one  
SIGTERM - termination of the daemon  
  
to compile: $ make daemon  
to run: $ ./daemon input_config.txt  
or $ ./run.sh  
