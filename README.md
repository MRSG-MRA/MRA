MRA++ 
===

MapReduce with Adapted Algorithms to Heterogeneous Environments, is based on MRSG Project.
On September 2016 was concluded the update of MRA++ to the volatile environment, adding failure-recovery mechanisms. MRA++ now is called MapReduce with Adapted Algorithms to Volatile and Heterogeneous Environments.

To run the example, follow these steps:

1) Make sure you have installed SimGrid (3.22 or higher  recommended).
   (http://simgrid.gforge.inria.fr/)

2) Inside MRA's root and examples directories, edit the Makefiles and change
   the INSTALL_PATH variable to match your SimGrid installation path
   (e.g. /usr).

3) Execute the example (./compile_run.sh platforms/mra-plat128.xml platforms/d-mra-plat128.xml mra128.conf volatiles/parse-boinc_new.txt 0).

4)
Into examples folder, has platform samples. Install the python before for running,  in order to create another platforms.

Syntax: platform_file.xml num_workers cores_per_node_min[:numCores_max] cpu_min[:cpu_max] latency_min[:latency_max] bw_min[:bw_max]'
	
	./create-mra-plat.py platform_file.xml 5 2 1e9 1e-4 1.25e8'
	
	Or: ./create-mra-plat.py platform_file.xml 10 2 4e9:7e9 1e-4 1.25e8
	Or: ./create-mra-plat.py platform_file.xml 10 2 7e9 1e-4 1.25e6:1.25e8
	Or: ./create-mra-plat.py platform_file.xml 10 2 7e9 1e-4:1e-2 1.25e8
	Or: ./create-mra-plat.py platform_file.xml 10 2 7e9 1e-4:1e-2 1.25e6:1.25e8
	Or: ./create-mra-plat.py platform_file.xml 10 2 4e9:7e9 1e-4:1e-2 1.25e6:1.25e8

  ./create-mra-depoly.py platform_file.xml
  
5) You can change the functions for map_task_cost, reduce_task_cost or map_output inside the hello_mrsg code, or
specify the final cost on the config file.

6) More explanation can be found on the how_to_use.txt and how_create_plat.txt files


