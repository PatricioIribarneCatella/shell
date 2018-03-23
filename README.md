# shell

Basic shell interpreter.

### Abstract

This project was developed from the scratch and motivated by the topics taught in Operating Systems, imparted in FIUBA (Engineering University of Buenos Aires). For example, some of them could be: *processes*, *virtual memory*, *file system*, *interruptions* and *signal handling*, among others.  



### Features 

- Command execution with or without arguments
- Environment and magic variables expansion
- Flow redirection and pipe
- Command execution with their own environment variables
- Background processes (just one for now)  


### Examples

- Command execution

```bash
$ ps
  PID TTY          TIME CMD
 4930 pts/2    00:00:03 zsh
30599 pts/2    00:00:00 ps
```

```bash
$ df -H /tmp
Filesystem      Size  Used Avail Use% Mounted on
tmpfs           8.3G  2.0M  8.3G   1% /tmp
```

- Environment variables expansion

```bash
$ echo $TERM
xterm-16color

$ echo $?
0

$ /bin/false
$ echo $?
1
```

- Standar flow redirection

```bash
$ ls -C /home /notexist >out.txt 2>err.txt
$ echo $?
2

$ cat out.txt
/home:
patricio

$ cat err.txt
ls: cannot access '/notexist': No such file or directory
```

- Pipe

```bash
$ ls -C /home /notexist 2>&1 | wc -l
3
```

- Command execution with their own environment variables

```bash
$ USER=nobody ENVIRON=nothing /usr/bin/env | grep "=no"
USER=nobody
ENVIRON=nothing
```

- Background processes

```bash
$ evince somefile.pdf &
$ 

=== time passed and the user close the document viewer ===

$ 	process 31755 done [evince somefile.pdf], status: 0
$ 
```



### Compile and Run

```bash
$ make run
```

For cleaning up:

```bash
$ make clean
```

