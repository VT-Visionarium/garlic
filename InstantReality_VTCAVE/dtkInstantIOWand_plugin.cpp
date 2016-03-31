/* This is slopped together from Patrick Shinpaugh's old code
 * from the VT CAVE (vis cube) which had GNU/Linux cluster
 * of computers driving the 4 walls with 16 projectors. */

#include <string>
#include <iostream>
#include <sstream>

#include <InstantIO/FieldAccessor.h>
#include <InstantIO/InstantIODef.h>
#include <InstantIO/ThreadedNode.h>
#include <InstantIO/NodeType.h>
#include <InstantIO/Matrix4.h>
#include <InstantIO/Rotation.h>
#include <InstantIO/Vec3.h>
#include <InstantIO/OutSlot.h>

#include <dtk.h>

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define SPEW()   std::cerr << __FILENAME__ << ":" << __LINE__\
  << " " <<  __func__ << "()" << std::endl


using namespace InstantIO;

namespace InstantIO
{
	class dtkInstantIOWandNode : public ThreadedNode
	{
	public:
		dtkInstantIOWandNode();
		virtual ~dtkInstantIOWandNode();

		static Node* create();

		virtual NodeType* type() const;

	protected:
		virtual void initialize();
		virtual void shutdown();
		virtual int processData ();

	private:
		dtkInstantIOWandNode( const dtkInstantIOWandNode& );
		const dtkInstantIOWandNode&
                    operator=( const dtkInstantIOWandNode& );

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

		dtkSharedMem* shm_wand;
		dtkSharedMem* shm_joystick;
		dtkSharedMem* shm_buttons;

		static NodeType type_;
	};



NodeType dtkInstantIOWandNode::type_(
	"dtkInstantIOWand"/*typeName must be the same as plugin filename */,
	&create,
	"Driver for 6DOF wand using DTK.", // short decription
	"Driver for 6DOF wand using DTK.", // long description
	"Patrick edited by lance", // author 
	0, //fields_,
	0 //sizeof( fields_ ) / sizeof( Field )
        );



dtkInstantIOWandNode::dtkInstantIOWandNode()
    :ThreadedNode(),
    wand_matrix(0),
    wand_rotation(0),
    wand_position(0),
    joystick_x_axis(0),
    joystick_y_axis(0),
    button_1(0),
    button_2(0),
    button_3(0),
    button_4(0),
    button_5(0),
    button_6(0),
    button_7(0),
    button_8(0),
    shm_wand(0),
    shm_joystick(0),
    shm_buttons(0)
{
    SPEW();
	addExternalRoute( "*", "{NamespaceLabel}/{SlotLabel}" );
}

dtkInstantIOWandNode::~dtkInstantIOWandNode()
{
}

Node* dtkInstantIOWandNode::create()
{
	return new dtkInstantIOWandNode;
}

NodeType* dtkInstantIOWandNode::type() const
{
	return &type_;
}

void dtkInstantIOWandNode::initialize()
{
    Node::initialize();

    wand_matrix = new OutSlot<Matrix4f>( "Matrix of wand tracker", Matrix4f() );
    assert( wand_matrix != 0 );
    wand_matrix->addListener( *this );
    addOutSlot( "wandmatrix", wand_matrix );

    wand_rotation = new OutSlot<Rotation>( "Rotation of wand tracker", Rotation() );
    assert( wand_rotation != 0 );
    wand_rotation->addListener( *this );
    addOutSlot( "wandrotation", wand_rotation );

    wand_position = new OutSlot<Vec3f>( "Position of wand tracker", Vec3f() );
    assert( wand_position != 0 );
    wand_position->addListener( *this );
    addOutSlot( "wandposition", wand_position );

    shm_wand = new dtkSharedMem(6*sizeof(float), "wand" );
    if( !shm_wand )
    {
        std::cerr << "ERROR: dtkInstantIOWandNode cannot open dtkSharedMem file " <<
            "wand" << std::endl;
    }

    joystick_x_axis = new OutSlot<float>( "Joystick x axis", 0.0f );
    assert( joystick_x_axis != 0 );
    joystick_x_axis->addListener( *this );
    addOutSlot( "joystick_x_axis", joystick_x_axis );

    joystick_y_axis = new OutSlot<float>( "Joystick y axis", 0.0f );
    assert( joystick_y_axis != 0 );
    joystick_y_axis->addListener( *this );
    addOutSlot( "joystick_y_axis", joystick_y_axis );

    shm_joystick = new dtkSharedMem(2*sizeof(float), "joystick" );
    if( !shm_joystick )
    {
        std::cerr << "ERROR: dtkInstantIOWandNode cannot open dtkSharedMem file " <<
            "wand" << std::endl;
    }

    button_1 = new OutSlot<bool>( "Button 1", false );
    assert( button_1 != 0 );
    button_1->addListener( *this );
    addOutSlot( "button_1", button_1 );

    button_2 = new OutSlot<bool>( "Button 2", false );
    assert( button_2 != 0 );
    button_2->addListener( *this );
    addOutSlot( "button_2", button_2 );

    button_3 = new OutSlot<bool>( "Button 3", false );
    assert( button_3 != 0 );
    button_3->addListener( *this );
    addOutSlot( "button_3", button_3 );

    button_4 = new OutSlot<bool>( "Button 4", false );
    assert( button_4 != 0 );
    button_4->addListener( *this );
    addOutSlot( "button_4", button_4 );

    button_5 = new OutSlot<bool>( "Button 5", false );
    assert( button_5 != 0 );
    button_5->addListener( *this );
    addOutSlot( "button_5", button_5 );

    button_6 = new OutSlot<bool>( "Button 6", false );
    assert( button_6 != 0 );
    button_6->addListener( *this );
    addOutSlot( "button_6", button_6 );

    button_7 = new OutSlot<bool>( "Button 7", false );
    assert( button_7 != 0 );
    button_7->addListener( *this );
    addOutSlot( "button_7", button_7 );

    button_8 = new OutSlot<bool>( "Button 8", false );
    assert( button_8 != 0 );
    button_8->addListener( *this );
    addOutSlot( "button_8", button_8 );

    shm_buttons = new dtkSharedMem(sizeof(unsigned char), "buttons" );
    if( !shm_wand )
    {
        std::cerr << "ERROR: dtkInstantIOWandNode cannot open dtkSharedMem file "
            << "wand" << std::endl;
    }
}

void dtkInstantIOWandNode::shutdown()
{
    Node::shutdown();

    if( wand_matrix != 0 )
    {
        removeOutSlot( "wandmatrix", wand_matrix );
        delete shm_wand;
        shm_wand = 0;
    }

    if( wand_rotation != 0 )
    {
        removeOutSlot( "wandrotation", wand_rotation );
    }

    if( wand_position != 0 )
    {
        removeOutSlot( "wandposition", wand_position );
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

int dtkInstantIOWandNode::processData()
{
    float old_loc[6] = { NAN, NAN, NAN, NAN, NAN, NAN };
    float old_joystick[2];
    char old_buttons;

    float loc[6];
    float joystick[2];
    char buttons;
    dtkMatrix mat;
    const float scale = 1.524;

    SPEW();

    setState( NODE_RUNNING );
    while( waitThread( 10 ) )
    {
        if( shm_wand->read( loc ) )
        {
            std::cerr <<
                "ERROR: dtkInstantIOWandNode error reading dtkSharedMem( \""
                << "wand" << "\" )" << std::endl;
            return -1;
        }
        if(!IsSameOrSetFloat6(old_loc, loc))
        {
            loc[0] *= scale;
            loc[1] *= -scale;
            loc[2] *= -scale;

            mat.identity();
            mat.rotateHPR(loc[3], loc[4], loc[5]);
            mat.translate(loc[0], loc[1], loc[2]);

            Vec3f pos;
            Rotation rot;

            mat.translate(&pos[0], &pos[2], &pos[1]);
            pos[1] = -pos[1];
            mat.quat(&rot[0], &rot[2], &rot[1], &rot[3]);
            rot[2] = -rot[2];

            Matrix4f tracker_mat;

            tracker_mat.setTransform(pos[0], pos[1], pos[2],
                    rot[0], rot[1], rot[2], rot[3]);

            wand_position->push(pos);
            wand_rotation->push(rot);

            wand_matrix->push(tracker_mat);
        }

        if( shm_joystick->read( joystick ) )
        {
            std::cerr << "ERROR: dtkInstantIOWandNode error "
                "reading dtkSharedMem( \"" << "joystick" << "\" )" << std::endl;
            return -1;
        }

        // Provide dead zone of +/- 0.05 to prevent small errors for joystick x
        if( fabsf( joystick[0] ) < 0.05 )
        {
            joystick[0] = 0.0f;
        }

        // Provide dead zone of +/- 0.05 to prevent small errors for joystick y
        if( fabsf( joystick[1] ) < 0.05 )
        {
            joystick[1] = 0.0f;
        }

        if( joystick[0] != old_joystick[0] || joystick[1] != old_joystick[1] )
        {
            joystick_x_axis->push( joystick[0] );
            joystick_y_axis->push( joystick[1] );

            old_joystick[0] = joystick[0];
            old_joystick[1] = joystick[1];
        }

        if( shm_buttons->read( &buttons ) )
        {
            std::cerr << "ERROR: dtkInstantIOWandNode "
                "error reading dtkSharedMem( \"" <<
                "buttons" << "\" )" << std::endl;
            return -1;
        }
        if( buttons != old_buttons )
        {
            unsigned char i = 1;
            button_1->push( i & buttons );
            i = i << 1;
            button_2->push( i & buttons );
            i = i << 1;
            button_3->push( i & buttons );
            i = i << 1;
            button_4->push( i & buttons );
            i = i << 1;
            button_5->push( i & buttons );
            i = i << 1;
            button_6->push( i & buttons );
            i = i << 1;
            button_7->push( i & buttons );
            i = i << 1;
            button_8->push( i & buttons );

            old_buttons = buttons;
        }
    }

    setState( NODE_SLEEPING );

    return 0;
}

} // namespace InstantIO
