#!/usr/bin/python

import subprocess

# Start the projectors in mono mode if not already on
output = subprocess.check_output("echo -e \":POWR?\r\" | \
        nc 192.168.0.11 1025", shell=True).rstrip().split()

# If you get 0 returned, it means projector is not powered on
if output[2][-1] == 0:
    retcode = subprocess.call("hy_start --mono",shell=True)
    if retcode == 1:
        print("Failed to start the projectros. Exiting...")
        sys.exit(1)
    else:
        print("Successfully started the projectors...")
else:
    # Sanity check to make sure projectors are in mono mode
    retcode = subprocess.call("hy_projectors_on --mono", shell=True)
    if retcode == 1:
        print("Failed to start the projectros. Exiting...")
        sys.exit(1)
    else:
        print("Projectors were already powered on...")

# Start pvserver with 8 processors in mono mode
# First, check and kill any existing pvservers running
pvserver_pids = subprocess.check_output("ps -ef | grep pvserver | awk \
        '{print $2}'", shell=True).split()
if len(pvserver_pids) > 1:
    print pvserver_pids
    for item in pvserver_pids:
        subprocess.call("kill -9 %s" %item, shell=True)

# Start the pvserver
print("No pvservers running in the background found.")
print("Starting new pvservers...")

#print pvserver_pids 
retcode = subprocess.call("echo -ne '\n' | mpiexec -np 8 pvserver \
        /usr/local/encap/paraview-v5.0.1/cave-mono.pvx &", shell=True)
if retcode == 1:
    print("Couldn't start pvserver. Exiting....")
    sys.exit(1)
else:
    print("Paraview server successfully started. Setting up VRPN server now...")
    print("You can connect to Paraview using hostname \"cube.sv.vt.edu\" and port 11111")

# Start the vrpn server
