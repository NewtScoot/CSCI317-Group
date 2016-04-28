//
//  server.c
//  Description: Allows clients to subscribe to multicaster stream.
//
//  Created by Patrick Ayres on 3/13/16.
//  Copyright © 2016 Patrick Ayres. All rights reserved.
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>


#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write

#define SERVER_PORT 5432
#define MAX_LINE 256
#define MAX_PENDING 5
#define MAXNAME 100
#define TABLE_SIZE 20

// define type numbers
#define REG 121
#define CONF 221
#define ADD_REG 131
#define RESP 231
#define LEAVE 321
#define CHAT_MESSAGE 421

/* structure of Registration Table */
struct registrationTable{
    int port;
    char name[MAXNAME];
    int req_no;
};

// will hold tables values
struct registrationTable table[TABLE_SIZE];

/* structure of the packet */
struct packet{
    short type;
    int sockId;
    int groupNum;
    char data[MAXNAME];
};

// global table stuct
typedef struct  {
    int sockid;
    int reqno;
    int groupNum;
}global_table;

// multicast buffer
typedef struct {
    int isRead;
    struct packet packet;
}circular_buffer;


circular_buffer buffer[10];
int bufferPointer = 0;
global_table record[TABLE_SIZE];
struct packet packet_rec;
struct packet packet_res;
struct packet packet_multicast;
struct packet packet_chat[1000];
pthread_t threads[1000];   // need 1000 threads in the server
int *exit_value;
int socketId = 0;
int foundInRecordTable = 0;
int foundInRecordTableByJoinHandler = 0;
int recordTablePointer = 0;
int tableHasValues = 0;

// global mutex variable
pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;

// waiting timer
void waitFor (unsigned int secs) {
    long retTime = time(0) + secs;     // Get finishing time.
    while (time(0) < retTime);    // Loop until it arrives.
}

// multicaster thread
void *multicaster()
{
    
    while(1){
        // multicast every 5 seconds
        //waitFor(5);
//
////        // mutex lock the global table
        pthread_mutex_lock(&my_mutex);
////        
////        // check first record is global table to see
////        //    if there is at least one client listed in the table
////        if(tableHasValues == 1 ){
////            
////            // at least 1 client found
////            // only run once
////            // now only this once read the text file into memory
////            if(packetContructed == 0){
////                
////                /* Constructing the multicast packet to send to clients */
////                packet_multicast.type = htons(RESP);
////
////                // manually copy the message over since strcpy fails
////                for(int i = 0; i < 100; i++){
////                    packet_multicast.data[i] = text[i];
////                }
////                printf("\n");
////                //strcpy(packet_multicast.data,text);  // this line fails so manually copied
////                
////                packetContructed++;
////            }
//        
//            // send message to everyone in the table with request number > 3
            // see if client is already in the global record table
            for(int i = 0; i < TABLE_SIZE; i++){
                
                // if the record has at least 3 registration requests
                if(record[i].reqno >= 3){
                    
                    // attempt to send the packet
                    if(send(record[i].sockid,&packet_multicast,sizeof(packet_multicast),0) < 0)
                    {
                        printf("\n Send failed\n");
                        exit(1);
                    }
                    
                    // print sent message
                    printf("\nSENT MULTICAST MESSAGE TO SOCKET_ID: %d\n", record[i].sockid);
                    printf("\tTYPE: %d\n", ntohs(packet_multicast.type));
                    printf("\tDATA: %s\n", packet_multicast.data);
                }
            }
        //}
    
        // mutex unlock the global table
        pthread_mutex_unlock(&my_mutex);
    
    }

    
    /* the function must return something - NULL will do */
    return NULL;
    
}

// join handler thread
void *join_handler(global_table *rec)
{
    int newsock;
    //struct packet packet_reg;
    newsock = rec->sockid;

    // wait for additional requests
    while(1){
        packet_chat[newsock].type = 0;
        if(recv(newsock,&packet_chat[newsock],sizeof(packet_chat[newsock]),0) < 0){
            printf("\n Cound not receive second registration packet\n");
            //exit(1);
        }
        
        // log incoming message
        if(ntohs(packet_chat[newsock].type) == REG && newsock == rec->sockid){
            
            // increment request number and log
            rec->reqno++;
            printf("INCOMING... RECORD FOUND: REQ_NO: %d  SOCK_ID: %d  GROUP_NUM: %d\n", rec->reqno, rec->sockid, rec->groupNum);
            
            
            
            // if the record has been found 3 times, exit
            if(rec->reqno == 3){
                
                // mutex lock the global table
                pthread_mutex_lock(&my_mutex);
                
                //enter data in the record table
                record[recordTablePointer].reqno = rec->reqno;
                record[recordTablePointer].sockid = rec->sockid;
                record[recordTablePointer].groupNum = rec->groupNum;
                recordTablePointer++;
                tableHasValues = 1;
                
                // mutex unlock the global table
                pthread_mutex_unlock(&my_mutex);
                
                // SEND ACK RESPONSE
                char serverResponse[] = "Client Now Registered for Multicast Messages\n";
            
                /* Constructing the registration packet at client */
                packet_res.type = htons(CONF);
                strcpy(packet_res.data,serverResponse);
                packet_res.sockId = rec->sockid;
                packet_res.groupNum = rec->groupNum;
        
                /* Send the registration packet to the client */
                if(send(newsock,&packet_res,sizeof(packet_res),0) <0)
                {
                    printf("\n Send failed\n");
                    exit(1);
                }
                        
                // print sent message
                printf("SENT:\n");
                printf("\tTYPE: %d\n", ntohs(packet_res.type));
                printf("\tSOCK_ID: %d\n", packet_res.sockId);
                printf("\tREGISTERED for GROUP: %d\n", packet_res.groupNum);
                printf("\tDATA: %s\n", packet_res.data);
                //break;

            }
            
        }
        else if(ntohs(packet_chat[newsock].type) == CHAT_MESSAGE && newsock == rec->sockid){
            
            // add message to buffer
            printf("Incoming chat message from SOCK_ID: %d for GROUP_NUM: %d\n", newsock, ntohs(packet_chat[newsock].groupNum));
        }
    }
    
    printf("Join_Handler Closing...\n");
    
    // Leave the thread
    pthread_exit(NULL);
}

int main(int argc, char* argv[])
{
    // declare variables
    short incomingPort;
    char *incomingAddress;
    struct sockaddr_in sin;
    struct sockaddr_in clientAddr;
    int s, new_s;
    int len;
    global_table client_info, leave_info;
    
    // create multicaster thread
    pthread_create(&threads[1], NULL, multicaster, NULL);

    
    /* setup passive open */
    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("tcpserver: socket");
        exit(1);
    }
    
    /* build address data structure */
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);
    
    // bind and listen for incoming
    if(bind(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
        perror("tcpclient: bind");
        exit(1);
    }
    
    printf("SERVER RUNNING on PORT %d...\n",ntohs(sin.sin_port));
    listen(s, MAX_PENDING);
    

    while(1){
        /* wait for connection, then receive and print text */
        if((new_s = accept(s, (struct sockaddr *)&clientAddr, &len)) < 0){
            perror("tcpserver: accept");
            exit(1);
        }
        if(recv(new_s,&packet_rec, sizeof(packet_rec), 0) < 0) {
            printf("\n Could not receive first registration packet \n");
            exit(1);
        }
        
        // log incoming message
        if(ntohs(packet_rec.type) == REG){
            
            
            // set found to false
            foundInRecordTable = 0;
            
            // see if client is already in the global record table
            for(int i = 0; i < TABLE_SIZE; i++){
                
                // check if socket ID is already there
                if(record[i].sockid == new_s){
                    foundInRecordTable = 1;
                }
            }
            
            // if the record was not found, record it and create join handler thread
            if(foundInRecordTable == 0) {
                
                // print recieved new message
                printf("\nRECEIVED NEW CLIENT:\n");
                printf("\tTYPE: %d\n", ntohs(packet_rec.type));
                printf("\tGROUP_NUM: %d\n", ntohs(packet_rec.groupNum));
                printf("\tDATA: %s\n", packet_rec.data);
                
                // create the client info
                client_info.reqno = 1;
                client_info.sockid = new_s;
                client_info.groupNum = ntohs(packet_rec.groupNum);
                
                printf("INCOMING... RECORD NOT FOUND: REQ_NO: %d  SOCK_ID: %d\n", client_info.reqno, client_info.sockid);
                
                
                pthread_create(&threads[new_s], NULL, join_handler, &client_info);
                //pthread_join(threads[0],&exit_value);
                
            }
        }
    }
    
}

