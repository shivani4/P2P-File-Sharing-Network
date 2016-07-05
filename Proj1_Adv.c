#include "p2p.h"



void TrackerCreation(int pid,int port_serv)
{
	char track_file[32];
	strcpy(track_file,"Tracker.out");
	track_fp = fopen(track_file, "a");
	fprintf(track_fp,"Type:Tracker\n");
	fprintf(track_fp,"Pid: %d\n",pid);
	//Creating a TCP socket for Tracker
	struct sockaddr_in Tracker_TCP,Manager_TCP;
	int addrlen,port,Tracker_TCP_Socket;   
	addrlen=sizeof(Tracker_TCP);  
	memset(&Tracker_TCP, 0, sizeof(Tracker_TCP));  
	Tracker_TCP.sin_family = AF_INET;  
	Tracker_TCP.sin_addr.s_addr=INADDR_ANY;
	Tracker_TCP.sin_port=htons(0);
	int i,j;
	if((Tracker_TCP_Socket = socket(PF_INET, SOCK_STREAM, 0))<0)
	{
		printf("Tracker TCP socket failed!\n");
		exit(0);
	}
	bind(Tracker_TCP_Socket, (struct sockaddr *) &Tracker_TCP, sizeof(Tracker_TCP));  
	getsockname(Tracker_TCP_Socket,(struct sockaddr*)&Tracker_TCP,&addrlen);  
	
	
	//Assigning detials of server and establishing a connection(TCP- since it will send the config info)
	memset(&Manager_TCP, 0, sizeof(Manager_TCP));     /* Zero out structure */
    Manager_TCP.sin_family      = AF_INET;             /* Internet address family */
    Manager_TCP.sin_addr.s_addr = INADDR_ANY;   /* Server IP address */
    Manager_TCP.sin_port        = htons(port_serv); /* Server port */
	//getsockname(Tracker_TCP_Socket,(struct sockaddr*)&Tracker_TCP,&addrlen); 
	if (connect(Tracker_TCP_Socket, (struct sockaddr *) &Manager_TCP, sizeof(Manager_TCP)) < 0)
	{
		printf("Error in Connection to Server");
		exit(0);
	}
	
	
	//Creating a UDP connection for Tracker to connect to the clients. Send this to Manager 
	int Tracker_UDP_Socket, nBytes;
	char buffer[1024];
	struct sockaddr_in Tracker_UDP,Tracker_UDP_client,UDP_connection_to_Client;
	socklen_t addr_size, client_addr_size;
	 if((Tracker_UDP_Socket = socket(PF_INET, SOCK_DGRAM, 0))<0)
	 {
		 printf("Socket failed!");
		 exit(0);
	}
	memset(&Tracker_UDP, 0, sizeof(Tracker_UDP));     /* Zero out structure */ 
	/*Configure settings in address struct*/
	Tracker_UDP.sin_family = AF_INET;
	Tracker_UDP.sin_port = 0;
	Tracker_UDP.sin_addr.s_addr = INADDR_ANY;
	/*Bind socket with address struct*/
	if((bind(Tracker_UDP_Socket, (struct sockaddr *) &Tracker_UDP, sizeof(Tracker_UDP)))<0)
	{
		printf("Bind failed");
	}
	
	/*Initialize size variable to be used later on*/
	addr_size = sizeof(Tracker_UDP);
	getsockname(Tracker_UDP_Socket,(struct sockaddr*)&Tracker_UDP,&addr_size);
	
	
	
	//Sending UDP port number via TCP connection
	char Buffer[100]; /* Buffer*/
	int recvMsgSize=100; /*Size of received message*/
	sprintf(Buffer,"%d",ntohs(Tracker_UDP.sin_port));//Storing UDP port number in buffer and sending through TCP connection
	port=ntohs(Tracker_UDP.sin_port);
	fprintf(track_fp,"Port number: %d\n",port);
	//fprintf(fp,"Client ID      TimeReceived      Filename     Raw Reply Message \n");
	fclose(track_fp);
	flag=1;
	if ((send(Tracker_TCP_Socket, Buffer, recvMsgSize, 0)) != recvMsgSize)
	{
		printf("Send failed!");
		exit(0);
	}
	//printf("Tracker created successfully!\n");
	
	
	//Tracker will start listening for UDP connections using select to queue the multiple requests
	fd_set readfds;
    int numfd;
    struct timeval tv;
    FD_ZERO(&readfds);
    
    numfd = Tracker_UDP_Socket + 1;
	int recv_len;
	char buf[500];
	struct Group_Show_interest packet_group_show_interest;
	int total_files=0;
	//static
	Group_Assign group_assign_packet;
	static struct File_maintenance file_data[25];
	memset(&file_data,0, sizeof(file_data));
	int x,y;
	for(x=0;x<25;x++)
	{
		for(y=0;y<25;y++)
		{
			file_data[x].neighbour_node_number[y]=99;
		}
	}
	
	
	
	static int neighbour_port_number;
	//Code to listen for UDP requests from Clients
	while(1)
	{
		tv.tv_sec = 30;//Time for which the tracker should wait before terminating
		FD_SET(Tracker_UDP_Socket, &readfds);
		printf("Tracker Waiting to receive requests\n");
		int recieve = select(numfd, &readfds, NULL, NULL, &tv);
		if (recieve == -1) 
		{
			perror("select"); // error occurred in select()
        } 
		else if (recieve == 0) 
		{
          printf("Timeout occurred!  No data after 30 seconds.\n");
		  break;
        }
		else
		{
			if (FD_ISSET(Tracker_UDP_Socket, &readfds)) 
			{
				printf("**************Enters here!!!*********\n");
                // FD_CLR(s, &readfds);
                memset(&packet_group_show_interest,0,sizeof(packet_group_show_interest));
                if((recvfrom(Tracker_UDP_Socket, (void*)&packet_group_show_interest,sizeof(packet_group_show_interest), 0, (struct sockaddr *) &Tracker_UDP_client, &addr_size)) <= 0)
				{
					printf("Receiving in Tracker failed");
					exit(0);
				}
				
				//Printing the data to the file
				
				clock_t dif=clock()-before;
				int seconds=dif/ CLOCKS_PER_SEC;
				track_fp = fopen(track_file, "a");
				fprintf(track_fp,"%d       %d        %s\n\n",seconds,packet_group_show_interest.client_node_id,packet_group_show_interest.filename);
				fclose(track_fp);
				
				int client_port_number;
				client_port_number=ntohs(Tracker_UDP_client.sin_port);
				char client_addr[32];
				strcpy(client_addr,inet_ntoa(Tracker_UDP_client.sin_addr));
				//printf("Packet received from: %d\n",client_port_number);
					
				memset(&group_assign_packet,0,sizeof(group_assign_packet));
				memset((char *) &UDP_connection_to_Client, 0, sizeof(UDP_connection_to_Client));
				printf("\nType:%d\n",packet_group_show_interest.type);
				
				switch(packet_group_show_interest.type)
					{
						case 1://Not willing to share. Do not add this to the group but send Group assign packet to it
						{	printf("\n In case 1 in the Tracker\n");
						
							//saving the details to connect back to the client. Can be sent directly from where it is received.However, extra caution!
							UDP_connection_to_Client.sin_family = AF_INET;
							UDP_connection_to_Client.sin_port = htons(client_port_number);
							UDP_connection_to_Client.sin_addr.s_addr = INADDR_ANY;
						
						
							int flag_check=0;
							group_assign_packet.msgtype=2;
							group_assign_packet.number_of_files=1;
							
							//loop to get the location(index) of the file
							for(i=0;i<total_files;i++)
							{
								if((strcmp(packet_group_show_interest.filename,file_data[i].file_name))==0)
								{
									flag_check=1;
									break;
								}
							}
							
							//to check whether the file exists in any group or not
							if(flag_check==0)
							{
								printf("There is no seed node for this file\n\n");
								exit(0);
							}
							else if(flag_check==1)//start preparing the group assign packet
							{
								strcpy(group_assign_packet.filename,packet_group_show_interest.filename);
								group_assign_packet.number_of_neighbours=file_data[i].number_of_neighbours;
								for(j=0;j<group_assign_packet.number_of_neighbours;j++)
								{
									group_assign_packet.neighbour_id[j]=file_data[i].neighbour_node_number[j];
									group_assign_packet.neighbour_port[j]=file_data[i].neighbour_port[j];
								}
								sprintf(buf,"%d",group_assign_packet.msgtype);
								//sending message type to enter the switch case at the client's end--will be 2 always
								if (sendto(Tracker_UDP_Socket, buf, sizeof(buf) , 0 , (struct sockaddr *) &UDP_connection_to_Client,addr_size)<=0)
								{
									//printf("Sending to Client failed from Tracker\n");
									exit(0);
								}
								
								printf("Message number 2 has been sent to the client\n");
								//Actual packet containing Group Assign Packet
								if (sendto(Tracker_UDP_Socket, (void*)&group_assign_packet, sizeof(group_assign_packet) , 0 , (struct sockaddr *) &UDP_connection_to_Client,addr_size)<=0)
								{
									//printf("Sending to Client failed from Tracker\n");
									exit(0);
								}
								printf("Tracker has sent the group Assign packet to client\n");
								
								int cnting=0;
								int chi=0;
								
								//Print in hex
								char hostn_udp[32];
								struct hostent *he_udp;
								unsigned char* file_ptr;
								char *store;
								int i1,i2,i3,i4;
								gethostname(hostn_udp ,sizeof(hostn_udp));
								he_udp=gethostbyname(hostn_udp);
								track_fp=fopen(track_file,"a");
								fprintf(track_fp,"00 %02x 00 %02x ",group_assign_packet.msgtype,group_assign_packet.number_of_files);
								file_ptr = (unsigned char*)group_assign_packet.filename;
								for(chi=0 ;chi<32;chi++){
									fprintf(track_fp,"%02x ",(unsigned int)file_ptr[chi]);
								}
								fprintf(track_fp,"00 %02x ",group_assign_packet.number_of_neighbours);
								for(cnting=0;cnting<group_assign_packet.number_of_neighbours;cnting++)
								{
									fprintf(track_fp,"00 %02x ",group_assign_packet.neighbour_id[cnting]);
									store=inet_ntoa(*(struct in_addr*)he_udp->h_addr);
									sscanf(store,"%d.%d.%d.%d",&i1,&i2,&i3,&i4);
									fprintf(track_fp,"%02x %02x %02x %02x ",i1,i2,i3,i4);
									fprintf(track_fp,"%02x %02x \n",((group_assign_packet.neighbour_port[cnting]>>8) & 0xFF),(group_assign_packet.neighbour_port[cnting] & 0xFF));
								}
								fclose(track_fp);
							}
							break;
						}
						case 2:
						{
							printf("\nIn case 2 of the Tracker\n");
							//to connect to the client sending the group interest packet 
							UDP_connection_to_Client.sin_family = AF_INET;
							UDP_connection_to_Client.sin_port = htons(client_port_number);
							UDP_connection_to_Client.sin_addr.s_addr = INADDR_ANY;
							
							
							//Add it to the group and send the Group assign packet too
							int flag_check=0;
							group_assign_packet.msgtype=2;
							group_assign_packet.number_of_files=1;
							
							//Getting the index of the file
							for(i=0;i<total_files;i++)
							{
								if((strcmp(packet_group_show_interest.filename,file_data[i].file_name))==0)
								{
									flag_check=1;
									break;
								}
							}
							if(flag_check==0)
							{
								printf("\n\nThere is no seed node for this file\n\n");
								
							}
							else if(flag_check==1)
							{	flag_check=0;
								strcpy(group_assign_packet.filename,packet_group_show_interest.filename);
								group_assign_packet.number_of_neighbours=file_data[i].number_of_neighbours;
								for(j=0;j<group_assign_packet.number_of_neighbours;j++)
								{
									group_assign_packet.neighbour_id[j]=file_data[i].neighbour_node_number[j];
									group_assign_packet.neighbour_port[j]=file_data[i].neighbour_port[j];
								}
								sprintf(buf,"%d",group_assign_packet.msgtype);
								
								
								//sending message type which will be 2 as in the client, receiving GROUP_ASSIGN is case2
								if (sendto(Tracker_UDP_Socket, buf, sizeof(buf) , 0 , (struct sockaddr *) &UDP_connection_to_Client,addr_size)<=0)
								{
									//printf("Sending to Client failed from Tracker\n");
									exit(0);
								}
								
								
								//sending the Group Assign packet that has been prepared
								if (sendto(Tracker_UDP_Socket, (void*)&group_assign_packet, sizeof(group_assign_packet) , 0 , (struct sockaddr *) &UDP_connection_to_Client,addr_size)<=0)
								{
									//printf("Sending to Client failed from Tracker\n");
									exit(0);
								}
																//Print in hex
								int cnting=0;
								int chi=0;
								
								//Print in hex
								char hostn_udp[32];
								struct hostent *he_udp;
								unsigned char* file_ptr;
								char *store;
								int i1,i2,i3,i4;
								gethostname(hostn_udp ,sizeof(hostn_udp));
								he_udp=gethostbyname(hostn_udp);
								track_fp=fopen(track_file,"a");
								fprintf(track_fp,"00 %02x 00 %02x ",group_assign_packet.msgtype,group_assign_packet.number_of_files);
								file_ptr = (unsigned char*)group_assign_packet.filename;
								for(chi=0 ;chi<32;chi++){
									fprintf(track_fp,"%02x ",(unsigned int)file_ptr[chi]);
								}
								fprintf(track_fp,"00 %02x ",group_assign_packet.number_of_neighbours);
								for(cnting=0;cnting<group_assign_packet.number_of_neighbours;cnting++)
								{
									fprintf(track_fp,"00 %02x ",group_assign_packet.neighbour_id[cnting]);
									store=inet_ntoa(*(struct in_addr*)he_udp->h_addr);
									sscanf(store,"%d.%d.%d.%d",&i1,&i2,&i3,&i4);
									fprintf(track_fp,"%02x %02x %02x %02x ",i1,i2,i3,i4);
									fprintf(track_fp,"%02x %02x \n",((group_assign_packet.neighbour_port[cnting]>>8) & 0xFF),(group_assign_packet.neighbour_port[cnting] & 0xFF));
								}
								fclose(track_fp);
								
								
								//Check if the node exists in the neighbour list already
								/*int neigh_count;
								int check_flag=0,check_0flag=0;
									for(neigh_count=0;neigh_count<25;neigh_count++)
									{
										if(file_data[i].neighbour_node_number[neigh_count]==packet_group_show_interest.client_node_id)
										{
											check_flag=1;
										}
								  
									}
								//}
								if(check_flag==0) //&& check_0flag==0)
								{
									file_data[i].neighbour_port[group_assign_packet.number_of_neighbours]=client_port_number;
									file_data[i].neighbour_node_number[group_assign_packet.number_of_neighbours]=packet_group_show_interest.client_node_id;
									file_data[i].number_of_neighbours++;
									if(file_data[i].number_of_neighbours>=25)
									{
										printf("Program supports only 25 neighbours\n");
										exit(0);
									}
								}*/
								
							}
							break;
						}				
							
						case 3://Search whether the file exists or not and then add the information to the group accordingly
						{	
							printf("\nIn case 3 of tracker");
							int neighbour_count=0;
							int flag_check=0;
							if(total_files==0)
							{
								//add the details to the structure
								strcpy(file_data[total_files].file_name,packet_group_show_interest.filename);
								file_data[total_files].neighbour_node_number[file_data[total_files].number_of_neighbours]=packet_group_show_interest.client_node_id;
								//Check for neighbour IP and how to send it
								file_data[total_files].neighbour_port[file_data[total_files].number_of_neighbours]=client_port_number;
								file_data[total_files].number_of_neighbours++;
								total_files++;
							}
							else{
									for(i=0;i<total_files;i++)
									{
										if((strcmp(packet_group_show_interest.filename,file_data[i].file_name))==0)
										{
											flag_check=1;//indicates that the file exists and we do not need to create another group
											break;
										}
									}
									if(flag_check==0)
									{
										//Make new file and assign details to it									
										
										strcpy(file_data[total_files].file_name,packet_group_show_interest.filename);
										file_data[total_files].neighbour_node_number[file_data[total_files].number_of_neighbours]=packet_group_show_interest.client_node_id;
										
										//Check for neighbour IP and how to send it
										file_data[total_files].neighbour_port[file_data[total_files].number_of_neighbours]=client_port_number;
										file_data[total_files].number_of_neighbours++;
										if(file_data[total_files].number_of_neighbours>=25)
										{
											printf("Program supports only 25 neighbours\n");
											exit(0);
										}
										total_files++;
										if(total_files>=25)
										{
											printf("\nProgram cannot handle so many files.Only 25 files allowed!!\n");
											exit(0);
										}
									}
									else if(flag_check==1)
									{
										//add neighbours to that file
										file_data[i].neighbour_port[file_data[i].number_of_neighbours]=client_port_number;
										file_data[i].neighbour_node_number[file_data[i].number_of_neighbours]=packet_group_show_interest.client_node_id;
										file_data[i].number_of_neighbours++;
										if(file_data[i].number_of_neighbours>=25)
										{
											printf("Program supports only 25 neighbours\n");
											exit(0);
										}
									}
								}
							break;
						}
					}
            }
		}
	}
		
}

void* file_download(void *group_assign_packet1)
{
	
	
	
		 Group_Assign group_assign_packet = *(Group_Assign*)group_assign_packet1;	
		int i;	 
		int UDP_socket_client;
		clock_t dif;
		int seconds;
		int pckt_delay=client_details.packet_delay;
		pckt_delay=pckt_delay/1000;
		struct sockaddr_in UDP_connection_cli,UDP_connection_to_Track,UDP_c_to_c;
		/*Initialize size variable to be used later on*/
		int slen=sizeof(UDP_connection_cli);
		char buf[500];
		char message[500];
		if ( (UDP_socket_client=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		{
			printf("UDP socket connection for client failed\n");
			exit(0);
		}
		memset((char *) &UDP_connection_cli, 0, sizeof(UDP_connection_cli));
		/*UDP_connection_cli.sin_family = AF_INET;
		UDP_connection_cli.sin_port = htons(0);
		UDP_connection_cli.sin_addr.s_addr = INADDR_ANY;
		if((bind(UDP_socket_client, (struct sockaddr *) &UDP_connection_cli, sizeof(UDP_connection_cli)))<0)
		{
			printf("Bind failed");
			exit(0);
		}
		getsockname(UDP_socket_client,(struct sockaddr*)&UDP_connection_cli,&slen);*/
		memset((char *) &UDP_connection_to_Track, 0, sizeof(UDP_connection_to_Track));



						
						for(i=0;i<group_assign_packet.number_of_neighbours;i++)
						{
							if(group_assign_packet.neighbour_id[i]!=client_details.node_no)
							{
							//	printf("*******************************In for loop*****************%d*************Neighbour id:%d\n",i);
								UDP_c_to_c.sin_family = AF_INET;
								UDP_c_to_c.sin_port = htons(group_assign_packet.neighbour_port[i]);
								UDP_c_to_c.sin_addr.s_addr = INADDR_ANY;
								int msgtype=3;
								char buffer1[100];
								sprintf(buffer1,"%d",msgtype);
								
								/*sleep(pckt_delay);
								//Sending the message type which is now 3
								if (sendto(UDP_socket_client, buffer1, sizeof(buffer1) , 0 , (struct sockaddr *) &UDP_c_to_c, slen)==-1)
								{
									printf("Sending to Another Client failed\n");
									exit(0);
								}
								*/	
								char buffer21[100];
                                                                strcpy(buffer21,group_assign_packet.filename);
                                                                strcat(buffer1,"#");
                                                                strcat(buffer1,buffer21);
	
								//Packet dealy 
								sleep(pckt_delay);
								//Sending the message type and filename to the other client
								//strcat(buffer1,buffer21);	
								if (sendto(UDP_socket_client, buffer1, sizeof(buffer1) , 0 , (struct sockaddr *) &UDP_c_to_c, slen)==-1)
								{
									printf("Sending to Another Client failed--arerere\n");
									exit(0);
								}
								
								//printing to the file
								in_fp=fopen(file_name,"a");
								dif=clock()-before;
								seconds=dif/ CLOCKS_PER_SEC;
								fprintf(in_fp,"%d    TO %d    CLIENT_INFO_REQ    %s \n",seconds,group_assign_packet.neighbour_id[i],group_assign_packet.filename);
								fclose(in_fp);
							
								struct File_Exchange file_info1;
								memset(&file_info1,0,sizeof(file_info1));
						//		printf("$$$$$$$$$$$$$$String sent from the thread is   %s$$$$$$$$$$$$$$$$$$$$$$$$$",buffer1);	
								if((recvfrom(UDP_socket_client,(void*)&file_info1,sizeof(file_info1), 0,(struct sockaddr *) &UDP_c_to_c, &slen)) <= 0)
								{
									printf("Error receiving segments requests in client");
									exit(0);
								}
									

								
								//Getting the location of the file from the array
								int cnt;
								int flag_check=0;
								for(cnt=0;cnt<25;cnt++)
								{
									if((strcmp(track[cnt].filename,group_assign_packet.filename))==0)
									{
										flag_check=1;
										break;
									}
								}
								//Setting the value of the received segments in the client for array with all client seg info		
								printf("Location of the file in the structure is:%d\n\n",cnt);
								if(flag_check==1)
								{
									printf("\nFlag check is 1 now!!!\n");
									int seg;
									for(seg=0;seg<650;seg++)
									{
										if(file_info1.segments[seg]==1)
										{
											track[cnt].segments[group_assign_packet.neighbour_id[i]][seg]=1;
											//printf("Neighbour id:%d     Segment id:%d\n\n",group_assign_packet.neighbour_id[i],seg);
											
										}
									}
								}
								
								//Writing the information to the client file
								char sg_info[7000];
								char temp1[100];
								int z;
								memset(&sg_info,0,sizeof(sg_info));
								memset(&temp1,0,sizeof(temp1));
								for(z=0;z<650;z++)
								{
									if(file_info1.segments[z]==1)
									{
										strcat(sg_info," ");
										sprintf(temp1,"%d",z);
										strcat(sg_info,temp1);
									}
									
								}
								in_fp=fopen(file_name,"a");
								dif=clock()-before;
								seconds=dif/ CLOCKS_PER_SEC;
								fprintf(in_fp,"%d    FROM %d    CLIENT_INFO_REP    %s    %s\n",seconds,group_assign_packet.neighbour_id[i],group_assign_packet.filename,sg_info);
								fclose(in_fp);
								//printf("Segments received are:%s\n",sg_info);*/
							}	
						}

						//Calculating the maximum segments that any file has
						int k,j,cnt,count=0,max_segments=0;
						int flag_check=0;
						for(cnt=0;cnt<25;cnt++)
						{
							if((strcmp(track[cnt].filename,group_assign_packet.filename))==0)
							{
								break;
							}
						}
								
						for(k=0;k<25;k++)
						{		
							count=0;								
							for(j=0;j<650;j++)
							{
								if(track[cnt].segments[k][j]==1)
									count++;
							}
							if(count>max_segments)
								max_segments=count;
						}
						printf("Max segments are:%d\n",max_segments);		
						max_file_download=max_segments;
						//Communication will start to receive segemnts (IN case 4)
						int msgtype=4;
						char buffer2[100];
					//	memset(&buffer2,0,sizeof(buffer2));
					//	sprintf(buffer2,"%d",msgtype);
						int flag2;
						int segment_count=0;
						int index_value=0;
						int prev=0;
						int gen=0;
					//	int rand_counter=0;
						
						//Running the loop till max segments to get all the segments and complete the download file
						for(i=0;i<max_segments;i++)
						{
							memset(&buffer2,0,sizeof(buffer2));
							if(track[cnt].segments[client_details.node_no][i]==1)
								continue;
							int randomn[25];
							memset(&randomn,0,sizeof(randomn));
							int rand_counter=0;
							for(j=0;j<25;j++)
							{
								if(track[cnt].segments[j][i]==1)
									{
										/*int q;
										int flag_in_max=0;
										for(q=0;q<max_segments;q++)
										{
											if(track[cnt].segments[j][q]==0)
											{
												flag_in_max=1;
											}
										}
										if(flag_in_max==0)
										{*/	
											randomn[rand_counter]=j;
											printf("Neighbour with segment %d is %d\n\n",i,j);
											rand_counter++;
										//}//break;
									}
									
							}
							
							if(prev>=rand_counter)
								prev=0;
							gen=prev;
							while(1)
							{
								index_value=gen;
								++gen;
								break;
							}
							/*for(gen=prev;gen<rand_counter;gen++)
							{	
								index_value=gen;
								break;	
							}
							//int index_value;	*/
							prev=gen;
						//	index_value=myRandom(rand_counter);
							

							//printf("Counter :%d   nIndex  %d     Previous:%d    Gen:%d    \n\n",rand_counter,index_value,prev,gen);
							j=randomn[index_value];
							//printf("Now connecting to j %d\n\n",j);
							int n_count;
							for(n_count=0;n_count<group_assign_packet.number_of_neighbours;n_count++)
							{
								if(group_assign_packet.neighbour_id[n_count]==j)
								{
									flag2=1;
									break;
								}
							}
							printf("Location of neighbour: %d\n\n",n_count);
							memset(&UDP_c_to_c,0,sizeof(UDP_c_to_c));
							UDP_c_to_c.sin_family = AF_INET;
							UDP_c_to_c.sin_port = htons(group_assign_packet.neighbour_port[n_count]);
							UDP_c_to_c.sin_addr.s_addr = INADDR_ANY;		
						
							
							//Sending the message type to another client
							/*sleep(pckt_delay);//packet delay for each send
							if (sendto(UDP_socket_client, buffer2, sizeof(buffer2) , 0 , (struct sockaddr *) &UDP_c_to_c, slen)==-1)
							{
								printf("Sending to Another Client failed\n");
								exit(0);
							}
							*/
							strcpy(buffer2,"4#");
							//char send[100];
							//memset(&send,0,sizeof(send));
							strcat(buffer2,group_assign_packet.filename);
							strcat(buffer2,"$");
							char buffer1[10];
							memset(&buffer1,0,sizeof(buffer1));
							sprintf(buffer1,"%d",i);
							strcat(buffer2,buffer1);
							//strcat(buffer2,send);
						//	printf("\n\n\n\n***************************************************String sent is     %s****************\n",buffer2);	
							sleep(pckt_delay);//packet delay for each send
						
							if (sendto(UDP_socket_client, buffer2, sizeof(buffer2) , 0 , (struct sockaddr *) &UDP_c_to_c, slen)==-1)
							{
								printf("Sending to Another Client failed\n");
								exit(0);
							}
							
							//Printing the details to the file
							in_fp=fopen(file_name,"a");
							dif=clock()-before;
							seconds=dif/ CLOCKS_PER_SEC;
							fprintf(in_fp,"%d    TO %d    CLIENT_SEG_REQ    %s    %d\n",seconds,group_assign_packet.neighbour_id[n_count],group_assign_packet.filename,i);
							fclose(in_fp);
							char receive[32];
							memset(&receive,0,sizeof(receive));
							//printf("Receive buffer:%s\n",receive);
							if((recvfrom(UDP_socket_client,receive,32, 0,(struct sockaddr *) &UDP_c_to_c, &slen)) <= 0)
							{
								printf("Error receiving segments requests in client");
								exit(0);
							}
							
							//printf("Data received is:%s\n",receive);
							in_fp=fopen(file_name,"a");
							dif=clock()-before;
							seconds=dif/ CLOCKS_PER_SEC;
							fprintf(in_fp,"%d    FROM %d    CLIENT_SEG_REP    %s    %d\n",seconds,group_assign_packet.neighbour_id[n_count],group_assign_packet.filename,i);
							fclose(in_fp);
							
							//printf("String received is:%s",receive);
							char file_name_1[100];
							int que=0;
							strcpy(file_name_1,group_assign_packet.filename);
							for(que=0;que!='\0';que++)
							{
								if(file_name_1[que]=='/')
									file_name_1[que]='_';
							}
							char node_id[10];
							sprintf(node_id,"%d",client_details.node_no);
							strcat(node_id,"-");
							strcat(node_id,file_name_1);
							FILE *fp3=fopen(node_id,"a");
							int start=i*32;
							fseek(fp3,start,SEEK_SET);
							fwrite(receive,1,32,fp3);
							fclose(fp3);
							track[cnt].segments[client_details.node_no][i]=1;
							//checking whether all files have been received
							int flaging=0;
							for(j=0;j<max_segments;j++)
							{
								if(track[cnt].segments[client_details.node_no][j]==0)
								{
									//printf("Flag is now set to 1\n");
									flaging=1;
									file_download_complete=1;
								}
							}
							segment_count++;
							if(segment_count==8||flaging==0)
							{
								break;
							}
						}
						
						int flaging=0;
						for(j=0;j<max_segments;j++)
						{
							if(track[cnt].segments[client_details.node_no][j]==0)
							{
								flaging=1;
							}
						}
						
						

						
						if(flaging==0)
						{
							in_fp=fopen(file_name,"a");
							dif=clock()-before;
							seconds=dif/ CLOCKS_PER_SEC;
							fprintf(in_fp,"%d *****FILE DOWNLOAD COMPLETE**** %s\n",seconds,group_assign_packet.filename);
							fclose(in_fp);
						}
						if(segment_count==8 && flaging==1)
						{
							
							assign_flag=1;
						}


			//Request timeout condition. If the client doesn't find 8 segments
						if(segment_count!=8)
							sleep(client_details.req_timeout);
}









void NodeAssignment(int i,int pid,int port_tracker,int manager_port)
{	
	sprintf(file_name,"%d%s",i,".out");
	FILE *in_fp = fopen(file_name, "w+");
	fprintf(in_fp,"Type:Client\n");
	fprintf(in_fp,"Pid: %d\n",pid);
	int seconds	;
	int pckt_delay=client_details.packet_delay;
	pckt_delay=pckt_delay/1000;
	//Creating a TCP socket for clients
	struct   sockaddr_in Client_TCP,Manager_TCP;
	int addrlen,port_self,TCP_socket_client,recvMsgSize=100;
	int RCVBUFSIZE=1000;
	char Buffer[100];
	addrlen=sizeof(Client_TCP);  
	memset(&Client_TCP, 0, sizeof(Client_TCP));  
	Client_TCP.sin_family = AF_INET;  
	Client_TCP.sin_addr.s_addr=INADDR_ANY;
	Client_TCP.sin_port=htons(0);
	if((TCP_socket_client = socket(PF_INET, SOCK_STREAM, 0))<0)
	{
		printf("Client Socket creation failed\n");
		exit(0);
	}		
	if((bind(TCP_socket_client, (struct sockaddr *) &Client_TCP, sizeof(Client_TCP)))<0)
	{
		printf("Binding for client connection failed");
		exit(0);
	}
	getsockname(TCP_socket_client,(struct sockaddr*)&Client_TCP,&addrlen);  
	port_self=ntohs(Client_TCP.sin_port); 
	fprintf(in_fp,"Tracker Port number: %d\n",port_tracker);
	fprintf(in_fp,"Self Port number: %d\n",port_self);
	//fprintf(in_fp,"Timestamp(sec)   From/To   Message Type          Message Sprcific Data \n");
	fclose(in_fp);
	
	
	//Creating a TCP connection on Manager port to ask for config details
	memset(&Manager_TCP, 0, sizeof(Manager_TCP));     /* Zero out structure */
    Manager_TCP.sin_family      = AF_INET;             /* Internet address family */
    Manager_TCP.sin_addr.s_addr = INADDR_ANY;   /* Server IP address */
    Manager_TCP.sin_port        = htons(manager_port); /* Server port */
	if ((connect(TCP_socket_client, (struct sockaddr *) &Manager_TCP, sizeof(Manager_TCP))) < 0)
	{
		printf("Here");
		printf("Error in Connection to Server");
		exit(0);
	}
	sprintf(Buffer,"%d",i);//Sending the node number to the Manager
	if ((send(TCP_socket_client, Buffer, recvMsgSize, 0) )!= recvMsgSize)
	{
		printf("Send failed!");
		exit(0);
	}
	if ((recvMsgSize = recv(TCP_socket_client, (void*)&client_details, sizeof(client_details), 0)) <= 0){
			printf("Error receiving message!");
			exit(0);
		}
	else{
			printf("\n***********Config Info********\n");
			printf("Node number:%d\n",client_details.node_no);
			printf("Packet delay:%d\n",client_details.packet_delay);
			printf("Packet Drop Probability:%d\n",client_details.packet_drop_probability);
			printf("Stored File Name:%s\n",client_details.file_name_initial);
			printf("File Download Request:%s\n",client_details.file_name_requested);
			printf("Start Time:%d\n",client_details.Start_time);
			printf("Share:%d\n",client_details.share);
			printf("Request Timeout:%d\n",client_details.req_timeout);
		}
		
		//Establish a UDP connection to talk to the Tracker now!
		struct sockaddr_in UDP_connection_client,UDP_connection_to_Tracker,UDP_connection_to_client;
		int UDP_socket_client;
		/*Initialize size variable to be used later on*/
		int slen=sizeof(UDP_connection_client);
		char buf[500];
		char message[500];
		if ( (UDP_socket_client=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		{
			printf("UDP socket connection for client failed\n");
			exit(0);
		}
		memset((char *) &UDP_connection_client, 0, sizeof(UDP_connection_client));
		UDP_connection_client.sin_family = AF_INET;
		UDP_connection_client.sin_port = htons(0);
		UDP_connection_client.sin_addr.s_addr = INADDR_ANY;
		if((bind(UDP_socket_client, (struct sockaddr *) &UDP_connection_client, sizeof(UDP_connection_client)))<0)
		{
			printf("Bind failed");
			exit(0);
		}
		getsockname(UDP_socket_client,(struct sockaddr*)&UDP_connection_client,&slen);
		memset((char *) &UDP_connection_to_Tracker, 0, sizeof(UDP_connection_to_Tracker));
		
		
		//connecting to tracker to show interest in receiving file
		UDP_connection_to_Tracker.sin_family = AF_INET;
		UDP_connection_to_Tracker.sin_port = htons(port_tracker);
		UDP_connection_to_Tracker.sin_addr.s_addr = INADDR_ANY;
		struct Group_Show_interest packet_group_show_interest,packet_group_show_interest_init;
		Group_Assign group_assign_packet;
		struct File_Exchange file_info;
		
		memset(&group_assign_packet,0,sizeof(group_assign_packet));
		memset(&packet_group_show_interest_init,0,sizeof(packet_group_show_interest_init));
		memset(&file_info,0,sizeof(file_info));
		packet_group_show_interest_init.msgtype=1;
		packet_group_show_interest_init.client_node_id=client_details.node_no;
		packet_group_show_interest_init.number_of_files=1;
		
		//Checking which file to be sent as interest
		if((strcmp(client_details.file_name_initial,"null"))==0)
		{
			if((strcmp(client_details.file_name_requested,"null"))==0)
			{
				printf("Node does not have any data to send or receive");
			}
			else//file to be downloaded must exist in the node
			{
				int flag_check=0;
				strcpy(file_info.filename,client_details.file_name_requested);
				strcpy(packet_group_show_interest_init.filename,client_details.file_name_requested);
				if(client_details.share==1)
					packet_group_show_interest_init.type=2;
				if(client_details.share==0)
					packet_group_show_interest_init.type=1;
				for(i=0;i<25;i++)
				{
					if((strcmp(track[i].filename,client_details.file_name_requested)==0))
					{
						flag_check=1;
						break;
					}
				}
				if(flag_check==0)
				{
					if(file_tot_client<=25)
					{
						strcpy(track[file_tot_client].filename,client_details.file_name_requested);
						file_tot_client++;
					}
				}
			}
		}
		else//Seed node which has the file initially
		{
			printf("Seed node is:%d\n",client_details.node_no);
			int f_ind=0;
			int flag_check=0;
			for(i=0;i<25;i++)
			{
				if((strcmp(track[i].filename,client_details.file_name_initial)==0))
				{
					flag_check=1;
					break;
				}
			}
			if(flag_check==1)
			{
				f_ind=i;
			}
			else if(flag_check==0 && file_tot_client<=25)
			{
				strcpy(track[file_tot_client].filename,client_details.file_name_initial);
				f_ind=file_tot_client;
				file_tot_client++;
				printf("the total number of files is:%d\n",file_tot_client);
			}
		

			printf("I am at the location to check file size\n");	
			//Calculating the total file size to set the segments in the structure
			FILE *fp1=fopen(client_details.file_name_initial,"r+");
			fseek(fp1, 0L, SEEK_END);
			int number_of_segments;
			int size= ftell(fp1);
			if(size%32!=0){
				number_of_segments=(size/32)+1;
			}
			else
			{
				number_of_segments=(size/32);
			}
			for(i=0;i<number_of_segments;i++)
			{
				printf("Value of i is :%d\n",i);
				file_info.segments[i]=1;
				printf("Value of index of the file is:%d\n",f_ind);
				track[f_ind].segments[client_details.node_no][i]=1;
			}
			fclose(fp1);
			strcpy(file_info.filename,client_details.file_name_initial);	
			strcpy(packet_group_show_interest_init.filename,client_details.file_name_initial);
			packet_group_show_interest_init.type=3;
		}
		
		printf("The group show interest packet is now ready and can be transmitted!!\n");	
		printf("The start time for this node %d is %d\n",client_details.node_no,client_details.Start_time);	
		//Timer loop. Wait till sendind the group interest packet
		int msec;
		do {
				
				
				clock_t difference = clock() - before;
				msec = difference/ CLOCKS_PER_SEC;
			} while ( msec < client_details.Start_time );
		
	//	sleep(pckt_delay);//packet delay for each send
		//Sending the request to the tracker using UDP connection
		if (sendto(UDP_socket_client, (void*)&packet_group_show_interest_init, sizeof(packet_group_show_interest_init) , 0 , (struct sockaddr *) &UDP_connection_to_Tracker, slen)==-1)
		{
			printf("Sending to Tracker failed\n");
			exit(0);
		}
		
		
		
		
	//Communication with the tracker to get the Group Assign message
	 char* value[2];
	fd_set readfds;
    int numfd;
    struct timeval tv;
    tv.tv_sec = 10000;
    tv.tv_usec = 50000;
    FD_ZERO(&readfds);
    FD_SET(UDP_socket_client, &readfds);
    numfd = UDP_socket_client + 1;
	struct sockaddr_in UDP_client_to_client;
	int recv_len;
	//Code to listen for UDP requests from Clients and Tracker
	while(1)
	{
	
		 FD_SET(UDP_socket_client, &readfds);
		 tv.tv_sec = 100;
		 //tv.tv_usec = 50;
		printf("Client is waiting to receive requests\n");
		int recieve = select(numfd, &readfds, NULL, NULL, &tv);
		if (recieve == -1) 
		{
			perror("select"); // error occurred in select()
        } 
		else if (recieve == 0) 
		{
          printf("Timeout occurred!  No data after 100 seconds!!\n");
		  break;
        }
		else
		{
			if (FD_ISSET(UDP_socket_client, &readfds)) 
			{
                // FD_CLR(s, &readfds);
                memset(&packet_group_show_interest,0,sizeof(packet_group_show_interest));
				
				if((recvfrom(UDP_socket_client,buf,sizeof(buf), 0, (struct sockaddr *) &UDP_connection_to_client, &slen)) <= 0)
				{
					printf("Error receiving requests in client");
					exit(0);
				}

				
				value[0]=strtok(buf,"#");
				printf("%s\n\n",value[0]);
				int counter=0;
				while(1)
				{
     					value[1]=strtok(NULL,"#");
     					printf("%s\n\n",value[1]);
    					break;
				}
				
				char copy[500];
				
				
				if(atoi(value[0])!=2)
				{
					strcpy(copy,value[1]);	
				}

			//	strcpy(copy,value[1]);
				//strcpy(copy,value[1]);*/	
				//printf("\n\n\n###########################%s####################################%s\n\n\n",value[0],copy);
				//No need to write the message type in the file
				//printf("\n\n Message type has been received!! in %s\n\n",buf);
				int cases=atoi(value[0]);
				//printf("\n\n%d\n\n",cases);
				switch(cases)
				{
					case 2:
					{
						printf("I am now in case 2 of the client");
						//printf("Message type 2-->%d",client_details.node_no);
						if((recvfrom(UDP_socket_client, (void*)&group_assign_packet,sizeof(group_assign_packet), 0, (struct sockaddr *) &UDP_connection_to_client, &slen)) <= 0)
						{
								printf("Receiving in Tracker failed");
								exit(0);
						}
						char neighbour_info[1000];
						char temp[100];
						
						memset(&neighbour_info,0,sizeof(neighbour_info));
						memset(&temp,0,sizeof(temp));
						for(i=0;i<group_assign_packet.number_of_neighbours;i++)
						{
							strcat(neighbour_info,",");
							sprintf(temp,"%d",group_assign_packet.neighbour_id[i]);
							strcat(neighbour_info,temp);
						}
						//printf("\n\nComplete neighbour string is:%s",neighbour_info);
						//printing to the file
						in_fp=fopen(file_name,"a");
						clock_t dif=clock()-before;
						seconds=dif/ CLOCKS_PER_SEC;
						fprintf(in_fp,"%d    FROM T    GROUP_ASSIGN    %s \n",seconds,neighbour_info);
						fclose(in_fp);
							
						
						//Printing the contents of the packet received from the tracker
						printf("\n#########Data received from the Tracker########\n");
						printf("I am node number:%d\n",client_details.node_no);
						printf("Message Type:%d\n",group_assign_packet.msgtype);
						printf("Number of files:%d\n",group_assign_packet.number_of_files);
						printf("File in the group:%s\n",group_assign_packet.filename);
						printf("Number of Neighbours:%d\n",group_assign_packet.number_of_neighbours);
						for(i=0;i<group_assign_packet.number_of_neighbours;i++)
						{
							printf("\n*************In neighbour froup*********\n");
							printf("Neighbour id:%d\n",group_assign_packet.neighbour_id[i]);
							printf("IP of neighbour is:%s\n",group_assign_packet.neighbour_ip);
							printf("Port of neighbour is:%d\n",group_assign_packet.neighbour_port[i]);
									
						}
						
						
						assign_flag=0;
						//Starting a new thread that will communicate with the client
						pthread_t file_transfer;
						assign_flag=0;
						if(pthread_create(&file_transfer, NULL, file_download,(void*) &group_assign_packet)) {

							fprintf(stderr, "Error creating thread\n");
							//return 1;

						}
						  pthread_join (file_transfer, NULL);
						  
						  
						  
						  if(assign_flag==1)
						  {
							  //printf("In the if case for assign flag\n");
							  sleep(pckt_delay);//packet delay for each send
							  if (sendto(UDP_socket_client, (void*)&packet_group_show_interest_init, sizeof(packet_group_show_interest_init) , 0 , (struct sockaddr *) &UDP_connection_to_Tracker, slen)==-1)
								{
									printf("Sending to Tracker failed\n");
									exit(0);
								}
						  }
						  
						  if(max_file_download!=0)
							{
								int cnt=0;
								for(cnt=0;cnt<25;cnt++)
								{
									if((strcmp(track[cnt].filename,group_assign_packet.filename))==0)
									{
										break;
									}
								}
								int chng;
								int flag_chng=1;
								for(chng=0;chng<max_file_download;chng++)
								{
									if(track[cnt].segments[client_details.node_no][chng]==0)
										flag_chng=0;
								}
								if(flag_chng==1)
								{
									if(packet_group_show_interest_init.type==2)
									{
											packet_group_show_interest_init.type=3;
											if (sendto(UDP_socket_client, (void*)&packet_group_show_interest_init, sizeof(packet_group_show_interest_init) , 0 , (struct sockaddr *) &UDP_connection_to_Tracker, slen)==-1)
											{
												printf("Sending to Tracker failed\n");
												exit(0);
											}
									}	
								}
								 
							}
						  
						  
						break;
					}
					
					case 3:
					{
						printf("In case 3 of the client\n");
						struct sockaddr_in UDP_client_to_neighbour;
					//	memset(&UDP_client_to_client,0,sizeof(UDP_client_to_client));
						memset(&buf,0,sizeof(buf));
						


						/*if((recvfrom(UDP_socket_client,buf,sizeof(buf),0, (struct sockaddr *) &UDP_client_to_client, &slen)) <= 0)
						{
							printf("Error receiving requests in client");
							exit(0);
						}*/
						
						//printf("Value in value 1 is:%s\n",value[1]);	
						strcpy(buf,copy);
					//	strncpy(buf, value[1], sizeof(buf));
					//	buf[sizeof(buf) - 1] = '\0';
						//printf("String in Case 3 is:%s",buf);
						int flag_check=0;
						for(i=0;i<25;i++)
						{
							if((strcmp(track[i].filename,buf)==0))
							{
								flag_check=1;
								break;
							}
						}
						if(flag_check==1)
						{
							//File exists at row number i
							struct File_Exchange file_send;
							memset(&file_send,0,sizeof(file_send));
							strcpy(file_send.filename,track[i].filename);
							int cnt;
							for(cnt=0;cnt<650;cnt++)
							{
								file_send.segments[cnt]=track[i].segments[client_details.node_no][cnt];
							}
							sleep(pckt_delay);//packet delay for each send
							if (sendto(UDP_socket_client, (void*)&file_send, sizeof(file_send) , 0 , (struct sockaddr *) &UDP_connection_to_client, slen)==-1)
							{
								printf("\n\nSegments not sent to the client--FAILED\n\n");
								exit(0);
							}
								
						}
						break;
					}
					
					
					case 4:
					{
						printf("\n\nMessage type is now 4 for client-->%d \n\n",client_details.node_no);
						memset(&UDP_client_to_client,0,sizeof(UDP_client_to_client));
						memset(&buf,0,sizeof(buf));
						/*if((recvfrom(UDP_socket_client,buf,sizeof(buf),0, (struct sockaddr *) &UDP_client_to_client, &slen)) <= 0)
						{
							printf("Error receiving requests in client");
							exit(0);
						}*/
						
						strcpy(buf,copy);
						//printf("\n\nString in Case 4 is:%s\n\n",buf);						
						char* token[2];
						token[0]=strtok(buf,"$");
						printf("%s\n\n",token[0]);
						while(1)
						{
							token[1]=strtok(NULL,"$");
							printf("%s\n\n",token[1]);
							break;
						}
						//printf("Filename is :%s and segment number is:%s\n",token[0],token[1]);
						FILE *fp2=fopen(token[0],"r");
						int start=(int)(atoi(token[1]))*32;
						fseek(fp2,start,SEEK_SET);
						char file_data[32];
						memset(&file_data,0,sizeof(file_data));
						fread(file_data,32,1,fp2);
						printf("Data read is :  %s\n",file_data);
						fclose(fp2);
						sleep(pckt_delay);//packet delay for each send
						if(sendto(UDP_socket_client, file_data, sizeof(file_data), 0 , (struct sockaddr *) &UDP_connection_to_client, slen)==-1)
						{
							printf("\n\nSegments not sent to the client--FAILED\n\n");
							exit(0);
						}
						break;
					}
					
					
					
				}
			}
		}
	}
}



//Node Creation by forking the parent process
void NodeCreation(int port_tracker,int manager_port)
{
	//printf("In node creation now!\n");
	pid_t pid;
	int pid1=0;
	int i=0;
	for(i=0;i<number_nodes;++i)
	{
		pid=fork();
		if(pid==0)
		{
			pid=getpid();
			pid1=pid;
			NodeAssignment(i,pid1,port_tracker,manager_port);
			return;
		}
	}
}


int main ()
{
	clock_t before = clock();
	/* master file descriptor list */
	fd_set master;
	/* temp file descriptor list for select() */
	fd_set read_fds;
	/* maximum file descriptor number */
	int fdmax;
	/* listening socket descriptor */
	int listener;
	/* newly accept()ed socket descriptor */
	int newfd;
	int nbytes;
	/* for setsockopt() SO_REUSEADDR, below */
	int yes = 1;
	/* clear the master and temp sets */
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	pid_t pid=0;
    int i=0,port_tracker,j;
	struct sockaddr_in Manager_TCP;
	struct sockaddr_in echoClntAddr;
	int addrlen,port_manager,Manager_TCP_Socket,clntSock,recvMsgSize,new; 
	unsigned int clntLen;	
	addrlen=sizeof(Manager_TCP);  
	
	//Creating TCP connection for Manager
	memset(&Manager_TCP, 0, sizeof(Manager_TCP));
	Manager_TCP.sin_family = AF_INET;  
	Manager_TCP.sin_addr.s_addr=INADDR_ANY;
	Manager_TCP.sin_port=htons(0);
	Manager_TCP_Socket = socket(PF_INET, SOCK_STREAM, 0);  
	bind(Manager_TCP_Socket, (struct sockaddr *) &Manager_TCP, sizeof(Manager_TCP));  
	getsockname(Manager_TCP_Socket,(struct sockaddr*)&Manager_TCP,&addrlen);  
	port_manager=ntohs(Manager_TCP.sin_port); 
	//printf("Manager Port:%d",port_manager);
	clntLen = sizeof(echoClntAddr);
	int RCVBUFSIZE=20;
	char Buffer[1000];
	
	//Storing the manager.conf file in a structure
	struct Node_Info node_details[25];
	memset(&node_details,0,sizeof(node_details));
	
	FILE *fp=fopen("manager.conf","r+");
	int counter=0,nodeid,packetdelay,packetdropporb,timeout=0,starttime,shareinfo;
	char filenameinit[100],filenamereq[100];
	 if (fp == NULL) {
        printf("Error: file pointer is null.");
        exit(1);
    }
    char line [1000];
	//Reading the file line by line and storing in Structure
    while(fgets(line,sizeof line,fp)!= NULL)  {	
	if(line[0]=='#' || line[0]==' ')
	{
		if(counter==6)
		{
			break;
		}
		else
		continue;
	}
	else
	{
		counter++;
		if(counter==1)
		{
			sscanf(line,"%d",&number_nodes);
			//printf("Number of nodes is :%d\n",number_nodes);
			for(i=0;i<number_nodes;i++)
			{
				strcpy(node_details[i].file_name_initial,"null");
				strcpy(node_details[i].file_name_requested,"null");
			}
			continue;
		}
		else if(counter==2)
		{	
			sscanf(line,"%d",&timeout);
			//printf("Request time out is:%d\n",timeout);
		}
		else if(counter==3)
		{
			sscanf(line,"%d %d %d",&nodeid,&packetdelay,&packetdropporb);
			if(nodeid==-1)
			{
				printf("No clients connected");
				exit(0);
			}
			else
			{
				while(nodeid!=-1)
				{	
					node_details[nodeid].node_no=nodeid;
					node_details[nodeid].packet_delay=packetdelay;
					node_details[nodeid].packet_drop_probability=packetdropporb;
					node_details[nodeid].req_timeout=timeout;
					fgets(line,sizeof line,fp);
					sscanf(line,"%d %d %d",&nodeid,&packetdelay,&packetdropporb);
					if(nodeid==-1)
						break;
				}
			}
		}
		else if (counter==4)
		{
			sscanf(line,"%d %s",&nodeid,filenameinit);
			if(nodeid==-1)
			{
				printf("No clients has initial file");
				exit(0);
			}
			else
			{
				while(nodeid!=-1)
				{	//printf("Checkout!!!\n");
					strcpy(node_details[nodeid].file_name_initial,filenameinit);
					fgets(line,sizeof line,fp);
					sscanf(line,"%d %s",&nodeid,filenameinit);
					if(nodeid==-1)
						break;
				}
			}	
		}
		else if(counter==5)
		{	counter++;
			sscanf(line,"%d %s %d %d",&nodeid,filenamereq,&starttime,&shareinfo);
			if(nodeid==-1)
			{
				printf("No clients has file to be downloaded");
				exit(0);
			}
			else
			{
				while(nodeid!=-1)
				{	
					strcpy(node_details[nodeid].file_name_requested,filenamereq);
					node_details[nodeid].Start_time=starttime;
					node_details[nodeid].share=shareinfo;
					fgets(line,sizeof line,fp);
					sscanf(line,"%d %s %d %d",&nodeid,filenamereq,&starttime,&shareinfo);
					if(nodeid==-1)
					{
						break;
					}
					
				}
			}	
		}
	}
 }
	fclose(fp);
	
	//Printing the structure
	for(i=0;i<number_nodes;i++)
	{
		node_details[i].node_no=i;
		//printf("Node number:%d\n",node_details[i].node_no);
	}
	//Manager starts listening for incoming requests from Tracker and Clients
	if (listen(Manager_TCP_Socket, MAXPENDING) < 0)
	{
        printf("Listen failed!");
		exit(0);
	}
	pid=fork();
	if(pid<0)
	{
		printf("Fork failed");
	}//Creates the tracker process!
	else if(pid==0)
	{
		pid=getpid();
		int pid1=pid;
		//printf("Creating Tracker now!!");
		TrackerCreation(pid1,port_manager);
	}		 
	else if(pid>0)	
	{	
		 
		//printf("In the parent part\n");
		if ((clntSock = accept(Manager_TCP_Socket, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0)
		{
			//printf("Error in accepting\n");
			exit(0);
		}
		int port_tracker;//=ntohs(echoClntAddr.sin_port);
		//printf("I am in parent now!!\n");
		if ((recvMsgSize = recv(clntSock, Buffer, RCVBUFSIZE, 0)) < 0){
			//printf("Error receiving message!");
			exit(0);
		}
		//printf("Buffer is : %s\n",Buffer);
		sscanf(Buffer, "%d", &port_tracker);
		NodeCreation(port_tracker,port_manager);
		close(clntSock);
		
		//Creating new connections for multiple client requests
		char buf[10240];
		// add the listener to the master set 
		FD_SET(Manager_TCP_Socket, &master);
		// keep track of the biggest file descriptor 
		fdmax = Manager_TCP_Socket; //so far, it's this one
		static int count_connection=0;
		int node_index;
		for(;;)
		{
			if(count_connection==number_nodes)
			{
				
				printf("Terminating the Manager\n");
				break;
				close(newfd);
				//close(Manager_TCP_Socket);
			}
			/* copy it */
			read_fds = master;
 
			if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
			{
				perror("Server-select() error lol!");
				exit(1);
			}
			//printf("Server-select() is OK...\n");
			/*run through the existing connections looking for data to be read*/
			for(i = 0; i <= fdmax; i++)
			{
				if(FD_ISSET(i, &read_fds))
				{ /* we got one... */
					if(i == Manager_TCP_Socket)
					{
						/* handle new connections */
						addrlen = sizeof(echoClntAddr);
						if((newfd = accept(Manager_TCP_Socket, (struct sockaddr *)&echoClntAddr, &addrlen)) == -1)
						{
							perror("Server-accept() error lol!");
						}
						else
						{
							
							//printf("Server-accept() is OK...\n");
 
							FD_SET(newfd, &master); /* add to master set */
							if(newfd > fdmax)
							{ /* keep track of the maximum */
								fdmax = newfd;
							}
							//printf("%s: New connection from %s on socket %d\n", argv[0], inet_ntoa(echoClntAddr.sin_addr), newf);
						}
					}
					else
					{
						/* handle data from a client */
						memset(buf,0,sizeof(buf));
						fflush(stdout);
						if((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0)
						{
							/* got error or connection closed by client */
							if(nbytes == 0)
							printf("Nothing received\n");
							else
							perror("recv() error lol!");
							/* close it... */
							close(i);
							/* remove from master set */
							FD_CLR(i, &master);
						}
						else
						{	/* we got some data from a client*/
							count_connection++;
							//printf("Client says %s to server\n",buf);
							node_index=atoi(buf);
							//printf("Node index: %d\n",node_index);
							if(send(i, (void *) &node_details[node_index], sizeof(node_details[node_index]), 0) == -1)
											perror("send() error lol!");
						}
					}
				}
			}
			
		}
	}
  return 0;
 }
