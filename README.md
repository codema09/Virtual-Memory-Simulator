# Virtual Memory Simulator

## Project Overview
This project is a C-based virtual memory simulator for multi-process environments using pure demand paging. It was developed as a course term project for the Operating Systems Lab at IIT Kharagpur under the guidance of Prof. Abhijit Das and Prof. Bivas Mitra in March 2024.

## Key Features
- Multi-process virtual memory simulation using demand paging
- Efficient page table mechanism for virtual-to-physical address translation
- Fault management system
- FCFS (First-Come, First-Served) process scheduler
- Ready queue handling and context switching mechanisms
- Process termination handling
- Least Recently Used (LRU) page-replacement algorithm integrated within the Memory Management Unit

## Technical Details

### Memory Management
- Implements virtual-to-physical address translation
- Handles page faults efficiently
- Uses LRU algorithm for page replacement

### Process Scheduling
- FCFS scheduler for task management in multi-process systems
- Efficient ready queue handling
- Context switching implementation

## Setup

### Requirements
- GCC compiler
- Make

### Building the Project
1. Clone the repository:
    ```bash
    git clone https://github.com/agrawalshivang/Virtual-Memory-Simulator
    ```
2. Compile the code:
    ```bash
    make all
    ```
3. Running the executable created:
    ```bash
    ./master arg1 arg2 arg3
    ```
4. Cleaning:
    ```bash
    make clean
    ```