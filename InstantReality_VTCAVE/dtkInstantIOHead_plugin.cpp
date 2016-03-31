//#define NDEBUG
#include <assert.h>
#include <math.h>
#include <string>
#include <stdlib.h>
#include <iostream>
#include <sstream>

#include <InstantIO/ThreadedNode.h>
#include <InstantIO/NodeType.h>
#include <InstantIO/OutSlot.h>
#include <InstantIO/MFTypes.h>
#include <InstantIO/Vec3.h>
#include <InstantIO/Matrix4.h>

#include <dtk.h>

#include <string.h>

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define SPEW()   std::cerr << __FILENAME__ << ":" << __LINE__\
  << " " <<  __func__ << "()" << std::endl


namespace InstantIO
{

template <class T> class OutSlot;

class DtkHead : public ThreadedNode
{
public:
    DtkHead();
    virtual ~DtkHead();

    // Factory method to create an instance of DtkHead.
    static Node *create();
  
    // Factory method to return the type of DtkHead.
    virtual NodeType *type() const;
  
protected:
    // Gets called when the DtkHead is enabled.
    virtual void initialize();
  
    // Gets called when the DtkHead is disabled.
    virtual void shutdown();
  
    // thread method to send/receive data.
    virtual int processData ();
  
private:

    OutSlot<Matrix4f> *viewPointOutSlot_;
  
    static NodeType type_;
};


NodeType DtkHead::type_(
    "dtkInstantIOhead" /*typeName must be the same as plugin filename */,
    &create,
    "DTK shared memory head to InstantIO" /*shortDescription*/,
    /*longDescription*/
    "sets view frustum view point Matrix4f by reading DTK shared memory named head",
    "lanceman"/*author*/,
    0/*fields*/,
    0/sizeof(Field));

DtkHead::DtkHead() :
    ThreadedNode()
{
    SPEW();
    // Add external route
    addExternalRoute("*", "{NamespaceLabel}/{SlotLabel}");

    SPEW();
}

DtkHead::~DtkHead()
{
    SPEW();
}

Node *DtkHead::create()
{
    SPEW();  
    return new DtkHead;
}

NodeType *DtkHead::type() const
{
    SPEW();  
    return &type_;
}

void DtkHead::initialize()
{
    SPEW();  
    
    // handle state and namespace updates
    Node::initialize();

    viewPointOutSlot_ = new OutSlot<Matrix4f>(
        "changing frustum view points (apexes) via DTK shared memory", Matrix4f());
    assert(viewPointOutSlot_);
    viewPointOutSlot_->addListener(*this);
    addOutSlot("head", viewPointOutSlot_);
}

void DtkHead::shutdown()
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

static inline
bool IsSameOrSetFloat6(float x[6], const float y[6])
{
    int i;
    for(i=0;i<6;++i)
        if(x[i] != y[i])
            break;
    if(i == 6) return true; // they are the same
    // cannot use sizeof(x) below.
    memcpy(x, y, 6*sizeof(float));
    return false;
}

// Thread method which gets automatically started as soon as a slot is
// connected
int DtkHead::processData()
{
    SPEW(); 

    setState(NODE_RUNNING);

    assert(viewPointOutSlot_);

    // It would appear that someone decided that
    // model units are in meters so a model with
    // some length equal to 1 corresponds to 1 meter.
    //
    // The DIVERSE CAVE cube size it 10 feet on a side.
    // Tracker position origin is at the center of the
    // CAVE cube corresponds to position (x,y,z)=(0,0,0)
    // The distance to the walls is 5 feet which is in
    // given as a distance of 1.0F in the shared memory.
    //
    float scale = 1.524F; // for VT CAVE 1/2 the length of a side in meters
    float loc[6];
    float oldLoc[6] = { NAN, NAN, NAN, NAN, NAN, NAN };
    dtkMatrix mat;

    dtkSharedMem* shm = new dtkSharedMem(6*sizeof(float), "head");
    assert(shm);
    if(!shm) return 1; // fail

    // TODO: We should be waiting at the sharedMemory read call and
    // not here.  That would give much better performance in so many
    // ways.  Problem is, we have no easy way to interrupt the blocking
    // shm->blockingRead(loc) call at quiting time.
    while(waitThread(10))
    {
        if(shm->read(loc)) // not blocking here like a good thread should not
        //if(shm->blockingRead(loc)) // Would be better
        //blocking here like a good thread should.
            // The above shm->read() should have spewed about the error.
            return -1; // fail

        // TODO: convert units and transform
        // TODO: Just moving viewpoint position for now
        if(IsSameOrSetFloat6(oldLoc, loc)) continue;

        // ===============================================================
        // ======================== CAUTION ==============================
        // ===============================================================
        // Do not use dtkCoord to build this mat there is a nasty bug in
        // the setting of dtkMatrix for a dtkCoord when using any of the
        // dtkMatrix methods that do it.  Unless you like PAIN.
        // Thu Jan 14 21:36:50 EST 2016
        // ===============================================================

        mat.identity();
        mat.rotateHPR(loc[3], loc[4], loc[5]);
        mat.translate(loc[0]*scale, loc[1]*scale, loc[2]*scale);
        mat.rotateHPR( 0.0f, -90.0F, 0.0F );

        // TODO: add a x,y,z, scaling.
	
        Matrix4f tracker_mat;

#if 0 // Both these methods give the same result. Confirmed it.
        // This method may be the faster of the two.
        int i, j;
	for(i=0;i<4;++i)
	    for(j=0; j<4; ++j)
		tracker_mat[i*4+j] = mat.element(i, j);
        //mat.print(stderr);
#else // Both these methods give the same result #if 1 or 0

        Vec3f tracker_pos;
	mat.translate( &tracker_pos[0], &tracker_pos[1], &tracker_pos[2] );
        Rotation tracker_rot;
	mat.quat( &tracker_rot[0], &tracker_rot[1], &tracker_rot[2], &tracker_rot[3] );
#if 0
std::cerr << "head (" << 
    tracker_pos[0] << " " << tracker_pos[1] << " " << tracker_pos[2] <<
    ") (" <<
    tracker_rot[0] << " " << tracker_rot[1] << " " <<
    tracker_rot[2] << " " << tracker_rot[3] <<
    ")" << std::endl;
#endif

        tracker_mat.setTransform( tracker_pos, tracker_rot);
#endif

        viewPointOutSlot_->push(tracker_mat);
    }

    delete shm;

    // Thread finished
    setState(NODE_SLEEPING);

    SPEW();  

    return 0;
}

} // namespace InstantIO
