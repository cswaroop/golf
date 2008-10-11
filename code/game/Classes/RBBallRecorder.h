/*
 *  RBBallRecorder.h
 *  golf
 *
 *  Created by Robert Rose on 10/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef __H_RBBallRecorder
#define __H_RBBallRecorder


#include "Rude.h"
#include "RBGolfBall.h"

const int kNumBallPositions = 256;

class RBBallRecord {
	
public:
	btVector3 m_position;
	btVector3 m_angVel;
	
};

class RBBallRecorder {
	
public:
	RBBallRecorder();
	~RBBallRecorder();
	
	void SetBall(RBGolfBall *ball) { m_ball = ball; }
	void Reset();
	void NextFrame(float delta, bool record);
	
	void RenderTracers();
	void RenderRecords();
	
private:
	
	
	RBBallRecord m_ballPositions[kNumBallPositions];
	
	RBGolfBall *m_ball;
	
	float m_timer;
	bool m_wrapped;
	int m_curBallPosition;
	int m_tracerTexture;
	
};

#endif
