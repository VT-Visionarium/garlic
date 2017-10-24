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
//  sax --num-aspects 4 testHead.x3d
//
//  sax --num-aspects 4 testWand.x3d
//
//  sax --num-aspects 4 moveTeapot.x3d 



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

// Marks the start of valid wand data  "6df2 1 1 [0 1.000 6 2]["
//    or if just buttons and joystick  "6df2 1 1 [0 -1.000 6 2]["

#define WAND_CHECKCHR      '1'
#define WAND_START_ANY     "6df2 1 1 [0 " // the next char is '-' or '1'
#define WAND_CHECKSTR      "1.000 6 2]["
/// We will receive this with just the buttons and joystick
// and the positions and rotation will be zeros.
#define WAND_END   "]\n"
#define WAND_MIN_LEN   30

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

    template <class T>
    OutSlot<T> *getSlot(const char *desc, const char *name);
    
    template <class T>
    void removeSlot(T *slot, const char *name);

    void sendHead(const char *buf, size_t len);
    void sendWand(const char *buf, size_t len);

    const size_t head_start_LEN;
    const size_t wand_start_any_LEN;
    const size_t wand_checkstr_LEN;


    // A constant calibration which we multiple by the
    // tracker head viewpoint matrix.
    Matrix4f headCal; // Calibration matrix
    Matrix4f wandCal; // Calibration matrix


    int fd; // socket file descriptor

    OutSlot<Matrix4f> *head_matrix;
 
    OutSlot<Matrix4f>* wand_matrix;
    OutSlot<Rotation>* wand_rotation;
    OutSlot<Vec3f>* wand_position;
    OutSlot<float>* joystick_x_axis;
    OutSlot<float>* joystick_y_axis;
    OutSlot<bool>* button_1;
    OutSlot<bool>* button_2;
    OutSlot<bool>* button_3;
    OutSlot<bool>* button_4;
    OutSlot<bool>* button_5;
    OutSlot<bool>* button_6;
    OutSlot<bool>* button_7;
    OutSlot<bool>* button_8;
 
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

ReadArtTracker::ReadArtTracker(): head_start_LEN(strlen(HEAD_START)),
    wand_start_any_LEN(strlen(WAND_START_ANY)),
    wand_checkstr_LEN(strlen(WAND_CHECKSTR))
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
        printf("bind() to address \""
                BIND_ADDRESS":%d\" failed: errno=%d: %s\n",
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

template <class T>
OutSlot<T> *ReadArtTracker::getSlot(const char *desc, const char *name)
{
    OutSlot<T> *slot;
    slot = new OutSlot<T>(desc, T());
    assert(slot);
    slot->addListener(*this);
    addOutSlot(name, slot);
    return slot;
}

template <class T>
void ReadArtTracker::removeSlot(T *slot, const char *name)
{
    assert(slot);
    if(slot)
    {
        removeOutSlot(name, slot);
        delete slot;
    }
}

void ReadArtTracker::initialize()
{
    SPEW();  
    
    // handle state and namespace updates
    Node::initialize();

    head_matrix = getSlot<Matrix4f>("matrix of head", "head");
    wand_matrix = getSlot<Matrix4f>("matrix of wand", "wandmatrix");
    
    wand_rotation = getSlot<Rotation>("Rotation of wand", "wandrotation");
    wand_position = getSlot<Vec3f>( "Position of wand", "wandposition");
    
    joystick_x_axis = getSlot<float>("Joystick x axis", "joystick_x_axis");
    joystick_y_axis = getSlot<float>("Joystick y axis", "joystick_y_axis");

    button_1 = getSlot<bool>("Button 1", "button_1");
    button_2 = getSlot<bool>("Button 2", "button_2");
    button_3 = getSlot<bool>("Button 3", "button_3");
    button_4 = getSlot<bool>("Button 4", "button_4");

    button_5 = getSlot<bool>("Button 5", "button_5");
    button_6 = getSlot<bool>("Button 6", "button_6");
    button_7 = getSlot<bool>("Button 7", "button_7");
    button_8 = getSlot<bool>("Button 8", "button_8");
}

void ReadArtTracker::shutdown()
{
     // handle state and namespace updates
    Node::shutdown();

    SPEW();

    removeSlot(head_matrix, "head");
    removeSlot(wand_matrix, "wandmatrix");

    removeSlot(wand_rotation, "wandrotation");
    removeSlot(wand_position, "wandposition");
        removeSlot(button_1, "button_1");


    removeSlot(joystick_x_axis, "joystick_x_axis");
    removeSlot(joystick_y_axis, "joystick_y_axis");

    removeSlot(button_1, "button_1");
    removeSlot(button_2, "button_2");
    removeSlot(button_3, "button_3");
    removeSlot(button_4, "button_4");

    removeSlot(button_5, "button_5");
    removeSlot(button_6, "button_6");
    removeSlot(button_7, "button_7");
    removeSlot(button_8, "button_8");
}



// These variable name are as the ART tracker defines them.  We must
// transform them into the Instant Reality defined coordinates.  The
// ART tracker coordinates and the Instant Reality coordinates systems
// are different.  We do not change to Instant Reality coordinates at
// the tracker controller so that we do not confuse other rendering
// software, and we didn't have the time to figure out how.
//
// We define the rotation 3x3 part of the 4x4 matrix as row x column
// indexes:
//
//        |  00  01  02  |
//        |              |
//    R = |  10  11  12  |
//        |              |
//        |  20  21  22  |
//
//
// and the 4x4 transform matrix has elements like as row x column
// indexes:
//  
//        |                  |       |                   |
//        |  00  01  02  03  |       |  00  01  02  X_t  |
//        |                  |       |                   | 
//        |  10  11  12  13  |       |  10  11  12  Y_t  |
//  M  =  |                  |   =   |                   |
//        |  20  21  22  24  |       |  20  21  22  Z_t  |
//        |                  |       |                   |
//        |  30  31  32  33  |       |   0   0   0   1   |
//        |                  |       |                   |
//
//  We started by applying a transformation which is a rotation that
//  switched makes transformed coordinates (instant Reality)  have
//  the following switch:
//
//     x_i = x_a, y_i = z_a, z_i = - y_a,
//
//  where _i is instant Reality and _a is ART tracker data.
//
//
//   We can get this by multiplying by:
//
//
//          |  1  0  0  0  |
//          |              |
//          |  0  0 -1  0  |
//   T  =   |              |
//          |  0  1  0  0  |
//          |              |
//          |  0  0  0  1  |
//
//  Note this is a +90 degree rotation about the X axis.
//  After this transformation the head tracker is much closer to
//  what we need for Instant Reality apps.
//
//  We do this rotation while reading in the numbers by switching the
//  position of the elements as they are read in the sscanf()
//

void  ReadArtTracker::sendWand(const char *buf, size_t len)
{
    // We have 3 cases:
    //
    //     1 - Both buttons/joystick and position/rotation
    //
    //     2 - Just buttons/joystick
    //
    //     3 - Neither
    //

    const char *end = buf + len - wand_start_any_LEN;
    // Set s to the start of the rotation part
    for(; buf < end; ++buf)
        if(!strncmp(buf, WAND_START_ANY, wand_start_any_LEN))
            break;
    buf += wand_start_any_LEN;
    if(end <= buf + WAND_MIN_LEN)
        // We did not get flystick tracker frame.  We assume that the
        // tracker is out range.
        return;

    bool havePos = false;
    if(*buf == WAND_CHECKCHR)
        havePos = true;
    else
        ++buf; // go to the start of string: "1.000 6 2]["

    if(strncmp(buf, WAND_CHECKSTR, wand_checkstr_LEN))
            // We got data that we did not expect:
            return;
    buf += wand_checkstr_LEN;

    float x, y, z, r00, r01, r02, r10, r11, r12, r20, r21, r22;
    uint32_t buttons;
    float xjoy, yjoy;

    int n = sscanf(buf, "%f %f %f][%f %f %f %f %f %f %f %f %f][%u %f %f",
            // note y and z are switched:
            &x, &z, &y, // Not quite like the head data there is no rx ry rz
            // This is the order of the values without switching 
            //&r00, &r10, &r20,  &r02, &r12, &r22,  &r01, &r11, &r21);
            // and here they are with the transformation applied
            // but without some minus signs:
            &r00, &r20, &r10, &r01, &r21, &r11, &r02, &r22, &r12,
            &buttons, &xjoy, &yjoy);
    if(n != 15)
        // We did not get head tracker frame. This time we got more data,
        // but it was not head tracker data.  We assume that the head
        // tracker is out range.
        return;

    //std::cout << mat << std::endl;

    //std::cout << buttons << "  " << xjoy << "," << yjoy << std::endl << std::endl;

    if(!havePos) return;

    // This completes that rotation "T" as defined above.
    r20 *= -1.0F;
    r21 *= -1.0F;
    r22 *= -1.0F;
    z *= -1.0F;
 
    // scale millimeters in and meters out:
    const float scale = 0.001F;
    // We can add the following calibration offset to x, y, z Instant
    // Reality positions in meters
    const float
        x_offset = 0.0F/*meters*/,
        y_offset = 0.0F/*meters*/,
        z_offset = 0.0F/*meters*/;

    Matrix4f mat;
    mat[0] = r00; mat[1] = r01; mat[2]  = r02; mat[3]  = x * scale + x_offset;
    mat[4] = r10; mat[5] = r11; mat[6]  = r12; mat[7]  = y * scale + y_offset;
    mat[8] = r20; mat[9] = r21; mat[10] = r22; mat[11] = z * scale + z_offset;
    mat[12] = mat[13] = mat[14] = 0.0F; mat[15] = 1.0F;


    // multiple  mult() or multLeft() which is it.  There documentation
    // does not explain it.  We want  mat = wandCal * mat.
    mat.mult(wandCal);

    Vec3f pos;
    Rotation rot;
    Vec3f s;

    mat.getTransform(pos, rot, s);


    wand_matrix->push(mat);
    wand_position->push(pos);
    wand_rotation->push(rot);
}


void  ReadArtTracker::sendHead(const char *buf, size_t len)
{
    const char *end = buf + len - head_start_LEN;
    // Set s to the start of the rotation part
    for(; buf < end; ++buf)
        if(!strncmp(buf, HEAD_START, head_start_LEN))
            break;
    buf += head_start_LEN;
    if(end <= buf + HEAD_MIN_LEN)
        // We did not get head tracker frame.  We assume that the
        // tracker is out range.
        return;

    float x, y, z, rx, ry, rz, r00, r01, r02, r10, r11, r12, r20, r21, r22;
    int n = sscanf(buf, "%f %f %f %f %f %f][%f %f %f %f %f %f %f %f %f",
            // note y and z are switched:
            &x, &z, &y, &rx, &ry, &rz,
            // This is the order of the values without switching 
            //&r00, &r10, &r20,  &r02, &r12, &r22,  &r01, &r11, &r21);
            // and here they are with the transformation applied
            // but without some minus signs:
            &r00, &r20, &r10, &r01, &r21, &r11, &r02, &r22, &r12);
    if(n != 15)
        // We did not get head tracker frame. This time we got more data,
        // but it was not head tracker data.  We assume that the head
        // tracker is out range.
        return;

    // This completes that rotation "T" as defined above.
    r20 *= -1.0F;
    r21 *= -1.0F;
    r22 *= -1.0F;
    z *= -1.0F;

    // We notice that there needs to be some finer calibration.


#if 0
    static uint32_t count = 0;
    ++count;

    fprintf(file, "%u %f %f %f %f %f %f %f %f %f\n", count,
            r00, r01, r02,  r10, r11, r12,  r20, r21, r22);

    printf("IN = %f %f %f,  %f %f %f\n\n"
            "   %f %f %f\n"
            "   %f %f %f\n"
            "   %f %f %f\n\n",
            x, y, z,
            rx, ry, rz, r00, r01, r02, r10, r11, r12, r20, r21, r22);
#endif

    // scale millimeters in and meters out:
    const float scale = 0.001F;
    // We can add the following calibration offset to x, y, z Instant
    // Reality positions in meters
    const float
        x_offset = 0.07F/*meters*/,
        y_offset = 0.03175F/*meters*/,
        z_offset = 0.0F/*meters*/;

    Matrix4f mat;
    mat[0] = r00; mat[1] = r01; mat[2]  = r02; mat[3]  = x * scale + x_offset;
    mat[4] = r10; mat[5] = r11; mat[6]  = r12; mat[7]  = y * scale + y_offset;
    mat[8] = r20; mat[9] = r21; mat[10] = r22; mat[11] = z * scale + z_offset;
    mat[12] = mat[13] = mat[14] = 0.0F; mat[15] = 1.0F;

  
    // multiple  mult() or multLeft() which is it.  There documentation
    // does not explain it.  We want  mat = headCal * mat.
    mat.mult(headCal);

#if 0

    int i;
    for(i=1; i<15; ++i)
        mat[i] = 0;
    for(i=0; i<16; i += 5)
        mat[i] = 1.0F;
#endif

    //printf("xyz = %f %f %f\n\n", mat[3], mat[7], mat[11]);

    head_matrix->push(mat);
}

static inline void setWandCalibration(Matrix4f &m)
{
    //////////////////////////////////////////////////////////////////
    //         Calibrate the wand using this constant 4 x 4 matrix
    //////////////////////////////////////////////////////////////////
    
    // See comments in setHeadCalibration()

    // Rotations heading (h), pitch (p), and roll (r) as explained above.
    const float h = 0, p = 0, r = 0,
        ch = cosf(h), sh = sinf(h),
        sp = sinf(p), cp = cosf(p),
        sr = sinf(r), cr = cosf(r),
        s = 1.0F;

    m[0] = (ch*cr - sh*sp*sr)*s;   m[1] = -sh*cp*s;   m[2] = (ch*sr + sh*sp*cr)*s;  m[3] = 0;
    m[4] = (sh*cr + ch*sp*sr)*s;   m[5] =  ch*cp*s;   m[6] = (sh*sr - ch*sp*cr)*s;  m[7] = 0;
    m[8] = -cp*sr*s;               m[9] = sp*s;       m[10]= cp*cr*s;               m[11]= 0;
    m[12]= 0;                      m[13]= 0;          m[14]= 0;                     m[15]= 1.0F;
}




static inline void setHeadCalibration(Matrix4f &m)
{
    //////////////////////////////////////////////////////////////////
    //         Calibrate the head using this constant 4 x 4 matrix
    //////////////////////////////////////////////////////////////////
    
    /* For the rotation calibration we use  heading (h), pitch (p), and roll(r)
     *
     *
     * Here is the rotation part of the matrix

  where ch = cos(h), sh = sin(h),
        sp = sin(p), cp = cos(p),
        sr = sim(r), cr = cos(r),

  rotation part of the matrixes 


         /  ch  -sh  0  \ /  1  0    0   \ /   cr   0  sr  \
         |              | |              | |               |
H P R  = |  sh   ch  0  | |  0  cp  -sp  | |   0    1  0   |
         |              | |              | |               |
         \  0    0   1  / \  0  sp   cp  / \  -sr   0  cr  /


           /  ch  -sh  0  \ /   cr       0    sr       \
           |              | |                          |
H (P R)  = |  sh   ch  0  | |  (sp sr)   cp  (-sp cr)  |
           |              | |                          |
           \  0    0   1  / \  (-cp sr)  sp  (cp cr)   /


         /  (ch cr - sh sp sr)  (-sh cp)  (ch sr + sh sp cr)  \
         |                                                    |
H P R =  |  (sh cr + ch sp sr)  (ch cp)   (sh sr - ch sp cr)  |
         |                                                    |
         \  (-cp sr)            (sp)      (cp cr)             /


With Scale(s) than rotation and than translation (tx,ty,tz):


                 /  (ch cr - sh sp sr) s   (-sh cp) s  (ch sr + sh sp cr) s   tx  \
                 |                                                                |
                 |  (sh cr + ch sp sr) s   (ch cp) s   (sh sr - ch sp cr) s   ty  |
T H P R Scale =  |                                                                |
                 |  (-cp sr) s             (sp) s     (cp cr) s               tz  |
                 |                                                                |
                 \   0                      0          0                      1   /
*/

    // Rotations heading (h), pitch (p), and roll (r) as explained above.
    const float h = 0, p = -45.0F*M_PI/180.0F, r = 0,
        ch = cosf(h), sh = sinf(h),
        sp = sinf(p), cp = cosf(p),
        sr = sinf(r), cr = cosf(r),
        s = 1.0F;

    m[0] = (ch*cr - sh*sp*sr)*s;   m[1] = -sh*cp*s;   m[2] = (ch*sr + sh*sp*cr)*s;  m[3] = 0;
    m[4] = (sh*cr + ch*sp*sr)*s;   m[5] =  ch*cp*s;   m[6] = (sh*sr - ch*sp*cr)*s;  m[7] = 0;
    m[8] = -cp*sr*s;               m[9] = sp*s;       m[10]= cp*cr*s;               m[11]= 0;
    m[12]= 0;                      m[13]= 0;          m[14]= 0;                     m[15]= 1.0F;
}


// Thread method which gets automatically started as soon as a slot is
// connected
int ReadArtTracker::processData()
{
    SPEW();

    setHeadCalibration(headCal);
    setWandCalibration(wandCal);


    setState(NODE_RUNNING);

    assert(head_matrix);

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

        // terminate the string.
        buf[ret] = '\0';

        printf("read(%zd bytes) = %s\n", ret, buf);

        if(ret > HEAD_MIN_LEN)
        {
            sendHead(buf, ret);
            sendWand(buf, ret);
        }



    }
    
    // Thread finished
    setState(NODE_SLEEPING);

    SPEW();  
    return 0;
}

} // namespace InstantIO
