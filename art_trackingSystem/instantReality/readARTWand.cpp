// NOTE: there is a calibration of the Wand tracker data in
// readARTHead_plugin.cpp (or readARTHead.cpp).


// define DEBUG_SPEW to have this spew every frame to stdout
//#define DEBUG_SPEW


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
#include <pthread.h>
#include <dlfcn.h>

#include <InstantIO/ThreadedNode.h>
#include <InstantIO/NodeType.h>
#include <InstantIO/OutSlot.h>
#include <InstantIO/MFTypes.h>
#include <InstantIO/Vec3.h>
#include <InstantIO/Matrix4.h>

// instead we use getenv() below:
//#include "readARTCommon.h"

// We get addresses from the readARTHead.cpp module which runs in another
// thread in this process.  It must have been started before this thread.
//
// Actually this will be called in dlsym() call in the main process of
// InstantReality (IR), when IR loads this module just before running the
// thread for the C++ object methods below.  so long as the other module
// in readARTHead.cpp is loaded and started we'll be able to get these
// addresses.
template <class T>
static T *getAddr(const char *sym)
{
    T *p;
    char name[128];
    snprintf(name, 128, "IR_SUCKS_%s", sym);
    //printf("getting address of \"%s\"\n", sym);
    const char *env = getenv(name);
    printf("%s=%s\n", name, env);
    assert(env);
    p = (T*) (uintptr_t) strtoul(env, 0, 16);
    assert(p);
    return p;
}


// work around sucky InstantReality interface:
// We need to access this data without InstantReality interfering.  The
// interfaces that InstantReality provides slows things down.  We want to
// read this data and we will use our own synchronization primitives,
// thank you.
pthread_mutex_t&     art_mutex          = *getAddr< pthread_mutex_t     >("art_mutex");
pthread_cond_t&      art_cond           = *getAddr< pthread_cond_t      >("art_cond");
bool&                art_haveWandPosRot = *getAddr< bool                >("art_haveWandPosRot");
bool&                art_haveHead       = *getAddr< bool                >("art_haveHead");
InstantIO::Matrix4f& art_headMatrix     = *getAddr< InstantIO::Matrix4f >("art_headMatrix");
InstantIO::Matrix4f& art_wandMatrix     = *getAddr< InstantIO::Matrix4f >("art_wandMatrix");
InstantIO::Vec3f&    art_wandPosition   = *getAddr< InstantIO::Vec3f    >("art_wandPosition");
InstantIO::Rotation& art_wandRotation   = *getAddr< InstantIO::Rotation >("art_wandRotation");
float&               art_wandXAxis      = *getAddr< float               >("art_wandXAxis");
float&               art_wandYAxis      = *getAddr< float               >("art_wandYAxis");
uint32_t&            art_buttons        = *getAddr< uint32_t            >("art_buttons");



#if 1
#  define SPEW()   std::cerr << __BASE_FILE__ << ":" << __LINE__\
  << " " <<  __func__ << "()" << std::endl
#else
#  define SPEW()  /*empty macro*/
#endif


namespace InstantIO
{

template <class T> class OutSlot;
class Wand : public ThreadedNode
{

public:
    // Factory method to create an instance of Wand.
    static Node *create();

protected:
    // Gets called when the Wand is enabled.
    virtual void initialize();
  
    // Gets called when the Wand is disabled.
    virtual void shutdown();
  
    // thread method to send/receive data.
    virtual int processData ();
  
private:
    Wand(void);
    virtual ~Wand();

    template <class T>
    OutSlot<T> *getSlot(const char *desc, const char *name);

    template <class T>
    void removeSlot(T *slot, const char *name);
    void updateButtons(void);

    uint32_t oldButtons;
    float oldJoyX;
    float oldJoyY;

    OutSlot<Matrix4f> *head_matrix;
    OutSlot<Matrix4f> *wand_matrix;
    OutSlot<Rotation> *wand_rotation;
    OutSlot<Vec3f> *wand_position;
    OutSlot<float> *joystick_x_axis;
    OutSlot<float> *joystick_y_axis;
    OutSlot<bool> *button[8];
};


// Somehow InstantReality reads this data structure
// no matter what it's called.
static NodeType InstantReality_stupid_data(
    "readARTWand" /*typeName must be the same as plugin filename */,
    &Wand::create,
    "wand Tracker to InstantIO" /*shortDescription*/,
    /*longDescription*/
    "wand Tracker to InstantIO",
    "lance"/*author*/,
    0/*fields*/,
    0/sizeof(Field));


Wand::Wand()
{
    SPEW();
    // Add external route
    addExternalRoute("*", "{NamespaceLabel}/{SlotLabel}");
}

Wand::~Wand()
{
    SPEW();
}

Node *Wand::create()
{
    SPEW();
    return new Wand;
}


template <class T>
OutSlot<T> *Wand::getSlot(const char *desc, const char *name)
{
    OutSlot<T> *slot;
    slot = new OutSlot<T>(desc, T());
    assert(slot);
    slot->addListener(*this);
    addOutSlot(name, slot);
    return slot;
}

template <class T>
void Wand::removeSlot(T *slot, const char *name)
{
    assert(slot);
    removeOutSlot(name, slot);
    delete slot;
}

void Wand::initialize()
{
    SPEW();

    // handle state and namespace updates
    Node::initialize();

    head_matrix = getSlot<Matrix4f>("matrix of head", "headmatrix");
    wand_matrix = getSlot<Matrix4f>("matrix of wand", "wandmatrix");

    wand_rotation = getSlot<Rotation>("Rotation of wand", "wandrotation");
    wand_position = getSlot<Vec3f>( "Position of wand", "wandposition");
 
    joystick_x_axis = getSlot<float>("Joystick x axis", "joystick_x_axis");
    joystick_y_axis = getSlot<float>("Joystick y axis", "joystick_y_axis");

    button[0] = getSlot<bool>("Button 1", "button_1");
    button[1] = getSlot<bool>("Button 2", "button_2");
    button[2] = getSlot<bool>("Button 3", "button_3");
    button[3] = getSlot<bool>("Button 4", "button_4");
    button[4] = getSlot<bool>("Button 5", "button_5");
    button[5] = getSlot<bool>("Button 6", "button_6");
    button[6] = getSlot<bool>("Button 7", "button_7");
    button[7] = getSlot<bool>("Button 8", "button_8");
}

void Wand::shutdown()
{
     // handle state and namespace updates
    Node::shutdown();

    SPEW();

    removeSlot(head_matrix, "headmatrix");
    removeSlot(wand_matrix, "wandmatrix");

    removeSlot(wand_rotation, "wandrotation");
    removeSlot(wand_position, "wandposition");

    removeSlot(joystick_x_axis, "joystick_x_axis");
    removeSlot(joystick_y_axis, "joystick_y_axis");

    removeSlot(button[0], "button_1");
    removeSlot(button[1], "button_2");
    removeSlot(button[2], "button_3");
    removeSlot(button[3], "button_4");

    removeSlot(button[4], "button_5");
    removeSlot(button[5], "button_6");
    removeSlot(button[6], "button_7");
    removeSlot(button[7], "button_8");
}


// We need a art_mutex lock to call this.
void Wand::updateButtons(void)
{
    oldButtons = art_buttons;
    // This needs to be where we map bit to button.
    // We should not "push" buttons anywhere else in
    // this code.
    button[0]->push((01   ) & art_buttons);
    button[1]->push((01<<1) & art_buttons);
    button[2]->push((01<<2) & art_buttons);
    button[3]->push((01<<3) & art_buttons);
    button[4]->push((01<<4) & art_buttons);
    button[5]->push((01<<5) & art_buttons);
    button[6]->push((01<<6) & art_buttons);
    button[7]->push((01<<7) & art_buttons);
}


// Thread method which gets automatically started as soon as a slot is
// connected
int Wand::processData()
{
    SPEW();

    setState(NODE_RUNNING);

    oldButtons = 0;
    float oldWandXAxis = 0, oldWandYAxis = 0;
    int ret;

    ret = pthread_mutex_lock(&art_mutex);
    assert(ret == 0);


    //////////////////////////////////////////////////////////////
    // Initialize the slots
    //
    // We are assuming that this module is loaded after the
    // readARTHead module.
    //
    updateButtons();

    oldWandXAxis = art_wandXAxis;
    joystick_x_axis->push(art_wandXAxis);

    oldWandYAxis = art_wandYAxis;
    joystick_y_axis->push(art_wandYAxis);
            
    wand_matrix->push(art_wandMatrix);
    wand_position->push(art_wandPosition);
    wand_rotation->push(art_wandRotation);

    head_matrix->push(art_headMatrix);
    //
    //
    //////////////////////////////////////////////////////////////

    //
    // waitThread() seems to be the hook that lets instant reality know
    // when we are starting/ending a cycle and we can continue running
    // if it returns true.
    // By it's name it's clear that they are catering to stupid
    // programmers that put sleeps in their code, and don't know that the
    // OS has other blocking calls, besides sleep, that handle system
    // resources very efficiently. 
    //
    while(waitThread(0))
    {
        // We wait for data from readHeadARTHead.cpp to
        // write to the art_* data.  They will signal us
        // via pthread_cond_signal().
        //
        // We will loose the mutex lock while we wait.
        ret = pthread_cond_wait(&art_cond, &art_mutex);

        // We now have the lock.
        
        // Now we have the mutex lock again.
        assert(ret == 0);

        // We wake up and just push the data that the
        // readARTHead.cpp module got for us and then
        // loop, and do it again.

        if(oldButtons != art_buttons)
            updateButtons();

        if(oldWandXAxis != art_wandXAxis)
        {    
            oldWandXAxis = art_wandXAxis;
            joystick_x_axis->push(art_wandXAxis);
            //std::cout << "xaxis=" << art_wandXAxis << std::endl;
        }


        if(oldWandYAxis != art_wandYAxis)
        {    
            oldWandYAxis = art_wandYAxis;
            joystick_y_axis->push(art_wandYAxis);
            //std::cout << "       yaxis=" << art_wandYAxis << std::endl;
        }

        if(art_haveWandPosRot)
        {
            wand_matrix->push(art_wandMatrix);
            wand_position->push(art_wandPosition);
            wand_rotation->push(art_wandRotation);
#ifdef DEBUG_SPEW
            std::cout << "----- art_wandMatrix -----" << std::endl;
            std::cout << art_wandMatrix << std::endl;
#endif
        }

        if(art_haveHead)
            // We already have this head matrix in readARTHead*.cpp but
            // that module is not visible from the <scene> node so
            // we copy the same head matrix to this IOSensor too.
            // Yup, working around Instant Reality's lack of documenting
            // how to ROUTE data from the Engine node to the scene node.
            head_matrix->push(art_headMatrix);

        // loop
    }

    ret = pthread_mutex_unlock(&art_mutex);
    assert(ret == 0);

    // Thread finished
    setState(NODE_SLEEPING);

    SPEW();  
    return 0;
}

} // namespace InstantIO
