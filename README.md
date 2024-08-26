# runtimeOS (WORK IN PROGRESS)

(Somewhat) an operating system for microcontrollers. It allows for multiple processes to run and be managed.  
Emulates multithreading by switching between processes really fast. Processes are only able to access
content in their own memory areas. 

# runtimeOS script language

The code from a script file will be parsed and converted to byte code, which can be  
executed as a new process on the microcontroller running rtOS.
