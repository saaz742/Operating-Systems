# OS HW0 Report

## Table of Contents
- [Setting Up The Environment](#setting-up-the-environment)
    - [Setting Up Virtual Machine](#setting-up-virtual-machine)
    - [Setting Up Git](#setting-up-git)
    - [Setting Up SSH for VSCode](#setting-up-ssh)
- [Learn to Work with Necessary Tools](#learn-to-work-with-necessary-tools)
    - [Learn Git](#learn-git)
    - [Learn Make](#learn-make)
    - [Learn GDB](#learn-gdb)
    - [Other Tools](#other-tools)
- [Start Practical Work](#start-practical-work)
    - [Words](#words)
    - [Make](#make)
    - [User Limits](#user-limits)
    - [GDB](#gdb)
    - [Compiling, Assembling, and Linking](#compiling-assembling-and-linking)

## Setting Up The Environment
Since I am working on a Windows machine, It is crucial to first set up a Linux environment to start coding. So I went through the instructions given to us. Since my Windows supports SSH, I am using PowerShell to run all of the commands.
### Setting Up Virtual Machine
I already had Oracle Virtual Box installed on my machine. So I skip to Vagrant installation. I installed the latest version of Vagrant using following command:
```
winget install Hashicorp.Vagrant
``` 
Then I used the following commands to set the virtual machine up:
```
vagrant init ce424/spring2020
vagrant up
```
Here I encountered a timeout error. I used the following command to fix it:
```
vagrant provision
```
Hopefully, this fixed the problem. I then used the following command to connect to the virtual machine via SSH:
```
vagrant ssh
```
Later, when I got my Gogs account, I ran the `vm_patch.sh` script in order to set up repositories. I ran into some problems with the script which had carriage return characters. To solve this problem, I opened the file in vim and used `:set ff=unix` command.
### Setting Up Git
I used my name and email to config git globaly. Then I used `ssh-keygen` to generate a key pair in my VM machine. Then I copied the public key to my account settings on Gogs and Github(for extra safety).
### Setting Up SSH for VSCode
Since I am used to VSCode, I want to use [Remote - SSH](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-ssh) extension to connect to the virtual machine. But before I can use it, I need to set up SSH keys. This is similar to what I did for Gogs. The difference is that I store private key on my own machine, and the public key on VM machine.

## Learn to Work with Necessary Tools
I mainly used provided links and some other tutorials on the YouTube to learn how to work with the tools. 
### Learn Git
Since I was previously familiar with Git, I only skimmed through the given resources.
### Learn Make
I used provided links, [this](https://www.youtube.com/watch?v=DtGrdB8wQ_8), [this](https://www.youtube.com/watch?v=PiFUuQqW-v8), [this](https://www.youtube.com/watch?v=GExnnTaBELk), and [this](https://www.youtube.com/watch?v=viQRxaxs70c) videos to learn how to work with Make.
### Learn GDB
I used provided links, [this](https://www.youtube.com/watch?v=bWH-nL7v5F4) video to learn how to work with GDB.
### Other Tools
For the rest of the tools, I was familiar with some of them so I only skimmed through the given resources. I also use alternative tools instead of some of them. For example I use [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) VSCode extension instead of CTags.

## Start Practical Work
### Words
First I implemented total word count and then word frequency count. I had a little bug in frequency count, so I used GDB to fix it. The implementation is in [words](./words/) directory.

### Make
I was able to explain the whole Makefile after watching the videos mentioned in [Learn Make](#learn-make). The explanation is in [Makefile.txt](./words/Makefile.txt) file.

### User Limits
First I read the `rlimit` man page and then I used `getrlimit` function in order to read resources limits. Implementation for this part is in [limits.c](./limits.c) file.

### GDB
I execute GDB commands as instructioned in the doc. The commands and outputs are in [gdb.txt](./gdb.txt) file.

### Compiling, Assembling, and Linking
For this part, I followed the given instructions. The commands and outputs are in [call.txt](./call.txt) file.
