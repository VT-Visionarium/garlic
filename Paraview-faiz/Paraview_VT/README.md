This repository contains some basic code to connect to 
HPC servers at Virginia Tech, connect ParaView with the servers,
upload a scene file, etc. At the time of writing this documenation, ParaView
5.2.0 was not installed on the cluster. Thus, I had it installed in my home
directory. Please load ParaView 5.2.0 module if it has been installed on the
clusters by then. Else, install it in your home directory.

1. ssh to NewRiver using the script ssh_newriver.bash
2. Request a node using the script request_node.bash
3. On the node that you got, load some modules using the script
   load_modules.bash as "source load_modules.bash".
4. Go back to your laptop/desktop where ParaView is installed and 
	create a ssh tunnel using the script create_ssh_tunnel.bash
5.

