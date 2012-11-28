#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "Controller.hpp"
#include "Protagonist.hpp"

Controller::Controller()
	: active_(false) 
{ }

Controller::~Controller() {}

//void Controller::update_(Protagonist & p){
//	//readWingNormals(/*normals*/);
//    //p->leftWing(normals[0]);
//    //p->rightWing(normals[1]);
//}
