
// define DEBUG_SPEW to have this spew every frame to stdout
#define DEBUG_SPEW


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

#include "readARTCommon.h"

#ifdef DEBUG_SPEW
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
protected:
    // Gets called when the Wand is enabled.
    virtual void initialize();
  
    // Gets called when the Wand is disabled.
    virtual void shutdown();
  
    // thread method to send/receive data.
    virtual int processData ();
  
private:
    // Factory method to create an instance of Wand.
    static Node *create();

    Wand(void);
    virtual ~Wand();

    template <class T>
    OutSlot<T> *getSlot(const char *desc, const char *name);

    template <class T>
    void removeSlot(T *slot, const char *name);

    float oldJoyX;
    float oldJoyY;
    
    OutSlot<Matrix4f> *wand_matrix;
    OutSlot<Rotation> *wand_rotation;
    OutSlot<Vec3f> *wand_position;
    OutSlot<float> *joystick_x_axis;
    OutSlot<float> *joystick_y_axis;
    OutSlot<bool> *button_1;
    OutSlot<bool> *button_2;
    OutSlot<bool> *button_3;
    OutSlot<bool> *button_4;
    OutSlot<bool> *button_5;
    OutSlot<bool> *button_6;
    OutSlot<bool> *button_7;
    OutSlot<bool> *button_8;
};


// Somehow InstantReality reads this data structure
// no matter what it's called.
static NodeType _type_InstantReality_stupid_data(
    "readARTWand" /*typeName must be the same as plugin filename */,
    &Wand::create,
    "head and wand Tracker to InstantIO" /*shortDescription*/,
    /*longDescription*/
    "head and wand Tracker to InstantIO",
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

void Wand::shutdown()
{
     // handle state and namespace updates
    Node::shutdown();

    SPEW();

    removeSlot(wand_matrix, "wandmatrix");

    removeSlot(wand_rotation, "wandrotation");
    removeSlot(wand_position, "wandposition");

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


// Thread method which gets automatically started as soon as a slot is
// connected
int Wand::processData()
{
    SPEW();

    setState(NODE_RUNNING);

    uint32_t oldButtons = 0;
    float oldWandXAxis = 0, oldWandYAxis = 0;
    int ret;

    ret = pthread_mutex_lock(&art_mutex);
    assert(ret == 0);

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
        ret = pthread_cond_wait(&art_cond, &art_mutex);
        assert(ret == 0);

        if(oldButtons != art_buttons)
        {
            oldButtons = art_buttons;
            button_1->push((01   ) & art_buttons);
            button_2->push((01<<1) & art_buttons);
            button_3->push((01<<2) & art_buttons);
            button_4->push((01<<3) & art_buttons);
            button_5->push((01<<4) & art_buttons);
            button_6->push((01<<5) & art_buttons);
            button_7->push((01<<6) & art_buttons);
            button_8->push((01<<7) & art_buttons);
        }

        if(oldWandXAxis != art_wandXAxis)
        {    
            oldWandXAxis = art_wandXAxis;
            joystick_x_axis->push(art_wandXAxis);
        }

        if(oldWandYAxis != art_wandYAxis)
        {    
            oldWandYAxis = art_wandYAxis;
            joystick_y_axis->push(art_wandYAxis);
        }

        if(art_havePosRot)
        {
            wand_matrix->push(art_wandMat);
            wand_position->push(art_wandPosition);
            wand_rotation->push(art_wandRotation);
        }
    }

    ret = pthread_mutex_unlock(&art_mutex);
    assert(ret == 0);

    // Thread finished
    setState(NODE_SLEEPING);

    SPEW();  
    return 0;
}

} // namespace InstantIO
