//
//  client.c
//  Description: Sends request to server to subscribe to multicaster
//
//  Created by Patrick Ayres on 3/13/16.
//  Copyright © 2016 Patrick Ayres. All rights reserved.
//

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<time.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close

#define SERVER_PORT 5432
#define MAX_LINE 256
#define MAXNAME 100

// define type numbers
#define REG 121
#define CONF 221
#define ADD_REG 131
#define RESP 231
#define LEAVE 321
#define CHAT_MESSAGE 421

pthread_t thread;
/* structure of the packet */
struct packet{
    short type;
    int sockId;
    int groupNum;
    char data[MAXNAME];
};
struct packet packet_reg; // packet for sending
struct packet packet_rec; // packet for receiving
struct packet packet_multicast; // packet for getting multicast

void waitFor (unsigned int secs) {
    long retTime = time(0) + secs;     // Get finishing time.
    while (time(0) < retTime);    // Loop until it arrives.
}

// join handler thread
void *recv_thread(int *client_sockId)
{
    printf("client_sockId: %d\n", *client_sockId);
    while(1){
        
        //Receive a reply from the server
        if(recv(*client_sockId,&packet_multicast,sizeof(packet_multicast),0) < 0)
        {
            printf("\n Could not receive message from mulicaster \n");
            exit(1);
        }
        
        // check if type is 421
        if(htons(packet_multicast.type) == CHAT_MESSAGE){
            
            // print received message
            printf("RECEIVED:\n");
            printf("\tClient #%d says: %s\n", htons(packet_multicast.sockId), packet_multicast.data);
            //printf("\tTYPE: %d\n", htons(packet_multicast.type));
            //printf("\tSOCK_ID: %d\n", packet_multicast.sockId);
            //printf("\tChat Message: %s\n", packet_multicast.data);
        }

        packet_multicast.type = 0;
    }
    
    return NULL;
}

int main(int argc, char* argv[])
{
    // declare variables
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    char serverReply[MAX_LINE];
    int s, leave_s;
    int len;
    char *webAddress;
    int groupNumber;
    int sockId;
    
    // throw error if not enough arguments
    if(argc >= 2){
        host = argv[1];
    }
    else{
        fprintf(stderr, "usage:newclient server\n");
        exit(1);
    }
    
    // grab the web address which needs to be converted
    webAddress = argv[2];
    
    // group number of chat room to join
    groupNumber = atoi(argv[3]);
    
    /* translate host name into peer's IP address */
    hp = gethostbyname(host);
    if(!hp){
        fprintf(stderr, "unkown host: %s\n", host);
        exit(1);
    }
    
    /* active open */
    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("tcpclient: socket");
        exit(1);
    }
    
    /* build address data structure */
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    //sin.sin_port = htons(SERVER_PORT);
    sin.sin_port = htons(atoi(argv[2]));
    
    if(connect(s,(struct sockaddr *)&sin, sizeof(sin)) < 0){
        perror("tcpclient: connect");
        close(s);
        exit(1);
    }
    
    // wait for one second
    waitFor(1);
    
    /* Constructing the registration packet at client */
    packet_reg.type = htons(REG);
    strcpy(packet_reg.data, hp->h_name);
    packet_reg.groupNum = htons(groupNumber);
    
    /* Send the registration packet to the server */
    if(send(s,&packet_reg,sizeof(packet_reg),0) <0)
    {
        printf("\n Send failed\n");
        exit(1);
    }
    
    // print sent message
    printf("SENT:\n");
    printf("\tTYPE: %d\n", ntohs(packet_reg.type));
    printf("\tGROUP_NUM: %d\n", ntohs(packet_reg.groupNum));
    printf("\tDATA: %s\n", packet_reg.data);
    
    // wait for one second
    waitFor(1);
    
    /* Constructing the registration packet at client */
    packet_reg.type = htons(REG);
    strcpy(packet_reg.data, hp->h_name);
    packet_reg.groupNum = htons(groupNumber);
    
    /* Send the registration packet to the server */
    if(send(s,&packet_reg,sizeof(packet_reg),0) <0)
    {
        printf("\n Send failed\n");
        exit(1);
    }
    
    // print sent message
    printf("SENT:\n");
    printf("\tTYPE: %d\n", ntohs(packet_reg.type));
    printf("\tGROUP_NUM: %d\n", ntohs(packet_reg.groupNum));
    printf("\tDATA: %s\n", packet_reg.data);
    
    // wait for one second
    waitFor(1);
    
    /* Constructing the registration packet at client */
    packet_reg.type = htons(REG);
    strcpy(packet_reg.data, hp->h_name);
    packet_reg.groupNum = htons(groupNumber);
    
    /* Send the registration packet to the server */
    if(send(s,&packet_reg,sizeof(packet_reg),0) <0)
    {
        printf("\n Send failed\n");
        exit(1);
    }
    
    // print sent message
    printf("SENT:\n");
    printf("\tTYPE: %d\n", ntohs(packet_reg.type));
    printf("\tGROUP_NUM: %d\n", ntohs(packet_reg.groupNum));
    printf("\tDATA: %s\n", packet_reg.data);
    
    // listen for response
    while(1){
        
        //Receive a reply from the server
        if(recv(s,&packet_rec,sizeof(packet_rec),0) < 0)
        {
            printf("\n Could not receive confirmation packet \n");
            exit(1);
        }
        
        // check if type is 221
        if(htons(packet_rec.type) == CONF){
            
            // print received message
            printf("RECEIVED:\n");
            printf("\tTYPE: %d\n", htons(packet_rec.type));
            printf("\tSOCK_ID: %d\n", packet_rec.sockId);
            printf("\tGROUP_NUM: %d\n", packet_rec.groupNum);
            printf("\tDATA: %s\n", packet_rec.data);
            
            // save sockId
            sockId = packet_rec.sockId;
            pthread_create(&thread, NULL, recv_thread, &s);
            //pthread_join(threads[0],&exit_value);
            
            // holds text for chat message
            char chatMessage [100];

            // loop getting input from user
            while(1){
                
                // get input from the user for the chat room
                printf("Enter a message: ");
                scanf ( "%s", chatMessage);
                strcpy(packet_reg.data, chatMessage);
                packet_reg.type = htons(CHAT_MESSAGE);
                
                /* Send the chat message packet to the server */
                if(send(s,&packet_reg,sizeof(packet_reg),0) <0)
                {
                    printf("\n Send failed\n");
                    exit(1);
                }
                sleep(1);
                // print sent message
                printf("SENT:\n");
                printf("\tTYPE: %d\n", ntohs(packet_reg.type));
                printf("\tDATA: %s\n", packet_reg.data);
            }
        }
//        // else if the message is from the multicaster
//        else if(htons(packet_rec.type) == RESP){
//            
//            // print received message
//            printf("MULTICAST MESSAGE RECEIVED:\n");
//            printf("\tTYPE: %d\n", htons(packet_rec.type));
//            printf("\tDATA: %s\n", packet_rec.data);
//            
//        }
    }

}