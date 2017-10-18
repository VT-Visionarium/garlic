//#define NDEBUG
#include <assert.h>

#include <math.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string.h>


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
class HeadTracker : public ThreadedNode
{
public:
    HeadTracker();
    virtual ~HeadTracker();

    // Factory method to create an instance of HeadTracker.
    static Node *create();
  
    // Factory method to return the type of HeadTracker.
    virtual NodeType *type() const;
  
protected:
    // Gets called when the HeadTracker is enabled.
    virtual void initialize();
  
    // Gets called when the HeadTracker is disabled.
    virtual void shutdown();
  
    // thread method to send/receive data.
    virtual int processData ();
  
private:

    OutSlot<Matrix4f> *viewPointOutSlot_;
  
    static NodeType type_;
};


NodeType HeadTracker::type_(
    "readUDP" /*typeName must be the same as plugin filename */,
    &create,
    "head Tracker to InstantIO" /*shortDescription*/,
    /*longDescription*/
    "sets head view frustum view point Matrix4f",
    "lance"/*author*/,
    0/*fields*/,
    0/sizeof(Field));

HeadTracker::HeadTracker() :
    ThreadedNode()
{
    SPEW();
    // Add external route
    addExternalRoute("*", "{NamespaceLabel}/{SlotLabel}");

    SPEW();
}

HeadTracker::~HeadTracker()
{
    SPEW();
}

Node *HeadTracker::create()
{
    SPEW();  
    return new HeadTracker;
}

NodeType *HeadTracker::type() const
{
    SPEW();  
    return &type_;
}

void HeadTracker::initialize()
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

void HeadTracker::shutdown()
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
}


// Thread method which gets automatically started as soon as a slot is
// connected
int HeadTracker::processData()
{
    SPEW(); 

    setState(NODE_RUNNING);

    assert(viewPointOutSlot_);

    // It would appear that someone decided that
    // model units are in meters so a model with
    // some length equal to 1 corresponds to 1 meter.
    //
    while(waitThread(10))
    {
	
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
