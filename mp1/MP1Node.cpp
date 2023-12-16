/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstaps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	//int id = *(int*)(&memberNode->addr.addr);
	//int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}
void MP1Node::cleanupNodeState() {
    memberNode->inGroup = false;
    
    memberNode->nnb = 0;
    memberNode->heartbeat = 0;
    memberNode->pingCounter = TFAIL;
    memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);
}
/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));// sender address
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
      /*
    * Your code goes here
    */
    // Node is down...
    memberNode->inited = false;
    
    // Cleanup node state
    cleanupNodeState();
    
    return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
        cout<<"Node "<<memberNode->addr.getAddress()<< " marked as failed"<<endl;

    	return;
    }
    
    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
        cout<<"Node "<<memberNode->addr.getAddress()<< " marked as outgroup"<<endl;
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {

    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
        
    	recvCallBack((void *)memberNode, (char *)ptr, size);
        
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */

    MessageHdr* receivedMsg = (MessageHdr*) malloc(size * sizeof(char));
    memcpy(receivedMsg, data, sizeof(MessageHdr));
    cout<<"receivedMsg "<<receivedMsg->msgType<< endl;
 

    if(receivedMsg->msgType == JOINREQ) {
        
        // Read message data
        int id;
        short port;
        long heartbeat;
        memcpy(&id, data + sizeof(MessageHdr), sizeof(int));
        memcpy(&port, data + sizeof(MessageHdr) + sizeof(int), sizeof(short));
        memcpy(&heartbeat, data + sizeof(MessageHdr) + sizeof(int) + sizeof(short), sizeof(long));
        
        cout<<"JoinRequest id : "<<id<<" port : "<<port<<" heartbeat : "<< heartbeat<<endl;

        addToMemberList(id ,port,heartbeat, memberNode->timeOutCounter);
        Address addr = getNodeAddress(id,port);

        //check if I(the cordinator) exists
        if(!existsInMemberlist(memberNode->addr))
            addToMemberList((int*)&memberNode->addr.addr[0]     // id
                        , (short*)&memberNode->addr.addr[4]     // port
                        , memberNode->heartbeat                 // heartbeat
                        , memberNode->timeOutCounter);          // time


        // data wil contain : msgType , table(addresses,heartbeats,time)
        size_t replysize = getTableSize();
        MessageHdr *replyMsg = (MessageHdr*) malloc(replysize * sizeof(char));
        
        replyMsg->msgType= JOINREP ;
        getTable(replyMsg);// size + data

        //send replymessage
        cout<<"send replymessage from:"<<memberNode->addr.getAddress()<<" to:"<<addr.getAddress()<<" size :"<<replysize<<endl<<endl;
        emulNet->ENsend(&memberNode->addr,&addr, (char *)replyMsg, replysize);

        free(replyMsg);
       
    }
    else if (receivedMsg->msgType == JOINREP)
    {   
        cout<<"received JOINREP"<<endl;

        memberNode->inGroup=true;
        
        createTable(receivedMsg);
    }
    else if (receivedMsg->msgType == GOSSIPHEARTBEAT)       //na2es 
    {
        cout<<"received GOSSIPHEARTBEAT"<<endl;
        updateMembersTable(receivedMsg);

    }
     free(receivedMsg);
    return true;
} 


/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
    if(memberNode->pingCounter == 0){
        //incrementing heartbeat
        memberNode->heartbeat++;

        //creating heartbeat message with the memberlist
        size_t gossipSize = getTableSize();
        MessageHdr *gossipMsg = (MessageHdr*) malloc(gossipSize * sizeof(char)); 
        gossipMsg->msgType=GOSSIPHEARTBEAT;
        getTable(gossipMsg);

        sendGossipStyleHeartbeats(gossipMsg);
        
        memberNode->pingCounter=TFAIL;
        
        
    }
    else{
        memberNode->pingCounter--;
    }
    //na2es checking for failures
    //loop iterator
    //  if timeout
    //    
    //      remove 
    Address toAddr ;
    std::vector<MemberListEntry>::iterator i ;
    
    //  loop
    for ( i = memberNode->memberList.begin()       
        ; i < memberNode->memberList.end()
        ; ++i )
    {

        toAddr = getNodeAddress( i->id ,i->port);

        if (memberNode->timeOutCounter - i->timestamp >  TREMOVE
            && ! (toAddr == memberNode->addr) )
        {
            memberNode->memberList.erase(i);
            #ifdef DEBUGLOG
                log->logNodeRemove(&memberNode->addr, &toAddr);
            #endif
                cout<<""<<memberNode->addr.getAddress();
        }

    }

    // Increment overall counter
    memberNode->timeOutCounter++;
    
    return;
}
/**
 * FUNCTION NAME: AddToMemberList
 *
 * DESCRIPTION: it will add the requested node to the coordinator's list
 */
void MP1Node::addToMemberList(int id, short port, long heartbeat,long timestamp){

    MemberListEntry *newMember = new MemberListEntry(id, port,heartbeat,timestamp);
    memberNode->memberList.insert(memberNode->memberList.end(),*newMember);

    cout<<"new item "<<newMember->id<<endl;

    delete newMember;
    return;   
}
/**
 * FUNCTION NAME: sendGossipStyleHeartbeat
 *
 * DESCRIPTION: randomly picks n number of receiver nodes and send them full list table
 */
void MP1Node::sendGossipStyleHeartbeats(MessageHdr *msg){
    
    int n = 4;    //numberOfreceivers
    size_t msgsize = getTableSize();
    std::vector<MemberListEntry>::iterator itr =memberNode->memberList.begin() ;

    //to update rand generation
    
    int listSize = memberNode->memberList.size();
    
    if (listSize > n )
        advance( itr , rand()%listSize ) ;

    while(itr>memberNode->memberList.end())
        advance( itr , rand()%listSize ) ;

    //loop to pick and send 4 gossip messages
    for (int i = 0; i < n && i < listSize ; ++i)
    {
        /* jump iterator to random node */
        

        Address toAddr = getNodeAddress( itr->id ,itr->port);
        
        // Not me not null
        if((! (toAddr == memberNode->addr)) && !isNullAddress(&toAddr) ){
            emulNet->ENsend(&memberNode->addr,&toAddr,(char *) msg, msgsize);
            cout<<"send gossip from:"<<memberNode->addr.getAddress()<<" to:"<<toAddr.getAddress()<<" size :"<<msgsize<<endl;
        
        } 
        else{
            n++;
        }

        if ( listSize > n 
            && itr == (memberNode->memberList.end()-1) 
            && i < (n-1) )
                itr = memberNode->memberList.begin() ;
        
        else 
            itr++;
        

    }
    // cout<<endl;
    return;
}
/**
 * FUNCTION NAME: updateMembersTable
 **a3mel ehhh recieved heartbeat u
        *fokha
        *if exist
            if newer
        *       update 
        *else
        *    addtolist
        *
        *
 */
void MP1Node::updateMembersTable(MessageHdr *gossipMsg){
    int id;
    short port;
    long recvHeartbeat;

    int numberOfItems;
    memcpy(&numberOfItems, gossipMsg + sizeof(MessageHdr), sizeof(int));
        
    // Deserialize member list entries
    int offset = sizeof(int);
        
    for(int i = 0; i < numberOfItems; i++) {           
        
        getNextNodeFromMessage(&id,&port,&recvHeartbeat,gossipMsg,&offset);
        
        if (!existsInMemberlist( getNodeAddress( id,port) ) ){
            addToMemberList(id,port,recvHeartbeat,memberNode->timeOutCounter);
        }
        else{
            if (memberNode->timeOutCounter - memberNode->myPos->timestamp >  TREMOVE/2){
                //do nothing
            }
            else if(memberNode->myPos->heartbeat < recvHeartbeat){
                memberNode->myPos->heartbeat = heartbeat;
                memberNode->myPos->timestamp = memberNode->timeOutCounter;
            }
        }
    }
    return;
}
/**
 * FUNCTION NAME: getTable
 *
 * DESCRIPTION: Function returns the table in messageHdr
 */
void MP1Node::getTable(MessageHdr *table){
    
    int numberOfMembers =memberNode->memberList.size();
    memcpy((char *)(table + 1), &numberOfMembers, sizeof(int));

    int offset = sizeof(int);
    
    std::vector<MemberListEntry>::iterator i ;
    //cout<<"begin : "<<memberNode->memberList.begin()<<" end : "<<memberNode->memberList.end()<<endl;
    //  loop
    for ( i = memberNode->memberList.begin()       
        ; i < memberNode->memberList.end()
        ; ++i )
    {

        memcpy( (char *)(table+1)+offset,   &i->id,      sizeof(int));
        offset +=sizeof(int);
       
        memcpy( (char *)(table+1)+offset,   &i->port,    sizeof(short));
        offset +=sizeof(short); 
        
        //  Checks if its my heartbeat to update it before it's sent
        if( getNodeAddress(i->id,i->port) == memberNode->addr){
           
            memcpy((char *)(table+1)+offset,    &memberNode->heartbeat,  sizeof(long));
            offset +=sizeof(long);
            cout<<"its me !!!"<<endl;
            //My position in the table
           
           
        }
        else{
            memcpy( (char *)(table+1)+offset,    &i->heartbeat   ,sizeof(long));
            offset +=sizeof(long);
            
        }
    }
   
    return;
}
void MP1Node::createTable(MessageHdr *msg){
    
        // Read message data
    int numberOfItems;
    memcpy(&numberOfItems, msg + sizeof(MessageHdr), sizeof(int));
    
    // Deserialize member list entries
    int offset = sizeof(int);
        
    for(int i = 0; i < numberOfItems; i++) {           
        int id;
        short port;
        long heartbeat;
        getNextNodeFromMessage(&id,&port,&heartbeat,msg,&offset);    
         cout<<"JoinRequest port : "<<id<<":"<<port<<" heartbeat : "<< heartbeat 

         <<endl;

        // Create and insert new entry
        addToMemberList(id, port, heartbeat, memberNode->timeOutCounter);
    }

}
bool MP1Node::existsInMemberlist(Address addr){
    std::vector<MemberListEntry>::iterator i ;
    Address toAddr;
    for ( i = memberNode->memberList.begin()       
        ; i < memberNode->memberList.end()
        ; ++i )
    {
        toAddr= getNodeAddress(i->id,i->port);
        if( addr == toAddr ){
            memberNode->myPos=i;
            return true;
        }
    }
    return false;
}
void MP1Node::getNextNodeFromMessage(int *id,short *port,long *heartbeat,MessageHdr *msg,int *offset){
    memcpy(&id,         msg + sizeof(MessageHdr) + *offset, sizeof(int));
    offset += sizeof(int);
    
    memcpy(&port,       msg + sizeof(MessageHdr) + *offset, sizeof(short));
    offset += sizeof(short);
        
    memcpy(&heartbeat,  msg + sizeof(MessageHdr) + *offset, sizeof(long));
    offset += sizeof(long);
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}
/**
 * FUNCTION NAME: getTableSize
 *
 * DESCRIPTION: Function returns the size of table 
 */
size_t MP1Node::getTableSize(){
   
    size_t memberListEntrySize = sizeof(int) + sizeof(short) + sizeof(long) ;//+ sizeof(long);
    return (sizeof(MessageHdr) + sizeof(int) + (memberNode->memberList.size() * memberListEntrySize));
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}
/*
 * FUNCTION NAME: getNodeAddress
 *
 * DESCRIPTION: merges ip and port number
 */
Address MP1Node::getNodeAddress(int id, short port ){
    Address addr;

    memset(&addr, 0, sizeof(Address));
     memcpy((int*)&addr.addr[0], &id,  sizeof(int));
     memcpy((short*)&addr.addr[4],&port, sizeof(short));
   
    return addr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
    //memberNode->memberList.insert()
}


/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
