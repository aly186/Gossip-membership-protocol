/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define TREMOVE 20

#define	TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
    GOSSIPHEARTBEAT, //------------
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	void cleanupNodeState();
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();						//mine
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);	// mine
	void nodeLoopOps();									// mine
	void addToMemberList(int id, short port, long heartbeat, long timestamp); //new one
	void sendGossipStyleHeartbeats(MessageHdr *msg);
	void updateMembersTable(MessageHdr *gossipMsg);
	void getTable(MessageHdr *table);        //new one
	void createTable(MessageHdr *msg);
	bool existsInMemberlist(Address addr);
	void getNextNodeFromMessage(int *id,short *port,long *heartbeat,MessageHdr *msg,int *offset);
	
	int isNullAddress(Address *addr);
	size_t getTableSize();
	Address getJoinAddress();
	Address getNodeAddress(int id, short port);//mine
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
