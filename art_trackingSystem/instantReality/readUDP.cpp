// To test run:
//
// make &&   InstantPlayer readUDP.x3d
//

#define PORT 5000
#define BIND_ADDRESS "0.0.0.0" // Any Address == "0.0.0.0"

//#define NDEBUG
#include <assert.h>

#include <math.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include <InstantIO/ThreadedNode.h>
#include <InstantIO/NodeType.h>
#include <InstantIO/OutSlot.h>
#include <InstantIO/MFTypes.h>
#include <InstantIO/Vec3.h>
#include <InstantIO/Matrix4.h>


#define SPEW()   std::cerr << __BASE_FILE__ << ":" << __LINE__\
  << " " <<  __func__ << "()" << std::endl



namespace InstantIO
{

template <class T> class OutSlot;
class ReadUDP : public ThreadedNode
{
public:
    ReadUDP();
    virtual ~ReadUDP();

    // Factory method to create an instance of ReadUDP.
    static Node *create();
  
    // Factory method to return the type of ReadUDP.
    virtual NodeType *type() const;
  
protected:
    // Gets called when the ReadUDP is enabled.
    virtual void initialize();
  
    // Gets called when the ReadUDP is disabled.
    virtual void shutdown();
  
    // thread method to send/receive data.
    virtual int processData ();
  
private:

    int fd; // socket file descriptor

    OutSlot<Matrix4f> *viewPointOutSlot_;
  
    static NodeType type_;
};


NodeType ReadUDP::type_(
    "readUDP" /*typeName must be the same as plugin filename */,
    &create,
    "read UDP data from a bound PORT" /*shortDescription*/,
    /*longDescription*/
    "Test Plug-in to read UDP data from a bound PORT",
    "lance"/*author*/,
    0/*fields*/,
    0/sizeof(Field));

ReadUDP::ReadUDP():
    ThreadedNode()
{
    SPEW();
    // Add external route
    addExternalRoute("*", "{NamespaceLabel}/{SlotLabel}");

    errno = 0;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0)
    {
        char errStr[256];
        printf("socket(AF_INET, SOCK_DGRAM, 0) failed: errno=%d: %s\n",
            errno, strerror_r(errno, errStr, 256));
        return;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_aton(BIND_ADDRESS, &addr.sin_addr);

    if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
    {
        char errStr[256];
        printf("bind() to address \""BIND_ADDRESS":%d\" failed: errno=%d: %s\n",
            PORT, errno, strerror_r(errno, errStr, 256));
        close(fd);
        fd = -1;
        return;
    }

    SPEW();
}

ReadUDP::~ReadUDP()
{
    SPEW();

    if(fd >= 0)
    {
        close(fd);
    }


    SPEW();
}

Node *ReadUDP::create()
{
    SPEW();

    ReadUDP *ht;

    ht = new ReadUDP;

    if(ht->fd == -1)
    {
        delete ht;
        // TODO: this will make stupid sax segfault.
        // You'd think that they at least check the return value
        // from a factory function.
        //
        // We do not know how else to make sax fail and exit.
        // Calling exit() does not work.
        return 0;
    }

    return ht;
}

NodeType *ReadUDP::type() const
{
    SPEW();  
    return &type_;
}

void ReadUDP::initialize()
{
    SPEW();

    // handle state and namespace updates
    Node::initialize();

    viewPointOutSlot_ = new OutSlot<Matrix4f>(
        "changing frustum view points", Matrix4f());
    assert(viewPointOutSlot_);
    viewPointOutSlot_->addListener(*this);
    addOutSlot("head", viewPointOutSlot_);
}

void ReadUDP::shutdown()
{
    assert(viewPointOutSlot_);

    if(viewPointOutSlot_)
    {
        removeOutSlot("head", viewPointOutSlot_);
        delete viewPointOutSlot_;
        viewPointOutSlot_ = 0;
    }

    // handle state and namespace updates
    Node::shutdown();

    SPEW();  

}

static void die(void)
{
    printf("Forcing exit the hard way\n");
    kill(getpid(), SIGKILL);
}


// Thread method which gets automatically started as soon as a slot is
// connected
int ReadUDP::processData()
{
    SPEW(); 

    setState(NODE_RUNNING);

    assert(viewPointOutSlot_);

    while(waitThread(0))
    {
        const size_t BUFLEN = 1024;
        char buf[BUFLEN+1];
        errno = 0;

        // This should block until there is data to read.
        ssize_t ret = recv(fd, buf, BUFLEN, 0);
        if(ret <= 0)
        {
            char errStr[256];
            printf("recv() failed: errno=%d: %s\n",
                errno, strerror_r(errno, errStr, 256));
            close(fd);
            fd = -1;
            die();
        }

        buf[ret] = '\0';

        printf("read(%zd bytes) = %s\n", ret, buf);
        Matrix4f tracker_mat;


        viewPointOutSlot_->push(tracker_mat);
    }

    // Thread finished
    setState(NODE_SLEEPING);

    SPEW();  

    return 0;
}

} // namespace InstantIO
