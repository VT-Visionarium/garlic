// Prerequisite: Get your ART tracker writing data to your computer
// on a port that you choose.

// Disclaimer: this code is only setup to parse a particular ART tracking
// data configuration, that is one 6DOF head and one fly stick 2.  There
// is a more generic ART tracking IOSensor code built into instant
// reality, but we could not get it to work in two days of trying.  We
// know that we can read UDP sockets with C++ code and it beats working
// with a fucking black box with no reasonable documentation.  We have to
// write custom code for our CAVE anyway, so what just a little more
// code to get is started.

// Test this by running:
//
//  sax --num-aspects 4 readARTTrackerUDP.x3d 


/* To get the format of the UDP data we read the document which
 * is not freely available:
 *
 *  http://www.ar-tracking.com/support/
 *
 *  The file was named: DTrack2_User-Manual.v2.13.pdf
 *
 * We had to register; enter email and password and wait for
 * email from them to get account.
 */

// This code will only work for a very particular ART controller
// configuration.  That's all we needed.
//
// To read art UDP data try running this netcat command (from a bash
// shell):
//
//   nc -ulp 5000
//
//
// NetCat rocks.
//
// With that you can see that the UDP data is in a simple ascii format.
// This format is explained in Appendix B of the above mentioned manual.

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


#define WITH_POS_ROT

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
#ifdef WITH_POS_ROT
    OutSlot<Vec3f> *posOutSlot_;
    OutSlot<Rotation> *rotOutSlot_;
#endif
  
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
    if(fd >= 0) close(fd);
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

    viewPointOutSlot_ = new OutSlot<Matrix4f>("head", Matrix4f());
    assert(viewPointOutSlot_);
    viewPointOutSlot_->addListener(*this);
    addOutSlot("head", viewPointOutSlot_);

#ifdef WITH_POS_ROT
    posOutSlot_ = new OutSlot<Vec3f>("headpos", Vec3f());
    assert(posOutSlot_);
    posOutSlot_->addListener(*this);
    addOutSlot("headpos", posOutSlot_);

    rotOutSlot_ = new OutSlot<Rotation>("headrot", Rotation());
    assert(rotOutSlot_);
    rotOutSlot_->addListener(*this);
    addOutSlot("headrot", rotOutSlot_);
#endif
}

void ReadArtTracker::shutdown()
{
    // handle state and namespace updates
    Node::shutdown();

    SPEW();  

    assert(viewPointOutSlot_);

    if(viewPointOutSlot_)
    {
        removeOutSlot("head", viewPointOutSlot_);
        delete viewPointOutSlot_;
        viewPointOutSlot_ = 0;
    }

#ifdef WITH_POS_ROT
    assert(posOutSlot_);

    if(posOutSlot_)
    {
        removeOutSlot("headpos", posOutSlot_);
        delete posOutSlot_;
        posOutSlot_ = 0;
    }

    assert(rotOutSlot_);

    if(rotOutSlot_)
    {
        removeOutSlot("headrot", rotOutSlot_);
        delete rotOutSlot_;
        rotOutSlot_ = 0;
    }
#endif

}


// Thread method which gets automatically started as soon as a slot is
// connected
int ReadArtTracker::processData()
{
    SPEW();

#define MOVE
#ifdef MOVE
    float direction = 1.0F;
    const float x_max = 1.0F;
    const float x_min = -1.0F;
    float x = x_min;

    const float rot_max = M_PI * 2.0F;
    const float rot_min = 0.0F;
    float rot_val = rot_min;
#endif


    setState(NODE_RUNNING);

    assert(viewPointOutSlot_);
    assert(posOutSlot_);
    assert(rotOutSlot_);

    // It would appear that someone decided that
    // model units are in meters so a model with
    // some length equal to 1 corresponds to 1 meter.
    //
    // waitThread() seems to be the hook that lets instant reality know
    // when we are starting/ending a cycle and we can continue running.
    // By it's name it's clear that they are catering to stupid
    // programmers that put sleeps in their code, and don't know that the
    // OS has other blocking calls, besides sleep, that handle system
    // resources very efficiently. 
    //
    while(waitThread(0))
    {
        const size_t BUFLEN = 1024;
        char buf[BUFLEN+1];
        errno = 0;

        // This should block until there is data to read.  Which is a very
        // efficient to get the data as soon as possible; but for all we
        // know the writer of instant reality fucked this up, and made this
        // block other threads too.
        ssize_t ret = recv(fd, buf, BUFLEN, 0);
        if(ret <= 0)
        {
            char errStr[256];
            printf("recv() failed: errno=%d: %s\n",
                errno, strerror_r(errno, errStr, 256));
            close(fd);
            fd = -1;
        }

        buf[ret] = '\0';

        printf("read(%zd bytes) = %s\n", ret, buf);

#if 0
        char *s;
        size_t len = strlen(
        // Set s to the start of the rotation part
        for(s = buf;
#endif

        Matrix4f mat;
        int i;
        for(i=0; i<16; ++i)
            mat[i] = 0.0F;

        // Set the diagonal to 1
        for(i=0; i<13; i+=5)
            mat[i] = 1.0F;

        mat[i] = 1.0F;

#ifdef MOVE
        if(x > x_max)
            direction = -1.0F;
        else if(x < x_min)
            direction = 1.0F;
        x += direction * 0.02;

        rot_val += 0.03;
        if(rot_val > rot_max)
            rot_val = rot_min;


        mat[3] = x;
#endif


#if 0
        // Get the rotation part of the 4x4 matrix
        for(i=0; i<9; ++i)
#endif


        Vec3f pos;
        pos[0] = 0; pos[1] = 0; pos[2] = 0;
        Rotation rot;
        rot[0] = 0;  rot[1] = rot_val;
        rot[2] = 0;  rot[3] = rot_val;

        // Normalize this rotation??
        float mag = 0;
        for(i=0; i<4; ++i)
            mag += rot[i]*rot[i];
        mag = sqrtf(mag);
        for(i=0; i<4; ++i)
            rot[i] /= mag;

#if 1
    std::cout << "------- head " << rot_val << "-------" << std::endl;
    int j;
    for(j=0;j<4;++j)
    {
        for(i=0; i<4; ++i)
            std::cout << mat[i+4*j] << "  ";
        std::cout << std::endl;
    }
    std::cout << std::endl;



#endif

        //mat.setTransform(pos, rot);

        viewPointOutSlot_->push(mat);
        posOutSlot_->push(pos);
        rotOutSlot_->push(rot);
    }

    // Thread finished
    setState(NODE_SLEEPING);

    SPEW();  

    return 0;
}

} // namespace InstantIO
