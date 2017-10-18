/* To get the format of the UDP data we read the document which
 * is not freely available:
 *
 *  http://www.ar-tracking.com/support/
 *
 * We had to register; enter email and password and wait for
 * email from them to get account.
 */

// This code will only work for a very particular ART controller
// configuration.  That's all we needed.
//
// To read art UDP data try running this netcat command:
//
//   nc -ulp 5000
//
// With that you can see the UDP data is in a simple ascii format.

#define PORT  (5000)
#define BIND_ADDRESS "192.168.0.10"

// Marks the start of valid head position data
#define HEAD_START "6d 1 [0 1.000]["
#define HEAD_END   "]["
#define HEAD_MIN_LEN   10
#define HEAD_MAX_LEN   300

// Marks the start of valid wand data
#define WAND_START_ALL     "6df2 1 1 [0 1.000 6 2]["
// We will receive this with just the buttons and joystick
// and the positions and rotation will be zeros.
#define WAND_START_BUTTONS "6df2 1 1 [0 -1.000 6 2]["
#define WAND_END   "]\n"
#define WAND_MIN_LEN   10
#define WAND_MAX_LEN   300

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
class ReadArtTracker : public ThreadedNode
{
public:
    ReadArtTracker();
    virtual ~ReadArtTracker();

    // Factory method to create an instance of ReadArtTracker.
    static Node *create();
  
    // Factory method to return the type of ReadArtTracker.
    virtual NodeType *type() const;
  
protected:
    // Gets called when the ReadArtTracker is enabled.
    virtual void initialize();
  
    // Gets called when the ReadArtTracker is disabled.
    virtual void shutdown();
  
    // thread method to send/receive data.
    virtual int processData ();
  
private:

    int fd; // socket file descriptor

    OutSlot<Matrix4f> *viewPointOutSlot_;
  
    static NodeType type_;
};


NodeType ReadArtTracker::type_(
    "readARTTrackerUDP" /*typeName must be the same as plugin filename */,
    &create,
    "head and wand Tracker to InstantIO" /*shortDescription*/,
    /*longDescription*/
    "head and wand Tracker to InstantIO",
    "lance"/*author*/,
    0/*fields*/,
    0/sizeof(Field));

ReadArtTracker::ReadArtTracker():
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

ReadArtTracker::~ReadArtTracker()
{
    SPEW();
}

Node *ReadArtTracker::create()
{
    SPEW();

    ReadArtTracker *ht;

    ht = new ReadArtTracker;

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

NodeType *ReadArtTracker::type() const
{
    SPEW();  
    return &type_;
}

void ReadArtTracker::initialize()
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

void ReadArtTracker::shutdown()
{
    // handle state and namespace updates
    Node::shutdown();

    SPEW();  

    if(fd >= 0) close(fd);

    assert(viewPointOutSlot_);

    if(viewPointOutSlot_)
    {
        removeOutSlot("head", viewPointOutSlot_);
        delete viewPointOutSlot_;
        viewPointOutSlot_ = 0;
    }
}

static void die(void)
{
    printf("Forcing exit the hard way\n");
    kill(getpid(), SIGKILL);
}


// Thread method which gets automatically started as soon as a slot is
// connected
int ReadArtTracker::processData()
{
    SPEW(); 

    setState(NODE_RUNNING);

    assert(viewPointOutSlot_);

    // It would appear that someone decided that
    // model units are in meters so a model with
    // some length equal to 1 corresponds to 1 meter.
    //
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

        // Find the head



        Matrix4f tracker_mat;

        Vec3f tracker_pos;
        tracker_pos[0] = 0; tracker_pos[1] = 0; tracker_pos[2] = 0;
        Rotation tracker_rot;
        tracker_rot[0] = 0;  tracker_rot[1] = 0;
        tracker_rot[2] = 0;  tracker_rot[3] = 0;
#if 1
std::cerr << "head (" << 
    tracker_pos[0] << " " << tracker_pos[1] << " " << tracker_pos[2] <<
    ") (" <<
    tracker_rot[0] << " " << tracker_rot[1] << " " <<
    tracker_rot[2] << " " << tracker_rot[3] <<
    ")" << std::endl;

#endif

        tracker_mat.setTransform( tracker_pos, tracker_rot);

        viewPointOutSlot_->push(tracker_mat);
    }

    // Thread finished
    setState(NODE_SLEEPING);

    SPEW();  

    return 0;
}

} // namespace InstantIO
