/*
 *  RBCamera.h
 *  golf
 *
 *  Created by Robert Rose on 9/13/08.
 *  Copyright 2008 Bork 3D LLC. All rights reserved.
 *
 */

#ifndef __H_RBCamera
#define __H_RBCamera

#include "RudeGL.h"

class RBCamera {

public:
	
	RBCamera();
	
	virtual void NextFrame(float delta)
	{
	}
	
	virtual void SetView(float aspect, int eye);
	
	void SetPos(const btVector3 &p) { m_pos = p; }
	void SetLookAt(const btVector3 &l) { m_lookAt = l; }
	
	virtual btVector3 & GetPos() { return m_pos; }
	virtual btVector3 & GetLookAt() { return m_lookAt; }
	virtual btVector3 & GetUp() { return m_up; }
	
protected:
	
	btVector3 m_pos;
	btVector3 m_lookAt;
	btVector3 m_up;
	
	float m_halffov;
};


#endif

