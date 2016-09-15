#include <string>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <sstream>

#include <InstantIO/OutSlot.h>
#include <InstantIO/FieldAccessor.h>
#include <InstantIO/InstantIODef.h>
#include <InstantIO/ThreadedNode.h>
#include <InstantIO/NodeType.h>
#include <InstantIO/Rotation.h>
#include <InstantIO/Vec3.h>

#include <vrpn_Tracker.h>
#include <vrpn_Button.h>

static inline void OutputDebugString(const char *str)
{
    printf("%s\n", str);
}

namespace InstantIO
{

template <class T> class OutSlot;


class ZSpaceNode : public ThreadedNode
{
public:
    ZSpaceNode();
    virtual ~ZSpaceNode();

    // Create setter and getter methods for a field.
    INSTANTIO_SETTER_GETTER(float, RandMax, randMax_);

	INSTANTIO_SETTER_GETTER(std::string, ConnectionString, connectionString_);
  
    // Factory method to create an instance of ZSpaceNode.
    static Node *create();
  
    // Factory method to return the type of ZSpaceNode.
    virtual NodeType *type() const;

	static void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b);
    static void VRPN_CALLBACK handle_tracker(void *userdata,const vrpn_TRACKERCB t);

	vrpn_Tracker_Remote *headtracker_;
	vrpn_Tracker_Remote *handtracker_;
	vrpn_Button_Remote  *button_;
  
protected:
    // Gets called when the ZSpaceNode is enabled.
    virtual void initialize();
  
    // Gets called when the ZSpaceNode is disabled.
    virtual void shutdown();
  
    // thread method to send/receive data.
    virtual int processData ();
  
private:
	std::vector<float> headtrackerData;				//head tracker data
	std::vector<float> handtrackerData;				//hand tracker data
	std::vector<int> buttonData;				//stylus button data
    ZSpaceNode(const ZSpaceNode &);
    const ZSpaceNode &operator=(const ZSpaceNode &);

    // A "static" field of ZSpaceNode.
    float randMax_;
	
	std::string connectionString_;

    // Dynamic slots
    OutSlot<Vec3f>    *headposition_;
    OutSlot<Rotation> *headrotation_;
    OutSlot<Vec3f>    *handposition_;
    OutSlot<Rotation> *handrotation_;
	OutSlot<bool> *button_0;
	OutSlot<bool> *button_1;
	OutSlot<bool> *button_2;
  
    // Type and type attributes
    static NodeType type_;
    static const char *typeName_;
    static const char *shortDescription_;
    static const char *longDescription_;
    static const char *author_;
    static Field fields_[2];
};

} // namespace InstantIO


namespace InstantIO {

// Node Type 
NodeType ZSpaceNode::type_(
    typeName_,
    &create,
    shortDescription_,
    longDescription_,
    author_,
    fields_,
    sizeof(fields_) / sizeof(Field));

const char *ZSpaceNode::typeName_ = 
    "ZSpaceNode";

const char *ZSpaceNode::shortDescription_ =
    "Driver for ZSpace trackers.";

const char *ZSpaceNode::longDescription_ = 
    "This is the long description for the ZSpace Node";

const char *ZSpaceNode::author_ =
    "Wolex";

// ###ADDCODE###
// Add additional fields here
Field ZSpaceNode::fields_[] = {
    Field(
        "randMax",                              // Name
        "The rand max for the vector values",   // description
        "3",                                   // default value
        FieldAccessor<ZSpaceNode, float>(&ZSpaceNode::setRandMax, &ZSpaceNode::getRandMax)
    ),

	Field(
		"connectionString",
		"The VRPNServer url",
		"Tracker0@localhost",
		FieldAccessor<ZSpaceNode, std::string>(&ZSpaceNode::setConnectionString, &ZSpaceNode::getConnectionString)
	)
};

//----------------------------------------------------------------------
ZSpaceNode::ZSpaceNode() : 
    ThreadedNode(),
    randMax_(0),
    headposition_(0), 
    headrotation_(0),
    handposition_(0), 
    handrotation_(0),
	button_0(0),
	button_1(0),
	button_2(0)
{
	headtrackerData.push_back(0.0);		
	headtrackerData.push_back(0.0);
	headtrackerData.push_back(0.0);
	headtrackerData.push_back(0.0);
	headtrackerData.push_back(0.0);
	headtrackerData.push_back(0.0);
	headtrackerData.push_back(0.0);
	headtrackerData.push_back(0.0);
	handtrackerData.push_back(0.0);		
	handtrackerData.push_back(0.0);
	handtrackerData.push_back(0.0);
	handtrackerData.push_back(0.0);
	handtrackerData.push_back(0.0);
	handtrackerData.push_back(0.0);
	handtrackerData.push_back(0.0);
	handtrackerData.push_back(0.0);
	buttonData.push_back(0);		
	buttonData.push_back(0);
	buttonData.push_back(0);

	// Add external route
    addExternalRoute("*", "{NamespaceLabel}/{SlotLabel}");
}

//----------------------------------------------------------------------
ZSpaceNode::~ZSpaceNode()
{
}

//----------------------------------------------------------------------
Node *ZSpaceNode::create()
{
    return new ZSpaceNode;
}

//----------------------------------------------------------------------
NodeType *ZSpaceNode::type() const
{
    return &type_;
}

//----------------------------------------------------------------------
void ZSpaceNode::initialize()
{
	const char* deviceName = "Tracker0@localhost";

    // handle state and namespace updates
    Node::initialize();

    // ###ADDCODE###
    // check/open the device/device-system state
    // create the dynamic slots for the current device configuration
    // setState (NODE_ERROR) on error
    //std::cerr << "ZSpace: Run initialize" << std::endl;
	OutputDebugString("ZSpace: Run initialize");
	//addLog("ZSpace: Run initialize\n");

	headtracker_ = new vrpn_Tracker_Remote(deviceName);
	handtracker_ = new vrpn_Tracker_Remote(deviceName);
	button_ = new vrpn_Button_Remote(deviceName);

	headtracker_->register_change_handler(&headtrackerData, this->handle_tracker, 0);
	handtracker_->register_change_handler(&handtrackerData, this->handle_tracker, 1);
	button_->register_change_handler(&buttonData, this->handle_button);

    headposition_ = new OutSlot<Vec3f>("Position of head tracker", Vec3f(0.f, 0.f, 0.f));
    assert(headposition_ != 0);
    headposition_->addListener(*this);
    addOutSlot("headposition", headposition_);

    headrotation_ = new OutSlot<Rotation>("Rotation of head tracker", Rotation());
    assert(headrotation_ != 0);
    headrotation_->addListener(*this);
    addOutSlot("headrotation", headrotation_);

    handposition_ = new OutSlot<Vec3f>("Position of hand tracker", Vec3f(0.f, 0.f, 0.f));
    assert(handposition_ != 0);
    handposition_->addListener(*this);
    addOutSlot("handposition", handposition_);

    handrotation_ = new OutSlot<Rotation>("Rotation of hand tracker", Rotation());
    assert(handrotation_ != 0);
    handrotation_->addListener(*this);
    addOutSlot("handrotation", handrotation_);

	button_0 = new OutSlot<bool>( "Button 0", false );
	assert( button_0 != 0 );
	button_0->addListener( *this );
	addOutSlot( "button_0", button_0 );

	button_1 = new OutSlot<bool>( "Button 1", false );
	assert( button_1 != 0 );
	button_1->addListener( *this );
	addOutSlot( "button_1", button_1 );

	button_2 = new OutSlot<bool>( "Button 2", false );
	assert( button_2 != 0 );
	button_2->addListener( *this );
	addOutSlot( "button_2", button_2 );

}

//----------------------------------------------------------------------
void ZSpaceNode::shutdown()
{
    // handle state and namespace updates
    Node::shutdown();

    // ###ADDCODE###
    // remove all dynamic slots and do other cleanups
    // setState (NODE_ERROR) on error

    //std::cerr << "ZSpace: Run shutdown" << std::endl;
	OutputDebugString("ZSpace: Run shutdown");
    if (headposition_ != 0)
    {
        removeOutSlot("headposition", headposition_);
        delete headposition_;
        headposition_ = 0;
    }
	if (headrotation_!=0)
    {
        removeOutSlot("headrotation", headrotation_);
        delete headrotation_;
        headrotation_ = 0;
    }
    if (handposition_ != 0)
    {
        removeOutSlot("handposition", handposition_);
        delete handposition_;
        handposition_ = 0;
    }
	if (handrotation_!=0)
    {
        removeOutSlot("handrotation", handrotation_);
        delete handrotation_;
        handrotation_ = 0;
    }
	if (button_0!=0)
    {
        removeOutSlot("button_0", button_0);
        delete button_0;
        button_0 = 0;
    }
	if (button_1!=0)
    {
        removeOutSlot("button_1", button_1);
        delete button_1;
        button_1 = 0;
    }
	if (button_2!=0)
    {
        removeOutSlot("button_2", button_2);
        delete button_2;
        button_2 = 0;
    }
}

// Thread method which gets automatically started as soon as a slot is
// connected
int ZSpaceNode::processData()
{
    Vec3f headpos;
    Rotation headrot;
    Vec3f handpos;
    Rotation handrot;

	// ###ADDCODE###
    // Open/connect to you device
    // Send/receive data in the waitThread() loop 
    // setState (NODE_ERROR) and return -1 on error

    bool deviceOpen = true;

     //std::cerr << "ZSpace: Run processData\n" << std::endl;
	 //addLog("ZSpace: Run processData\n");
	OutputDebugString("ZSpace: Run processData");

    if (deviceOpen)
        setState(NODE_RUNNING);
    else
    {
        setState (NODE_ERROR);
        return -1;
    }

    // Important: you need to wall waitThread in every loop
    // time is in millisecond

    while (waitThread(1))
    {
		//handle trackers
		headtracker_->mainloop();
		//head tracker
		if (headposition_ != 0)
		{      
			headpos.set( headtrackerData[0], headtrackerData[1], headtrackerData[2] );
			headposition_->push(headpos);
		}

		if (headrotation_ != 0)
		{
			headrot.set ( headtrackerData[3], headtrackerData[4], headtrackerData[5], headtrackerData[6] );
			headrotation_->push(headrot);
		}
		//hand tracker
		handtracker_->mainloop();
		if (handposition_ != 0)
		{      
			handpos.set( handtrackerData[0], handtrackerData[1], handtrackerData[2] );
			handposition_->push(handpos);
		}

		if (handrotation_ != 0)
		{
			handrot.set ( handtrackerData[3], handtrackerData[4], handtrackerData[5], handtrackerData[6] );
			handrotation_->push(handrot);
		}
		//handle buttons
		button_->mainloop();
		if (button_0!=0) button_0->push(buttonData[0]!=0);
		if (button_1!=0) button_1->push(buttonData[1]!=0);
		if (button_2!=0) button_2->push(buttonData[2]!=0);
    }

    // Thread finised
	setState(NODE_SLEEPING);

    //###ADDCODE### 

    //std::cerr << "ZSpace: finish processData" << std::endl;
	OutputDebugString("ZSpace: finish processData");

    return 0;
}


void VRPN_CALLBACK ZSpaceNode::handle_tracker(void *userdata, const vrpn_TRACKERCB t)
{
	std::vector<float> tData;
	
	tData.push_back(t.pos[0]);
	tData.push_back(t.pos[1]);
	tData.push_back(t.pos[2]);
	tData.push_back(t.quat[0]);
	tData.push_back(t.quat[1]);
	tData.push_back(t.quat[2]);
	tData.push_back(t.quat[3]);
	tData.push_back(t.sensor);
	*((std::vector<float> *)(userdata)) = tData;
	//if (t.sensor == 0)//head
	//else if (t.sensor == 1)//stylus
}

void VRPN_CALLBACK ZSpaceNode::handle_button(void *userdata, const vrpn_BUTTONCB b)
{
	std::vector<int> tData(3,0);
	char buffer [50];
	sprintf(buffer, "Button number %d was just %s\n",  b.button, b.state ? "pressed":"released");
	OutputDebugString(buffer);
	tData[b.button] = b.state;
	*((std::vector<int> *)(userdata)) = tData;
}

} // namespace InstantIO
