/*
 *  RudeSkinnedMesh.h
 *  golf
 *
 *  Created by Robert Rose on 9/20/08.
 *  Copyright 2008 Bork 3D LLC. All rights reserved.
 *
 */

#ifndef __H_RudeSkinnedMesh
#define __H_RudeSkinnedMesh

#include "RudeMesh.h"

class RudeObject;


class RudeSkinnedMesh : public RudeMesh {
	
public:
	
	RudeSkinnedMesh(RudeObject *owner);
	~RudeSkinnedMesh();
	
	virtual int Load(const char *name);
	
	virtual void NextFrame(float delta);
	virtual void Render();

	virtual float GetFrame() { return m_frame; }
	virtual void SetFrame(float f);
	virtual void AnimateTo(float f);
	
	void SetAnimate(bool a) { m_animate = a; }
	
private:

	float m_frame;
	float m_toFrame;
	float m_fps;
	
	bool m_animateTo;
	
	bool m_animate;
	
};


#endif
