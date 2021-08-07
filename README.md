# CXX_Thread_Pool
A thread pool for testing parallel algorithms and dispatch design in C++. Synchronization of waiting threads by spin-lock or sleeping with conditionals.

Note, because of the low-constraint nature - in an attempt to maximize potential performance - modifying the variables in the thread pool might eventually lead to deadlock.
  Specifically, with sleeping a time critical tactic is performed with initializing a lock guard for the main thread before it gives control to the daemon threads.
  Additionally, the volatile attribute of the boolean values for the spinlock case are important to ensure that the system does not think a locally cached value is valid.
  
  Feel free to change settings to see what breaks and what doesn't, but assuming compiler optimizations are turned off then the system provided as is should be quite performant
    and free of deadlock.

To set the threads to sleep, change the spin sleep macro definition in the header file to 0, for spin set it to 1.
  Change it beyond that is ill-advised and will likely spawn runtime errors.
  Spin can be substantially faster, but if the work for the threads is straining then overall greater performance might be found with sleeping.
    Notably, spin keeps the threads running at full capacity so can cause declines in clock performance as high temperatures are sustained.
      Note, this might be a useful basis to build a CPU benchmark...

While under the MIT License, please cite me if reusing this is your own work, and please enjoy!

Example cases for the functions that are usuable for the thread dispatch are provided in another repository, a research case on parallel collision detection.
