# linux-prewarm-process-rfc
WIP - For now it's just an idea of how to launch processes ahead of time for bash scripts

# context / problem
shell scripts (in any flavour/variant) are incredibly composable due to unix pipelines. 
This led many people to develop a plethora CLI utilies for all sorts of problems out there
(curl, grep, sed, cat - and the list goes on and on).

The problem is that there is a long list of housekeeping actions that the operating system
must perform before creating processes such as:

- dynamic linker kicks in and load dependent libraries
- allocated physical space to accomodate virtual addresses
- read executable from disk
- allocate pid
- put process on OS scheduler (usually CFS)
- just to name a few.... there is actually much more than this.

Also, it's worth mentioning that while killing processes a lot of the aforementioned
tasks must be undone by the OS which also takes time.

As a result, even small shell script files tend to be noticible slower than equivalent
programs written in other programming languages. 

For instance, imagine that you are writing a AWS lambda runtime in Bash which doesn't do
anything else other than echoing back whatever was sent through the input payload:

```bash
#!/bin/sh

set -euo pipefail

# function handler
function handler () {
  EVENT_DATA=$1
  echo "$EVENT_DATA" 1>&2;
  RESPONSE="Echoing request: '$EVENT_DATA'"

  echo $RESPONSE
}

# Processing
while true
do
  HEADERS="$(mktemp)"
  # Get an event. The HTTP request will block until one is received
  EVENT_DATA=$(curl -sS -LD "$HEADERS" -X GET "http://${AWS_LAMBDA_RUNTIME_API}/2018-06-01/runtime/invocation/next")

  # Extract request ID by scraping response headers received above
  REQUEST_ID=$(grep -Fi Lambda-Runtime-Aws-Request-Id "$HEADERS" | tr -d '[:space:]' | cut -d: -f2)

  # Run the handler function from the script
  RESPONSE=$($(echo "$_HANDLER" | cut -d. -f2) "$EVENT_DATA")

  # Send the response
  curl -X POST "http://${AWS_LAMBDA_RUNTIME_API}/2018-06-01/runtime/invocation/$REQUEST_ID/response"  -d "$RESPONSE"
done
```

PS.: if we ignore shell built-ins such as `set` and `echo`, there are 8 processes being launched in which all 
those OS tasks are performed for each of them.

The execution time of this shell script is around **180-230ms on average**.

However, if I, functionality-wise. implement, the exact same script but in C in which I compile it all in
a single binary, I can get the execution time to be around **5-13ms on average**. 

Creating this `'monolith-binary'` is a well-known technique that embedded developers tend to use. It saves both storage
and memory space for low-resource devices. (Busybox and Toybox are well-known libraries that implement such approach).

While I'm not aiming this idea for low-resource devices, it also happens to be a way to reduce execution time of a
program (by eliminating most of the housekeeping OS tasks I mentioned earlier).


# proposed solution
This idea aims for reducing the execution times disparity between bash script and monolith-binaries for high-resource
devices such as webservers and HPC clusters. It has 2 big components to make it work as shown below:

## kernel space - new syscalls

Currently, syscall `execve(2)` initiates a binary execution in the kernel. 

From the [docs](https://man7.org/linux/man-pages/man2/execve.2.html):
> execve() executes the program referred to by pathname.  This <br/>
> causes the program that is currently being run by the calling <br/>
> process to be replaced with a new program, with newly initialized <br/>
> stack, heap, and (initialized and uninitialized) data segments.

This triggers a bunch of userspace <-> kernel interactions that eventually
run the program passed in as a parameter. The problem with that is:

1. It's an I/O blocking operation
2. Once triggered the program will run, there is no way back. That's equivalent
 of droping down a pebble at the top of a cliff

What I propose is to create 2 new syscalls:

- `prewarm`: which will do pretty much everything that `execve` does however, it will
   set a new task state to [`TASK_NEW`](https://elixir.bootlin.com/linux/v6.1.1/source/include/linux/sched.h#L100)
   on the OS scheduler so the process is never scheduled for execution until `runpid`
   syscall is triggered. (This would leverage the deferred-executions in the kernel to make it
   as async as possible).

- `runpid`: which will change existing task state to [`TASK_RUNNING`](https://elixir.bootlin.com/linux/v6.1.1/source/include/linux/sched.h#L85)
   which essentially tells the OS scheduler that it can run at scheduler's earliest
   convinience.

PS: names of syscalls are most definitely open for suggestions, I struggle with giving 
things code names. I'm horrible at this :-)

## user space - bash interpreter

My idea is to add a `shopt` option that once enabled, it would instruct bash interpreter
that upload script loading, it would:

1. extract all commands to be sent to the OS from its parser grammar. Bash already parses it's
   contents upon execution but for different purposes as of now. If you want to take a look at how
   that works, I suggest the `parse.y`file - that's a grammar file crafted for the famous YACC parser
2. make a syscall `prewarm` for every program in the script ahead of time and collect their PIDs.
3. when it's time to actually run the program, it will make a syscall `runpid` specifying the PID of the
   warmed up process.
   
So from the user's perspective, it would like something like this:

```bash
#!/bin/bash

# enable prewarm pids mechanism. When not specified, 
# bash would work as it normally does (through execve syscalls)
shopt -s prewarm_pids

echo "test" > /tmp/file.txt
cat /tmp/file.txt
rm -f /tmp/file.txt
```

## important notes

There are many ways to implement this in kernel space and I'm not 100% yet which one will be 
the easiest one / most effective one... so to avoid getting ahead of myself I listed only the
core points with the simplest implementation (new syscalls) but I'm aware that it's incredibly
hard get kernel developers to agree with a new syscall :-) So different approches will have
to be investigated too until we settle on how to make it work.

As for the Bash interpreter, I'm not too set on Bash but given that this is still arguably
the most used shell interpreter and the licenses being compatible, it may be the best candidate
for now.

## what I'm looking for out this PoC

1. I first want to make sure that the performance gain is relevant to the point that
   other people would care about (something greater than 30%). So at first I'm more interested
   in the easiest/quick-and-dirty implementation just to prove the concept. If that's good then
   I would be willing to 'do-it-right' which would account for all sorts of corner cases such as
   NUMA archictectures, programs in bash loops, CPU affinity and so on then submit it to both
   GNU Bash and Linux Kernel communities.

2. Regardless of acceptance of those communities, I would be happy to write a paper on the subject
   if the performance gain is satisfactory.
