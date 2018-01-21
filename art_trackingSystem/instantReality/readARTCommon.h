// This file is a little redundant now that we use dlsym()
// to find/load these symbols in readARTWand.cpp, but had InstantReality
// not sucked so badly we could have used this file to get access to
// these globals.  We use dlsym() to shared data between instant IO
// C++ modules.

// We have two IOSensors which we connect with pthread condition signals:
// one is in readARTHead.cpp and the other is in readARTWand.cpp.  It's a
// work-around to not being able to figure-out how to ROUTE data across
// namespaces between an InstantReality <Engine> and a <Scene>.


extern pthread_mutex_t art_mutex;
extern pthread_cond_t  art_cond;

//////////////////////////////////////////////////////////////////////////
//        SHARED DATA
//////////////////////////////////////////////////////////////////////////

// art_haveWandPosRot: Do we have new wand matrix/position/rotation data set?
extern bool                art_haveWandPosRot;

// art_haveHead: Do we have new head matrix data?
extern bool                art_haveHead;
// art_haveHand: Do we have new hand matrix (labeled LHT 1 on unit)?
extern bool                art_haveHand;

extern InstantIO::Matrix4f art_headMatrix;
extern InstantIO::Matrix4f art_handMatrix;
extern InstantIO::Matrix4f art_wandMatrix;
extern InstantIO::Vec3f    art_wandPosition;
extern InstantIO::Rotation art_wandRotation;

// The joystick axis and button always will be sent.
extern float    art_wandXAxis;
extern float    art_wandYAxis;
extern uint32_t art_buttons;
