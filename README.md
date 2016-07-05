------------------------------------------
Description of the working of the program:
------------------------------------------
The program is a peer to peer file sharing network which somewhat emulates a Bit Torrent. The peers have their own files which they are ready to share(depending on the cases mentioned) with others who request them. Once the files are in the network they are accessible to the other nodes based on the group assigned to them. It has an added advantage when a same file is being accessed by many peers at the same time.'

-------------------------------------------
Working of the program:
-------------------------------------------
The program takes input a manager.conf file which has initial configuration information. The manager reads the config file and sends this information to the clients using TCP after forking them. The manager then terminates as no further action is required from its end. The Tracker which is the primary node in this infrastructure communicates its UDP port number to the manager via a TCP connection. The UDP port number is the one on which the clients communicate for the group interest and group assign messages.
These messages follow a certain packet format which is described below. The tracker keeps all the information about the files and the nodes which consist of a single group. There is a group pertaining to each file which consists of all interested in sharing it.

At the start of the program there should essentially be atleast a single node which consiss of the entire file. All the details pertaining to the message exchange and other functionalites are grouped in various sections.

The file then creates replicas with the bits transferred from ts neighbours on a group. In this speciific program 32 bit segment is transferred at once. After every 8 segments the node asks the tracker to assign a different group. This avoids the same nodes to be busy all the time and adds more number of neighbours to get file and also decrease the time in getting the file.Each node will get the requested file in the format as node_no-original_file_name.
 
Note: All ports are generated dynamically

---------------------------------------------------------------------
PROGRAM FLOW
---------------------------------------------------------------------
Below is the basic funcitoning of the program and how it is implemented
1) The Manager spawns th tracker and clients and terminates
	-- TCP connection is established with the Tracker
	--Tracker sends its UDP port number to the Manager
	--Manager sends config information(which is stored in a structure in the program) to the clients after forking the number of clients 	mentioned in the config file.
2)The clients in the program will mention if they are the seed nodes or they want to download the file specified. This program will work for multiple files in cases where the groups are separate. 
	--Group show interest is sent to the Tracker on the UDP port. Tracker replies with a Group Assign Packet containing a list of neighbours
	--The message format is followed in these cases. However, as structures are used for implementation, the entire packet will be transferred consisting of information about 25 nodes(Maximum that can be held by the program)

	Tracker has 3 cases :
	1. Interested node is not ready to share
	2. Interested node is ready to share
	3. Interested node is the seed, so it will not send GROUP_ASSIGN to it.
	--CLNT_INFO_REQ is sent to the individual node to identify the segments they possess in a CLNT_INFO_REP message.
3) Based upon the group assign packet which consists of a list of neighbours, request is sent to get the number of segments. 
	--As there will be multiple clients trying to talk to the same client, SELECT is used to queue the requests and it ensures the requests are not lost. 	
	--All the process related to file download for a partivcular client is done in a separate thread which is combined once the entire file is downloaded. There will be multiple group updates. After every 8 segments have been sent the Node will go back to the Tracker to get the other neighbours.
4) The algorithm followed is ROUND ROBIN(for every set of neigbours in a single group update). 
	--The probability of the initial node being contacted is high in this case as after every group update, the initial neighbour is the same.
	--SEG_INFO_REQ and SEG_INFO_REP are the messages used for transfer in this case.
5)Based on the algorithm the entire file is downloaded and the node can now transfer files to other nodes.
6)Termination for the tracker is 30 seconds after no activity, for clients is 100 seconds after no activity and the Manager terminates as soon as it spawns all the clients and tracker.

-------------------
Reused Code: 
-------------------
Certain code fragments were used from stackoverflow and references have been cited. Moreover, the code from references was modified as per the requirements and then used. Certain examples were referred (Example: Understanding the working of SELECT) and have been incorporated by manipulationg them.

-----------------------------
Message format
-----------------------------
 Client to Client uses 4 types of messages as below:
1) CLNT_INFO_REQ (Asks for which segments the other client has)
2) CLNT_INFO_REP (Client sends the segments it has -> Binary array is maintained to keep a track of it)
3)SEG_INFO_REQ(Based on the ROUND ROBIN Algorithm used it will send request to one of the neighbours)
4)SEG_INFO_REP(Client will send the required segment)
Once the download is completed the node can enter the group and become available to other nodes.
Message format has been followed as required. However, as structures are being used, size of neighbours has been fixed to 25.
No extra fields were added in any of the formats that were specified.

Number of files is 1 for a paticular node. There can be separate nodes(not similar ones) fovarious files:
ex: seed 1 1.jpg				seed 3 2.jpg
    2 wants to download 1.jpg 			4 wants to download 2.jpg
Thus different nodes will contribute to multiple files being transferred. However, program will not handle multiple file exchanges for the same nodes.

Program handles files being requested at the same time and works for files upto 20KB.
Packet delay has been used before sending any packet.
Will work fine even if all nodes want to share their data
Idiosyncrasies:
1)Will not have any transfer with loss implementation.
2)Multiple files for the same node(Initially will not work). A single node can request a single file. Althiugh the network will have multile files being exchanged among a group of nodes.
3)Only one group show interest will be sent to the tracker per node intitally. After every 8 segments anther one will be sent.
4)The program will work for the clients mentioned initially in the config file. Extra inputs will be ignored based on it. 

---------------------------------------
Protocol corner cases: 
----------------------------------------
1. One to one transfer - Works fine for all the cases(File size of 20kb has been tested. Image files have also been tested.)
2. Many-to-Many transfer - Works fine for all the cases (Tested for same start time and all available for sharing)
3. Transfer with multiple files - Works fine but a single node can either request a file or store a file.--> The program can be scaled to handle these. Would implement them by forking in the client too.
4. Transfer with loss. - Not implemented
The program will fail for multiple files being sent in a single group interest packet as it considers only one file per node(Initially)
Performance improves if there are multiple clients in the network who are ready to share. ROUND ROBIN implementation will however put more load on the one being listed the earliest in the list.

All the output formats have been followed and raw data has been printed in hex and all the message types have been captured in the log .out files.

Termination method:Tracker terminates after 30 seconds. Manager will terminate as soon as it finishes spawning the tracker and the clients specified in the config file.

Start time has been used to allow the node to wait before sending a download request. Seed nodes send the request to the Tracker as sson as the program starts.

Few data structures have been declared global to be accessed by the thread for file download.
------------------------------------------------------
Behaviour under certain scenarios
------------------------------------------------------
1)If there are multiple files for a client, it will take input as the last one amongst them
2)Works fine for same start times. 
3)File will be created in the specified format.
4)If the file is not on the disk or if there is no seed node for the file, message will be printed.
5)25 clients and 20KB(640 segments) file size can be used
6)The program can be scaled and multiple files for the same client will be implemented by modifying certain conditions which are causing overhead.
7)Many to Many works with multiple files for different groups wherein every node can either act as a source node or request for file).
8)Group update facilitates faster completion of file download. If there is no segment update and if the file has not been completely downloaded, it will have a request timeout as mentioned.
9)It has assumed that the file mentioned in the config file will be present by the same name. 

********IMP: ex: if 1.jpg is mentioned in the program, it should read from 1.jpg itself , it will not prefix it initially with the seed node.*******

Header file contains all the required data structures. MakeFile removes the .out files and kills the processes(if left).
Future Work:
To work for multiple files and to transfer messages in a more efficient way.

References:
http://beej.us/guide/bgnet/output/html/multipage/inet_ntoaman.html -> For use of select for handling multiple requests at the same time
http://stackoverflow.com/
http://www.tcpipguide.com/free/t_TCPIPDataLinkLayerProtocolsSerialLineInternetProto.htm

