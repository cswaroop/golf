/*
 *  RBTGame.cpp
 *  golf
 *
 *  Created by Robert Rose on 10/10/08.
 *  Copyright 2008 Bork 3D LLC. All rights reserved.
 *
 */

#include "RBTGame.h"
#include "RudeGL.h"
#include "RudeGLD.h"
#include "RudeTextureManager.h"
#include "RudeText.h"
#include "RudeFile.h"
#include "RudeDebug.h"
#include "RudePerf.h"
#include "RudePhysics.h"
#include "RudeRegistry.h"
#include "RudeTimer.h"
#include "RudeTweaker.h"
#include "RudeSound.h"

#include "btTransform.h"

bool gDebugCamera = false;
RUDE_TWEAK(DebugCamera, kBool, gDebugCamera);
bool gDebugCameraPrev = false;

bool gDebugFinishHole = false;
RUDE_TWEAK(DebugFinishHole, kBool, gDebugFinishHole);

bool gDebugFinishCourse = false;
RUDE_TWEAK(DebugFinishCourse, kBool, gDebugFinishCourse);

bool gDebugResetHole = false;
RUDE_TWEAK(DebugResetHole, kBool, gDebugResetHole);

bool gDebugGuidePosition = false;
RUDE_TWEAK(DebugGuidePosition, kBool, gDebugGuidePosition);

float gDecoDrop = 0.0f;
RUDE_TWEAK(DecoDrop, kFloat, gDecoDrop);

bool gRenderUI = true;
RUDE_TWEAK(RenderUI, kBool, gRenderUI);


const unsigned int kBallDistanceTopColor = 0xFF666666;
const unsigned int kBallDistanceBotColor = 0xFF000000;
const unsigned int kBallDistanceOutlineTopColor = 0xFF00FFFF;
const unsigned int kBallDistanceOutlineBotColor = 0xFF22FFFF;

const unsigned int kBallDistanceTopFireColor = 0xFF666666;
const unsigned int kBallDistanceBotFireColor = 0xFF000000;
const unsigned int kBallDistanceOutlineTopFireColor = 0xFF0000FF;
const unsigned int kBallDistanceOutlineBotFireColor = 0xFF2222FF;

const unsigned int kBallRemainingTopColor = 0xFF666666;
const unsigned int kBallRemainingBotColor = 0xFF000000;
const unsigned int kBallRemainingOutlineTopColor = 0xFFFFFFFF;
const unsigned int kBallRemainingOutlineBotColor = 0xFFFFFFFF;

const unsigned int kParTopColor = 0xFFF69729;
const unsigned int kParBotColor = 0xFF000000;
const unsigned int kParOutlineTopColor = 0xFFFFFFFF;
const unsigned int kParOutlineBotColor = 0xFFFFFFFF;

float gWindForceMultiplier = 0.6f;
RUDE_TWEAK(WindForceMultiplier, kFloat, gWindForceMultiplier);

const float kFollowTimerThreshold = 2.0f;

#define NO_DECO_EDITOR

tRudeButtonAnimKeyframe gSwingButtonAnimData[8] = {
{ 5.0f, 0 },
{ 0.1f, 1 },
{ 0.07f, 2 },
{ 0.05f, 3 },
{ 0.05f, 0 },
{ 0.1f, 1 },
{ 0.07f, 2 },
{ 0.05f, 3 },
};

RBTGame::RBTGame(int holeNum, const char *terrainfile, eCourseTee tee, eCourseHoles holeset, eCourseWind wind, int par, int numPlayers, bool restorestate)
: m_swingYaw(0.0f)
, m_swingHeight(0.0f)
, m_swingCamYaw(0.0f)
, m_moveGuide(false)
, m_moveHeight(false)
, m_curClub(0)
, m_ui(0)
, m_guideIndicatorButton(0)
, m_holeIndicatorButton(0)
, m_botBarBg(0)
, m_nextClubButton(0)
, m_prevClubButton(0)
, m_clubButton(0)
, m_cameraButton(0)
, m_helpButton(0)
, m_terrainButton(0)
, m_holeText(0)
, m_parText(0)
, m_strokeText(0)
, m_remainingDistText(0)
, m_scoreText(0)
, m_shotEncouragementText(0)
, m_clubDistText(0)
, m_powerRangeText(0)
, m_windText(0)
, m_shotDistText(0)
, m_shotPowerText(0)
, m_shotAngleText(0)
, m_guidePowerText(0)
, m_scoreControl(0)
, m_holeHeightText(0)
, m_guideScreenCalc(false)
, m_swingPower(0.0f)
, m_swingAngle(0.0f)
, m_followTimer(0.0f)
, m_stopTimer(0.0f)
, m_encouragementTimer(0.0)
, m_oobTimer(0.0f)
, m_windDir(0.0f)
, m_windSpeed(0.0f)
, m_placementGuidePower(0.0f)
, m_ballShotDist(0.0f)
, m_ballToHoleDist(0.0f)
, m_playedBallDissapointmentSound(false)
, m_landscape(false)
, m_holeNum(holeNum)
, m_dropDecoText(0)
, m_dumpDecoText(0)
{	
	m_result = kResultNone;
	
	RudePhysics::GetInstance()->Init();
		
	m_terrain.Load(terrainfile);
	m_terrain.SetTee(tee);
	
	m_par = par;
	m_numPlayers = numPlayers;
	m_tee = tee;
	m_holeSet = holeset;
	m_wind = wind;
	m_curPlayer = 0;
	
	m_oobTimer = 0.0f;
	
	m_landscape = RGL.GetLandscape();
	
	m_placementGuidePower = 100.0f;
	
	m_playedBallDissapointmentSound = false;
	
	m_ball.Load("ball_1");
	m_ball.SetPosition(btVector3(0,100,0));
	m_ball.SetCurMaterial(kTee);
	
	m_ballRecorder.SetBall(&m_ball);
	
	m_ballGuide.SetBall(&m_ball);
	
	m_golfer.Load("golfer");
	
	m_pin.LoadMesh("pin");
	
	m_skybox.Load("skybox");
	
	m_ballCamera.SetBall(&m_ball);
	m_ballCamera.SetTerrain(&m_terrain);

	m_terrainui.SetTerrain(&m_terrain);

	// Load UI data
	if(RUDE_IPAD)
	{
		m_ui.SetFileRect(RudeRect(0,0,-1,-1));
		m_ui.Load("game_ipad");
	}
	else
	{
		m_ui.SetFileRect(RudeRect(0,0,-1,-1));
		m_ui.Load("game_iphone");
	}
	
	if(gDebugCamera)
		m_curCamera = &m_debugCamera;
	else
		m_curCamera = &m_ballCamera;
	
	m_guideScreenCalc = false;
	
#ifndef NO_DECO_EDITOR
	m_dropDecoText.SetText("DecoDrop");
	m_dropDecoText.SetAlignment(RudeTextControl::kAlignLeft);
	m_dropDecoText.SetFileRect(RudeRect(60,0,90,180));
	
	m_dumpDecoText.SetText("DecoDump");
	m_dumpDecoText.SetAlignment(RudeTextControl::kAlignLeft);
	m_dumpDecoText.SetFileRect(RudeRect(90,0,120,180));
#endif
	
	// score control
	m_scoreControl = m_ui.GetChildControl<RBScoreControl>("scoreControl");
	m_scoreControl->SetActiveHole(holeNum, holeset);
	
	// stroke/status controls
	
	m_holeText = m_ui.GetChildControl<RudeTextControl>("holeText");
	m_holeText->SetFormat(kIntValue, "%d");
	m_holeText->SetValue(((float) (m_holeNum + 1)));
	
	
	m_parText = m_ui.GetChildControl<RudeTextControl>("parText");
	m_parText->SetFormat(kIntValue, "PAR %d");
	
	m_remainingDistText = m_ui.GetChildControl<RudeTextControl>("remainingDistText");
	m_remainingDistText->SetFormat(kIntValue, "%d yds");
	
	m_strokeText = m_ui.GetChildControl<RudeTextControl>("strokeText");
	m_strokeText->SetFormat(kIntValue, "Stroke %d");
	
	m_scoreText = m_ui.GetChildControl<RudeTextControl>("scoreText");
	m_scoreText->SetFormat(kSignedIntValue, "%s");
	
	m_clubDistText = m_ui.GetChildControl<RudeTextControl>("clubDistText");
	m_clubDistText->SetFormat(kIntValue, "%d yds");
	
	m_powerRangeText = m_ui.GetChildControl<RudeTextControl>("powerRangeText");
	
	m_windText = m_ui.GetChildControl<RudeTextControl>("windText");
	m_windText->SetFormat(kIntValue, "%d mph");
	
	m_shotEncouragementText = m_ui.GetChildControl<RudeTextControl>("shotEncouragementText");
	
	m_shotDistText = m_ui.GetChildControl<RudeTextControl>("shotDistText");
	m_shotDistText->SetFormat(kIntValue, "%d yds");
	
	m_shotPowerText = m_ui.GetChildControl<RudeTextControl>("shotPowerText");
	m_shotPowerText->SetFormat(kIntValue, "%d %% Power");
	
	m_shotAngleText = m_ui.GetChildControl<RudeTextControl>("shotAngleText");
	m_shotAngleText->SetFormat(kIntValue, "%d %% Angle");
	
	m_guidePowerText = m_ui.GetChildControl<RudeTextControl>("guidePowerText");
	m_guidePowerText->SetFormat(kIntValue, "%d yds");

	m_holeHeightText.SetText("");
	m_holeHeightText.SetAlignment(RudeTextControl::kAlignLeft);
	m_holeHeightText.SetPosition(0,0);
	m_holeHeightText.SetStyle(kOutlineStyle);
	m_holeHeightText.SetColors(0, 0xFF666666, 0xFF000000);
	m_holeHeightText.SetColors(1, 0xFFFFFFFF, 0xFFFFFFFF);
    m_holeHeightText.SetAdjustDrawRectToEdges(false);
						  
	// swing controls
	
	
	
	m_botBarBg = m_ui.GetChildControl<RudeButtonControl>("botBarBg");
	

	m_swingControl = m_ui.GetChildControl<RBSwingControl>("swingControl");
	m_swingControl->SetGolfer(&m_golfer);
	
	// patch up swing button textures

	m_swingButton = m_ui.GetChildControl<RudeButtonAnimControl>("swingButton");
	
	int numframes = sizeof(gSwingButtonAnimData) / sizeof(tRudeButtonAnimKeyframe);
	for(int i = 0; i < numframes; i++)
	{
		tRudeButtonAnimKeyframe *frame = &gSwingButtonAnimData[i];
		
		if(frame->m_texture == 0)
			frame->m_texture = RudeTextureManager::GetInstance()->LoadTextureFromPNGFile("ui_swing");
		else if(frame->m_texture == 1)
			frame->m_texture = RudeTextureManager::GetInstance()->LoadTextureFromPNGFile("ui_swing_d");
		else if(frame->m_texture == 2)
			frame->m_texture = RudeTextureManager::GetInstance()->LoadTextureFromPNGFile("ui_swing_c");
		else if(frame->m_texture == 3)
			frame->m_texture = RudeTextureManager::GetInstance()->LoadTextureFromPNGFile("ui_swing_b");
	}
	
	m_swingButton->SetAnimData(gSwingButtonAnimData, numframes);
	
	m_moveButton = m_ui.GetChildControl<RudeButtonControl>("moveButton");
	m_menuButton = m_ui.GetChildControl<RudeButtonControl>("menuButton");
	
	m_guideIndicatorButton.SetTexture("guide");
	m_holeIndicatorButton.SetTexture("guide2");
	
	m_swingYaw = 0.0f;
	m_swingCamYaw = 0.0f;
	
	
	m_prevClubButton = m_ui.GetChildControl<RudeButtonControl>("prevClubButton");
	m_clubButton = m_ui.GetChildControl<RudeButtonControl>("clubButton");
	m_nextClubButton = m_ui.GetChildControl<RudeButtonControl>("nextClubButton");
	
	m_helpButton = m_ui.GetChildControl<RudeButtonControl>("helpButton");
	m_cameraButton = m_ui.GetChildControl<RudeButtonControl>("cameraButton");
	m_terrainButton = m_ui.GetChildControl<RudeButtonControl>("terrainButton");

	m_guideAdjust = m_ui.GetChildControl<RudeControl>("guideAdjust");
	m_swingCamAdjust = m_ui.GetChildControl<RudeControl>("swingCamAdjust");

	m_windControl = m_ui.GetChildControl<RBWindControl>("windControl");

	m_terrainSliceControl = m_ui.GetChildControl<RBTerrainSliceControl>("terrainSlice");
	m_terrainSliceControl->SetBallGuide(&m_ballGuide);
	
	if(restorestate)
	{
		int result = LoadState();
		
		if(result < 0)
		{
			m_terrain.FinalizeGuidePoints();
			SetState(kStateTeePosition);
		}
	}
	else
	{
		m_terrain.FinalizeGuidePoints();
		SetState(kStateTeePosition);
	}
	
	m_pin.SetPosition(m_terrain.GetHole());
	
	SetupUI();
}

RBTGame::~RBTGame()
{
	RudePhysics::GetInstance()->Destroy();
}

void RBTGame::SetupUI()
{
	int paroffx = 0;
	if(m_holeNum + 1 >= 10)
		paroffx += 16;
		
	int x, y;
	m_parText->GetPosition(x, y);
	m_parText->SetPosition(x + paroffx, y);

	m_remainingDistText->GetPosition(x, y);
	m_remainingDistText->SetPosition(x + paroffx, y);
	
	Resize();
}

void RBTGame::SaveState()
{
	RudeRegistry *reg = RudeRegistry::GetSingleton();
	
	tRBTGameStateSave save;
	
	save.size = sizeof(save);
	save.state = m_state;
	save.curClub = m_curClub;
	save.windDir = m_windDir;
	save.windVec = m_windVec;
	save.windSpeed = m_windSpeed;
	save.swingPower = m_swingPower;
	save.swingAngle = m_swingAngle;
	save.ballToHoleDist = m_ballToHoleDist;
	save.ball = m_ball.GetPosition();
	save.ballMaterial = m_ball.GetCurMaterial();
	save.hole = m_terrain.GetHole();
	
	reg->SetByte("GOLF", "GS_INGAME_STATE", &save, sizeof(save));
	
	for(int i = 0; i < m_numPlayers; i++)
	{
		RBScoreTracker *tracker = GetScoreTracker(i);
		tracker->SaveState(i);
	}
}

int RBTGame::LoadState()
{
	RudeRegistry *reg = RudeRegistry::GetSingleton();
	
	tRBTGameStateSave load;
	int loadsize = sizeof(load);
	
	if(reg->QueryByte("GOLF", "GS_INGAME_STATE", &load, &loadsize) == 0)
	{
		if(load.size != sizeof(load))
		{
			RUDE_REPORT("Failed to load game state, size mismatch!\n");
			return -1;
		}

		m_state = load.state;
		m_curClub = load.curClub;
		m_windDir = load.windDir;
		m_windVec = load.windVec;
		m_windSpeed = load.windSpeed;
		m_swingPower = load.swingPower;
		m_swingAngle = load.swingAngle;
		m_ballToHoleDist = load.ballToHoleDist;
		m_ball.StickAtPosition(load.ball);
		m_ball.SetCurMaterial(load.ballMaterial);
		m_terrain.SetHole(load.hole);
	
		RestoreState();
		
		
		return 0;
	}
	
	return -1;
}

void RBTGame::RestoreState()
{
	if(m_state == kStatePositionSwing2
	   || m_state == kStatePositionSwing3
	   || m_state == kStateWaitForSwing
	   || m_state == kStateMenu
	   || m_state == kStateExecuteSwing
	   || m_state == kStateRegardBall
	   || m_state == kStateTerrainView)
		m_state = kStatePositionSwing;
	
	if(m_state == kStateHitBall
	   || m_state == kStateFollowBall)
		m_state = kStateHitBall;
	
	m_windText->SetValue(m_windSpeed);
	m_windControl->SetWind(m_windDir, m_windSpeed);
	m_ball.SetWind(m_windVec);
	
	m_terrain.FinalizeGuidePoints();
	
	// update golfer renderable
	RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
	m_golfer.SetSwingType(club->m_type);
	
	// tell terrain to update guide point
	m_terrain.UpdateGuidePoint(m_ball.GetPosition(), club->m_dist * 3.0f);
	
	switch(m_state)
	{
		case kStateTeePosition:
		{
			NextClub(0);
		}
			break;
		case kStatePositionSwing:
		{
			SetState(m_state);
		}
			break;
		case kStateHitBall:
		{
			// animate the golfer to the end of the swing
			m_golfer.SetForwardSwing(1.0f);
			
			// tweak some crap w/ the camera to trick it into positioning properly
			MoveAimCamera(RudeScreenVertex(0,0), RudeScreenVertex(0,0));
			FreshGuide(true);
			m_ballCamera.SetTrackMode(kHitCamera);
			m_ballCamera.NextFrame(1.0f);
		}
			break;
        default:
            break;
	}
}

void RBTGame::SetState(eRBTGameState state)
{
	// If we're at the driving range reset the ball once it stops
	if(m_holeSet == kCourseDrivingRange)
	{
		if(state == kStateRegardBall)
		{
			state = kStateTeePosition;
		}
	}
	
	RUDE_REPORT("RBTGame::SetState %d => %d\n", m_state, state);
	eRBTGameState prevstate = m_state;
	m_state = state;
	
	switch(m_state)
	{
		case kStateTeePosition:
			{
				m_curClub = 0;
				NextClub(0);
			}
			break;
		case kStatePositionSwing:
			{
				m_swingButton->ResetTimer();
				
				m_cameraButton->SetTexture("ui_camera_flag");
				
				m_golfer.SetReady();
				
				m_swingHeight = 0.0f;
				
				if(prevstate == kStatePositionSwing2 ||
					prevstate == kStateTerrainView)
				{
					m_ballCamera.SetHeight(10.0f);
				}
				else if(prevstate == kStateExecuteSwing)
				{
				}
				else
				{
					m_swingYaw = 0.0f;
					
					FreshShotEncouragement();
					
					AutoSelectClub();
				}
				
				m_terrain.SetEnablePuttingGreen(true);
				
				FreshGuide(true);
				SetHoleHeightText();
			}
			break;
		case kStatePositionSwing2:
			{
				m_placementGuidePosition = m_guidePosition;
				m_encouragementTimer = 0.0f;
				
				m_cameraButton->SetTexture("ui_camera");
				
				if(prevstate != kStatePositionSwing3)
				{
					m_swingCamYaw = 0.0f;
					m_swingHeight = 50.0f;
				}
				
				m_terrain.SetEnablePuttingGreen(true);
				
				FreshGuide();
				MoveAimCamera(RudeScreenVertex(0,0), RudeScreenVertex(0,0));
			}
			break;
		case kStatePositionSwing3:
			{
				m_placementGuidePosition = m_guidePosition;
			}
			break;
		case kStateMenu:
			{
				m_menu.Reset(m_holeNum, m_holeSet);
			}
			break;
		case kStateExecuteSwing:
			{
				m_encouragementTimer = 0.0f;
				
				m_terrain.SetEnablePuttingGreen(false);
				m_swingControl->Reset();
				m_swingHeight = 0.0f;
				FreshGuide();
			}
			break;
		case kStateWaitForSwing:
			{
			}
			break;
		case kStateHitBall:
			{ 
				float powermultiplier = 1.0f;
				
				RBTerrainMaterialInfo &material = m_terrain.GetMaterialInfo(m_ball.GetCurMaterial());
				int powerrange = material.m_penalty_power_max - material.m_penalty_power_min;
				
				if(powerrange > 0)
				{
					int powerpenalty = (rand() % powerrange) + material.m_penalty_power_min;
					powermultiplier = (float) powerpenalty;
					powermultiplier /= 100.0f;
				}
				
				m_swingPower = m_swingControl->GetPower() * powermultiplier;
				m_swingAngle = m_swingControl->GetAngle() * material.m_penalty_angle;
				
				if(m_swingAngle < -1.0f)
					m_swingAngle = -1.0f;
				else if(m_swingAngle > 1.0f)
					m_swingAngle = 1.0f;
				
				m_ballRecorder.Reset();
			}
			break;
		case kStateFollowBall:
			{
				m_oobTimer = 0.0f;
				m_stopTimer = 0.0f;
				m_followTimer = 0.0f;
				m_ballCamera.SetTrackMode(kAfterShotCamera);
			}
			break;
		case kStateRegardBall:
			{
				GetScoreTracker(m_curPlayer)->AddStrokes(m_holeNum, 1);
				
				m_ballCamera.SetDesiredHeight(5.0f);
				m_ballCamera.ResetGuide(m_terrain.GetHole());
				m_ballCamera.SetTrackMode(kRegardCamera);
				
				eRBTerrainMaterial material = m_ball.GetCurMaterial();
				
				switch(material)
				{
					case kRough:
						m_shotEncouragementText->SetText("Rough");
						break;
					case kFairwayFringe:
					case kFairway:
						m_shotEncouragementText->SetText("Fairway");
						break;
					case kGreen:
						m_shotEncouragementText->SetText("Green");
						break;
					case kGreenFringe:
						m_shotEncouragementText->SetText("Fringe");
						break;
					case kSandtrap:
						m_shotEncouragementText->SetText("Bunker");
						break;
					default:
						m_shotEncouragementText->SetText("");
						break;
				}
				
				m_encouragementTimer = 3.0f;

				SetHoleHeightText();
				
			}
			break;
		case kStateBallOutOfBounds:
			{
				// one stroke penalty
				GetScoreTracker(m_curPlayer)->AddStrokes(m_holeNum, 1);
				
				StickBallInBounds();
				
				SetState(kStateRegardBall);
			}
			break;
		case kStateBallInHole:
			{
				GetScoreTracker(m_curPlayer)->AddStrokes(m_holeNum, 1);
				
				RudeSound::GetInstance()->PlayWave(kSoundBallInHole);
				RudeSound::GetInstance()->BgmVol(0.75f);
				
				int strokes = GetScoreTracker(m_curPlayer)->GetNumStrokes(m_holeNum);
				int par = GetScoreTracker(m_curPlayer)->GetPar(m_holeNum);
				
				if(strokes < par)
					RudeSound::GetInstance()->PlayWave(kSoundCheer);
				else if(strokes == par)
					RudeSound::GetInstance()->PlayWave(kSoundClaps);
				
				m_ballCamera.SetDesiredHeight(5.0f);
				m_ballCamera.SetGuide(m_terrain.GetHole());
				m_ballCamera.SetTrackMode(kRegardCamera);
				
			}
			break;
		case kStateTerrainView:
			{
				m_terrainui.StartDisplay();
				m_terrainui.SetPositions(m_ball.GetPosition(), m_terrain.GetHole(), m_guidePosition);
			}
			break;
	}
	
	SaveState();
}

void RBTGame::StateTeePosition(float delta)
{
	btVector3 tee = m_terrain.GetTeeBox();
	m_ball.StickAtPosition(tee);
	m_ball.SetCurMaterial(kTee);

	m_terrain.SetBallInHole(false);
	
	btVector3 ballToHole = m_terrain.GetHole() - tee;
	ballToHole.setY(0.0f);
	float yardage = ballToHole.length() / 3.0f;
	
	m_curClub = RBGolfClub::AutoSelectClub(yardage, m_ball.GetCurMaterial());
	
	
	// figure out wind speed
	
	int windspeed = 0;
	
	switch(m_wind)
	{
		case kCourseNoWind:
			windspeed = 0;
			break;
		case kCourseLowWind:
		{
			windspeed = rand() % 4 + 1;
		}
			break;
		case kCourseHighWind:
		{
			windspeed = rand() % 10 + 4;
		}
			break;
	}
	
	m_windText->SetValue((float) windspeed);
	
	m_windSpeed = (float) windspeed;
	
	btVector3 windx(0,0,-gWindForceMultiplier);
	
	float winddir = (float) (rand() % 360);
	m_windDir = (winddir / 180.0f) * 3.1415926f;
	
	btMatrix3x3 rmat;
	rmat.setEulerYPR(-m_windDir, 0.0f, 0.0f);
	
	m_windVec = rmat * windx;
	m_windVec *= m_windSpeed;
	
	m_windControl->SetWind(m_windDir, (float) windspeed);
	m_ball.SetWind(m_windVec);
	
	RUDE_REPORT("Wind: dir=%f speed=%f vec=(%f %f %f)\n", winddir, m_windSpeed, m_windVec.x(), m_windVec.y(), m_windVec.z());
	
	SetState(kStatePositionSwing);
}

void RBTGame::StatePositionSwing(float delta)
{
	m_encouragementTimer -= delta;
	
	float alpha = m_encouragementTimer;
	if(alpha > 1.0f)
		alpha = 1.0f;
	m_shotEncouragementText->SetAlpha(alpha);
	
	if(RUDE_IPAD)
	{
		if(m_terrain.GetPutting())
			m_ballGuide.NextFrame(delta);
	}

}

void RBTGame::StatePositionSwing2(float delta)
{
	m_encouragementTimer = 0.0f;
	
	m_ballGuide.NextFrame(delta);
}

void RBTGame::StatePositionSwing3(float delta)
{
	m_ballGuide.NextFrame(delta);
}

void RBTGame::StateHitBall(float delta)
{
	HitBall();
	
	SetState(kStateFollowBall);
	
}

void RBTGame::StateFollowBall(float delta)
{	
	m_ballRecorder.NextFrame(delta, true);
	
	btVector3 ball = m_ball.GetPosition();
	
	// check to make sure the ball is in bounds
	
	m_oobTimer += delta;
	
	if(m_oobTimer > 1.0f)
	{
		m_oobTimer = 0.0f;
		
		bool inbounds = !m_terrain.IsOutOfBounds(ball);
		
		if(inbounds)
		{
			m_ballLastInBoundsPosition = ball;
		}
		else
		{
			if(m_terrain.IsOutOfBoundsAndGone(ball))
			{
				SetState(kStateBallOutOfBounds);
				return;
			}
		}
	}
	
	// increment the follow timer and switch to follow cam
	
	m_followTimer += delta;
	
	if(m_followTimer > kFollowTimerThreshold)
	{
		// enable physics precision
		RudePhysics::GetInstance()->SetPrecise(true);
		
		
		// project ball forward based on velocity
		const float kBallForwardTime = 2.0f;
		btVector3 ballvel = m_ball.GetLinearVelocity();
		ballvel.setY(0);
		
		btVector3 futureball = ball + ballvel * kBallForwardTime;
		
		btVector3 placement = m_terrain.GetCameraPlacement(futureball);
		
		m_ballCamera.ResetGuide(placement);
		m_ballCamera.SetTrackMode(kPlacementCamera);
		
		m_golfer.SetCasual();
		
		m_followTimer = -100.0f;
	}
	else
	{
	}
	
	
	// update yardage calculation
	
	btVector3 dist = ball - m_dBall;
	dist.setY(0.0f);
	float df = dist.length();
	m_ballShotDist = df / 3.0f;
	
	btVector3 ballToHole = m_terrain.GetHole() - ball;
	ballToHole.setY(0.0f);
	m_ballToHoleDist = ballToHole.length() / 3.0f;
	
	
	// determine if the ball is stopped
	
	if(m_ball.GetStopped())
	{
		if(m_terrain.GetBallInHole())
		{
			SetState(kStateBallInHole);
			return;
		}
		
		if(!m_playedBallDissapointmentSound)
		{
			if(m_ballToHoleDist < 2.0f)
			{
				RudeSound::GetInstance()->PlayWave(kSoundMissedPutt);
				m_playedBallDissapointmentSound = true;
			}
		}
		
		const float kBallStoppedObservationTime = 2.0f;
		
		m_stopTimer += delta;
		
		if(m_stopTimer > kBallStoppedObservationTime)
		{
			SetState(kStateRegardBall);
		}
	}
}

void RBTGame::StateRegardBall(float delta)
{
	m_encouragementTimer -= delta;
	
	float alpha = m_encouragementTimer;
	if(alpha > 1.0f)
		alpha = 1.0f;
	m_shotEncouragementText->SetAlpha(alpha);
}

void RBTGame::FreshShotEncouragement()
{
	if(m_ball.GetCurMaterial() == kGreen)
	{
		RBScoreTracker *tracker = GetScoreTracker(m_curPlayer);
		int strokes = tracker->GetNumStrokes(m_holeNum) + 1;
		int scoreIfShotMade = strokes - m_par;
		
		const float kEncouragementTimer = 3.0f;
		
		if(scoreIfShotMade == -2)
		{
			m_shotEncouragementText->SetText("For Eagle!");
			m_encouragementTimer = kEncouragementTimer;
		}
		else if(scoreIfShotMade == -1)
		{
			m_shotEncouragementText->SetText("For Birdie!");
			m_encouragementTimer = kEncouragementTimer;
		}
		else if(scoreIfShotMade == 0)
		{
			m_shotEncouragementText->SetText("For Par");
			m_encouragementTimer = kEncouragementTimer;
		}
		else if(scoreIfShotMade == 1)
		{
			m_shotEncouragementText->SetText("For Bogey");
			m_encouragementTimer = kEncouragementTimer;
		}
		else
			m_shotEncouragementText->SetText("");
	}
	
	
}

void RBTGame::AutoSelectClub()
{
	btVector3 ballToHole = m_terrain.GetHole() - m_ball.GetPosition();
	ballToHole.setY(0.0f);
	float yardage = ballToHole.length() / 3.0f;
	
	m_curClub = RBGolfClub::AutoSelectClub(yardage, m_ball.GetCurMaterial());
	
	RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
	
	// update golfer renderable
	m_golfer.SetSwingType(club->m_type);
	
	// tell terrain to update guide point
	m_terrain.UpdateGuidePoint(m_ball.GetPosition(), club->m_dist * 3.0f);
	
	// If putting, render the putting terrain
	if(club->m_type == kClubPutter)
	{
		m_terrain.SetPutting(true);
		m_swingControl->SetNoSwingCommentary(true);
	}
	else
	{
		m_terrain.SetPutting(false);
	}
	
	// Play the putting background music all the time
	RudeSound::GetInstance()->PlaySong(kBGMPutting);
	RudeSound::GetInstance()->BgmVol(1.0f);
	
	NextClub(0);
}

void RBTGame::NextClub(int n)
{
	if(n > 0)
		m_curClub = RBGolfClub::NextClub(m_curClub, m_ball.GetCurMaterial());
	if(n < 0)
		m_curClub = RBGolfClub::PrevClub(m_curClub, m_ball.GetCurMaterial());
	
	RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
	
	m_clubButton->SetTexture(club->m_textureName);
	
	m_golfer.SetSwingType(club->m_type);
	
	m_placementGuidePower = club->m_dist;
	
	RBTerrainMaterialInfo &material = m_terrain.GetMaterialInfo(m_ball.GetCurMaterial());
	
	if(material.m_penalty_power_min == 100)
	{
		m_powerRangeText->SetText("100%");
	}
	else
	{
		char str[64];
		snprintf(str, 64, "%d~%d %%", material.m_penalty_power_min, material.m_penalty_power_max);
		
		m_powerRangeText->SetText(str);
	}
	
	if(club->m_options & kFirePower)
	{
		m_clubDistText->SetColors(0, kBallDistanceTopFireColor, kBallDistanceBotFireColor);
		m_clubDistText->SetColors(1, kBallDistanceOutlineTopFireColor, kBallDistanceOutlineBotFireColor);
	}
	else
	{
		m_clubDistText->SetColors(0, kBallDistanceTopColor, kBallDistanceBotColor);
		m_clubDistText->SetColors(1, kBallDistanceOutlineTopColor, kBallDistanceOutlineBotColor);
	}
	
	FreshGuide();
}

void RBTGame::MovePosition(const RudeScreenVertex &p, const RudeScreenVertex &dist)
{
	//printf("move %d %d\n", p.m_x, p.m_y);
	
#if RUDE_IPAD == 1
	const float kYawDamping = 0.001f;
#else
	const float kYawDamping = 0.002f;
#endif
	
	RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
	m_placementGuidePower = club->m_dist;
	
	if(!m_moveHeight && dist.m_x > 30)
	{
		m_moveGuide = true;
	}
	else if(!m_moveGuide && dist.m_y > 30)
	{
		m_moveHeight = true;
	}
		
		
	if(m_moveGuide)
	{
		float dx = (float) p.m_x;
		MoveSwingYaw(dx * kYawDamping);
	}
	
	if(m_moveHeight)
	{
        const float kHeightDamping = 0.004f;
		
        float dy = (float) p.m_y;
		MoveSwingHeight(-dy * kHeightDamping);
	}
}

void RBTGame::MoveSwingYaw(float dx)
{
    m_swingYaw += dx;
    FreshGuide();
}

void RBTGame::MoveSwingHeight(float dy)
{
    const float kMaxHeight = 1.0f;
    
    m_swingHeight += dy;
    if(m_swingHeight < 0.0f)
        m_swingHeight = 0.0f;
    if(m_swingHeight > kMaxHeight)
        m_swingHeight = kMaxHeight;
    
    m_ballCamera.SetDesiredHeight(m_swingHeight);
}

void RBTGame::MoveAimCamera(const RudeScreenVertex &p, const RudeScreenVertex &dist)
{
	//printf("move %d %d\n", p.m_x, p.m_y);
	
#if RUDE_IPAD == 1
	const float kYawDamping = 0.003f;
	const float kHeightDamping = 0.002f;
#else
	const float kYawDamping = 0.006f;
	const float kHeightDamping = 0.004f;
#endif

	if(dist.m_x > 30)
	{
		float dx = (float) p.m_x;
		MoveAimYaw(dx * kYawDamping);
	}
	
	float dy = (float) p.m_y;
	MoveAimHeight(-dy * kHeightDamping);
}

void RBTGame::MoveAimYaw(float dx)
{
    m_swingCamYaw += dx;
    m_ballCamera.SetYaw(m_swingCamYaw);   
}

void RBTGame::MoveAimHeight(float dy)
{
    const float kMaxHeight = 1.0f;
    
    m_swingHeight += dy;
	if(m_swingHeight < 0.0f)
		m_swingHeight = 0.0f;
	if(m_swingHeight > kMaxHeight)
		m_swingHeight = kMaxHeight;
	
	m_ballCamera.SetDesiredHeight(m_swingHeight);
}

void RBTGame::StickBallInBounds()
{
	// assumes the ball is just barely out of bounds!
	
	const float kTestLen = 10.0f;
	const float kTestIncrement = 0.5f;
	
	float scores[8];
	float bestscore = 0.0f;
	int bestscorer = -1;
	btVector3 bestpos;
	
	for(int i = 0; i < 8; i++)
	{
		scores[i] = 0.0f;
		
		btVector3 v(1.0f, 0.0f, 0.0f);
		
		btMatrix3x3 rotmat;
		rotmat.setEulerYPR(i * 0.25f * 3.1415926f, 0.0f, 0.0f);
		
		v = rotmat * v;
		
		for(float f = kTestIncrement; f < kTestLen; f += kTestIncrement)
		{
			btVector3 testpos = m_ballLastInBoundsPosition + v * f;
			bool inbounds = !m_terrain.IsOutOfBounds(testpos);
		
			if(inbounds)
				scores[i] += 1.0f;
			
			if(scores[i] > bestscore)
			{
				bestscore = scores[i];
				bestscorer = i;
				
				bestpos = testpos;
			}
		}
	}
	
	if(bestscorer < 0)
	{
		m_ball.StickAtPosition(m_ballLastInBoundsPosition);
		return;
	}
	
	m_ball.StickAtPosition(bestpos);
	
}

void RBTGame::FreshGuide(bool firstTime)
{
	btVector3 ball = m_ball.GetPosition();
	btVector3 guide = m_terrain.GetGuidePoint();
	btVector3 aimvec = guide - ball;
	
	btMatrix3x3 mat;
	mat.setEulerYPR(m_swingYaw, 0.0f, 0.0f);
	btVector3 newGuide = mat * aimvec;
	newGuide.setY(0);

	if(newGuide.length2() < 0.0001f)
		newGuide = btVector3(0,0,1);

	newGuide.normalize();
	newGuide *= RBGolfClub::GetClub(m_curClub)->m_dist * 3.0f;
	newGuide *= m_placementGuidePower / RBGolfClub::GetClub(m_curClub)->m_dist;
	newGuide += ball;
	//btVector3 newGuide = btVector3(0,0,-100) + ball;

	m_golfer.SetPosition(ball, newGuide);
	
	
	RUDE_PERF_START(kPerfFreshGuide);
	
	m_ballGuide.SetGuide(newGuide);
	
	RUDE_PERF_STOP(kPerfFreshGuide);
	
	newGuide = m_ballGuide.GetLastGuidePoint();
	
	if(firstTime)
		m_ballCamera.ResetGuide(newGuide);
	
	
	
	m_ballCamera.SetGuide(newGuide);
	m_ballCamera.SetDesiredHeight(m_swingHeight);
	
	
	
	if(m_state == kStatePositionSwing2)
	{
		m_ballCamera.SetTrackMode(kAimCamera);
	}
	else
	{
		RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
		
		if(club->m_type == kClubPutter)
			m_ballCamera.SetTrackMode(kPuttCamera);
		else
			m_ballCamera.SetTrackMode(kHitCamera);
	}
	
	m_dBall = ball;
	m_guidePosition = newGuide;
	m_placementGuidePosition = m_guidePosition;
	
	m_terrainSliceControl->SetCoursePositions(m_dBall, m_terrain.GetHole(), m_guidePosition);
	
#ifndef NO_DECO_EDITOR
	RUDE_REPORT("Pointer is at %f %f %f\n", m_placementGuidePosition.x(), m_placementGuidePosition.y(), m_placementGuidePosition.z());
#endif
}

void RBTGame::HitBall()
{

	
	const float kMaxAngleModifier = 2.5f;
	float angleModifier = m_swingAngle * (kMaxAngleModifier / 180.0f) * 3.1415926f;
	
	RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
	
	RudeSound::GetInstance()->PlayWave(club->m_swingsound);
	
	if(club->m_type == kClubPutter)
		RudePhysics::GetInstance()->SetPrecise(true);
	else
		RudePhysics::GetInstance()->SetPrecise(false);
	
	float swingAccuracyPenalty = 0.0f;
	
	if(club->m_options & kFirePower)
	{
		int ac = (rand() % 6) - 3;
		swingAccuracyPenalty = (float) ac;
		swingAccuracyPenalty = swingAccuracyPenalty / 180.0f * 3.1415926f;
	}
	
	
	float loft = club->m_loft;
	loft = (loft / 180.0f) * 3.1415926f;
	
	btVector3 ball = m_ball.GetPosition();
	btVector3 guide = m_terrain.GetGuidePoint();
	btVector3 aimvec = guide - ball;
	aimvec.setY(0);
	aimvec.normalize();
	
	btMatrix3x3 guidemat;
	guidemat.setEulerYPR(m_swingYaw + angleModifier + swingAccuracyPenalty, 0.0f, 0.0f);
	aimvec = guidemat * aimvec;
	
	aimvec.setY(tan(loft));
	aimvec.normalize();
	
	//aimvec = btVector3(0,0.9,-0.1);
	//aimvec.normalize();
	
	btVector3 linvel = aimvec * club->m_power * m_swingPower;
	
	
	btVector3 upvec(0,1,0);
	btVector3 rightvec = aimvec.cross(upvec);
	
	const float kMaxSliceModifier = 20.0f;
	
	btVector3 spinForce = rightvec * m_swingAngle * kMaxSliceModifier;
	
	//printf("spinForce = %f %f %f\n", spinForce.x(), spinForce.y(), spinForce.z());
	
	m_ballLastInBoundsPosition = ball;
	
	m_ball.HitBall(linvel, spinForce);
	
}


void RBTGame::AdjustGuide()
{
	if(!m_guideScreenCalc)
		return;
	
	btVector3 ball = m_ball.GetPosition();
	btVector3 guide = m_terrain.GetGuidePoint();
	btVector3 aimvec = guide - ball;
	
	//printf("screen point: %d %d\n", m_guideScreenPoint.m_x, m_guideScreenPoint.m_y);
	
	btVector3 screenp((float) m_guideScreenPoint.m_x, (float) m_guideScreenPoint.m_y, 0.0f);
	
	btVector3 worldp = RGL.InverseProject(screenp);
	
	btVector3 eyep = RGL.GetEye();
	
	btVector3 result;
	bool found = m_terrain.CastToTerrain(eyep, worldp, result);
	
	RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
	
	if(found)
	{
		btVector3 newaimvec = result - ball;
		newaimvec.setY(0.0f);
		float distance = newaimvec.length() / 3.0f;
		newaimvec.normalize();
		
		aimvec.setY(0.0f);
		aimvec.normalize();
		
		btVector3 yup(0,1,0);
		btVector3 xdir = aimvec.cross(yup);
		
		float inx = xdir.dot(newaimvec);
		
		float angle = aimvec.dot(newaimvec);

		if(angle > 1.0f)
			angle = 1.0f;
		
		m_swingYaw = acos(angle);
		
		if(inx < 0.0f)
			m_swingYaw *= -1.0f;
		
		//printf("angle %f yaw %f\n", angle, m_swingYaw);
		
		//FreshGuide();
		
		//m_ballGuide.SetGuide(result);
		
		m_placementGuidePosition = result;
	
		m_placementGuidePower = distance;
		
		if(m_placementGuidePower < 1.0f)
			m_placementGuidePower = 1.0f;
		if(m_placementGuidePower > 400.0f)
			m_placementGuidePower = 400.0f;
	}
	else
	{
		m_placementGuidePower = club->m_dist;
	}
	
	//printf("guide %f %f %f\n", m_guidePosition.x(), m_guidePosition.y(), m_guidePosition.z());
	
	
	m_guideScreenCalc = false;
}

void RBTGame::SetHoleHeightText()
{
	char text[64];

	// Adjust hole height indicator
	float holeHeight = m_terrain.GetHole().y() - m_ball.GetPosition().y();
	int heightFeet = int(holeHeight);
	int heightYards = heightFeet / 3;
	int heightInches = (int) (holeHeight * 12.0f);

	if(heightYards >= 3)
	{
		snprintf(text, 64, "+%d yds", heightYards);
	}
	else if(heightYards <= -3)
	{
		snprintf(text, 64, "%d yds", heightYards);
	}
	else if(heightFeet >= 1)
	{
		snprintf(text, 64, "+%d'", heightFeet);
	}
	else if(heightFeet <= -1)
	{
		snprintf(text, 64, "%d'", heightFeet);
	}
	else if(heightInches > 1)
	{
		snprintf(text, 64, "+%d\"", heightInches);
	}
	else if(heightInches < -1)
	{
		snprintf(text, 64, "%d\"", heightInches);
	}
	else
	{
		text[0] = '\0';
	}

	m_holeHeightText.SetText(text);
}

void RBTGame::NextFrame(float delta)
{
	RUDE_PERF_START(kPerfRBTGameNextFrame);
	
	if(RGL.GetLandscape() != m_landscape)
	{
		m_landscape = RGL.GetLandscape();
		SetupUI();
	}
	
	if(gDebugCamera != gDebugCameraPrev)
	{
		gDebugCameraPrev = gDebugCamera;
		
		if(gDebugCamera)
		{
			m_debugCamera.SetPos(m_ball.GetPosition() + btVector3(0,10,0));
			m_curCamera = &m_debugCamera;
		}
		else
			m_curCamera = &m_ballCamera;
	}
	
	if(gDebugFinishHole || gDebugFinishCourse)
	{
		m_terrain.SetBallInHole(true);
		SetState(kStateBallInHole);
		gDebugFinishHole = false;
	}
	
	if(gDebugFinishCourse)
	{
		m_terrain.SetBallInHole(true);
		SetState(kStateBallInHole);
		m_result = kResultComplete;
		m_done = true;
	}
		
	if(gDebugResetHole)
	{
		SetState(kStateTeePosition);
		gDebugResetHole = false;
	}
	
#ifndef NO_DECO_EDITOR
	if(gDecoDrop > 0.0f)
	{
		m_terrain.DropDecorator(m_placementGuidePosition, gDecoDrop);
		gDecoDrop = 0.0f;
	}
#endif
	
	
	RUDE_PERF_START(kPerfPhysics);
	
	switch(m_state)
	{
		case kStateTeePosition:
		case kStateWaitForSwing:
		case kStateHitBall:
		case kStateFollowBall:
		case kStateRegardBall:
			RudePhysics::GetInstance()->NextFrame(delta);
			break;
        default:
            break;
	}
	
	
	RUDE_PERF_STOP(kPerfPhysics);
	
	m_help.NextFrame(delta);
	
	switch(m_state)
	{
		case kStateTeePosition:
		{
			StateTeePosition(delta);
			
			btVector3 ball = m_ball.GetPosition();
			btVector3 ballToHole = m_terrain.GetHole() - ball;
			ballToHole.setY(0.0f);
			m_ballToHoleDist = ballToHole.length() / 3.0f;
		}
			break;
		case kStatePositionSwing:
			StatePositionSwing(delta);
			m_ballRecorder.NextFrame(delta, false);
			m_swingButton->NextFrame(delta);
			break;
		case kStatePositionSwing2:
			StatePositionSwing2(delta);
			m_ballRecorder.NextFrame(delta, false);
			break;
		case kStatePositionSwing3:
			StatePositionSwing3(delta);
			break;
		case kStateMenu:
			m_menu.NextFrame(delta);
			
			if(m_menu.Done())
			{
				if(m_menu.GetResult() == kMenuResume)
					SetState(kStatePositionSwing);
				else if(m_menu.GetResult() == kMenuQuit)
				{
					m_result = kResultQuit;
					m_done = true;
				}
					
			}
			
			break;
			
		case kStateExecuteSwing:
			
			m_swingControl->NextFrame(delta);
			
			if(m_swingControl->WillSwing())
				SetState(kStateWaitForSwing);
			
			break;
		case kStateWaitForSwing:
		{
			m_swingControl->NextFrame(delta);
			
			if(m_golfer.HasSwung())
				SetState(kStateHitBall);
		}
			break;
		case kStateHitBall:
		{
			m_swingControl->NextFrame(delta);
			
			StateHitBall(delta);
			
			//btVector3 ball = m_ball.GetPosition();
			//printf("ball: %f %f %f\n", ball.x(), ball.y(), ball.z());
		}
			break;
		case kStateFollowBall:
		{
			m_swingControl->NextFrame(delta);
			
			StateFollowBall(delta);
		}
		
			break;
			
		case kStateRegardBall:
		{
			StateRegardBall(delta);
			m_ballRecorder.NextFrame(delta, false);
		}
			break;

		case kStateTerrainView:
		{
			m_terrainui.NextFrame(delta);
		}
			break;
			
		default:
			break;
	}


	//RGLD.DebugDrawLine(m_guidePosition, m_guidePosition + btVector3(0,10,0));
	
	m_golfer.NextFrame(delta);
	m_windControl->NextFrame(delta);
	m_terrainSliceControl->NextFrame(delta);
	
	m_ball.NextFrame(delta);
	m_curCamera->NextFrame(delta);
	
	
	RUDE_PERF_STOP(kPerfRBTGameNextFrame);
}

void RBTGame::RenderCalcOrthoDrawPositions()
{
	switch(m_state)
	{
		case kStatePositionSwing:
		case kStatePositionSwing2:
		case kStatePositionSwing3:
			{

				m_guidePositionScreenSpace = RGL.Project(m_guidePosition);
				m_holePositionScreenSpace = RGL.Project(m_terrain.GetHole());
				
				if(m_state == kStatePositionSwing)
				{
					//if(RGL.GetLandscape())
					//	m_guidePositionScreenSpace.setX(240.0f);
					//else
					//	m_guidePositionScreenSpace.setX(160.0f);
				}
				
				if(m_state == kStatePositionSwing3)
				{
					m_placementGuidePositionScreenSpace = RGL.Project(m_placementGuidePosition);
					
					if(gDebugGuidePosition)
					{
						RUDE_REPORT("Placement Guide: %f %f %f\n", m_placementGuidePosition.x(), m_placementGuidePosition.y(), m_placementGuidePosition.z());
					}
				}
				
				
			}
			break;
        default:
            break;
	}
}

void RBTGame::RenderGuide(float aspect)
{
	const int kGuideSize = 32;

#if RUDE_IPAD == 1
	const int kHoleIndicatorOffset = 30;
	const int kHoleIndicatorOverlapOffset = -50;
#else
	const int kHoleIndicatorOffset = 10;
	const int kHoleIndicatorOverlapOffset = -40;
#endif
	
	// Set hole indicator position
	
	RudeRect holeRect(
		(int) m_holePositionScreenSpace.y() - kGuideSize - kHoleIndicatorOffset,
		(int) m_holePositionScreenSpace.x() - kGuideSize,
		(int) m_holePositionScreenSpace.y() + kGuideSize - kHoleIndicatorOffset,
		(int) m_holePositionScreenSpace.x() + kGuideSize
		);
	m_holeIndicatorButton.SetDrawRect(holeRect);
	
	m_holeHeightText.SetPosition((int) m_holePositionScreenSpace.x() + 10, (int) m_holePositionScreenSpace.y() - kHoleIndicatorOffset - 9);
	RudeRect holeTextRect = m_holeHeightText.GetDrawRect();
	holeTextRect.m_right += 80;
	
	
	if(m_state != kStatePositionSwing3)
	{
		RudeRect guideRect(
				   (int) m_guidePositionScreenSpace.y() - kGuideSize,
				   (int) m_guidePositionScreenSpace.x() - kGuideSize,
				   (int) m_guidePositionScreenSpace.y() + kGuideSize,
				   (int) m_guidePositionScreenSpace.x() + kGuideSize
				   );
		
		m_guideIndicatorButton.SetDrawRect(guideRect);
	}

	RudeRect guidePowerTextRect;
	
	if(m_state == kStatePositionSwing3)
	{

		RudeRect r(
				   (int) m_placementGuidePositionScreenSpace.y() - kGuideSize,
				   (int) m_placementGuidePositionScreenSpace.x() - kGuideSize,
				   (int) m_placementGuidePositionScreenSpace.y() + kGuideSize,
				   (int) m_placementGuidePositionScreenSpace.x() + kGuideSize
				   );
		
		m_guideIndicatorButton.SetDrawRect(r);

		const int kTextSize = 16;
		const int kTextOffset = -58;
		guidePowerTextRect = RudeRect(
				   (int) m_placementGuidePositionScreenSpace.y() - kTextSize + kTextOffset,
				   (int) m_placementGuidePositionScreenSpace.x() - kTextSize * 2,
				   (int) m_placementGuidePositionScreenSpace.y() + kTextSize + kTextOffset,
				   (int) m_placementGuidePositionScreenSpace.x() + kTextSize * 2
				   );
		
		m_guidePowerText->SetDrawRect(guidePowerTextRect);
			
	}
	else
	{
		const int kTextSize = 16;
		const int kTextOffset = -38;
		guidePowerTextRect = RudeRect(
			(int) m_guidePositionScreenSpace.y() - kTextSize + kTextOffset,
			(int) m_guidePositionScreenSpace.x() - kTextSize * 2,
			(int) m_guidePositionScreenSpace.y() + kTextSize + kTextOffset,
			(int) m_guidePositionScreenSpace.x() + kTextSize * 2
			);
		
		m_guidePowerText->SetDrawRect(guidePowerTextRect);
	}

	if(guidePowerTextRect.Overlaps(holeTextRect))
	{
		holeRect += RudeScreenVertex(0,kHoleIndicatorOverlapOffset);
		m_holeIndicatorButton.SetDrawRect(holeRect);
		m_holeHeightText.SetPosition((int) m_holePositionScreenSpace.x() + 10,
			(int) m_holePositionScreenSpace.y() - kHoleIndicatorOffset - 9 + kHoleIndicatorOverlapOffset);
	}

	m_holeIndicatorButton.Render();
	m_guideIndicatorButton.Render();
	
	m_holeHeightText.Render();

	if(m_state == kStatePositionSwing2 ||
		m_state == kStatePositionSwing3)
	{
		m_guidePowerText->SetValue(m_placementGuidePower);
		m_guidePowerText->Render();
	}



}



void RBTGame::RenderShotInfo(bool showShotDistance, bool showClubInfo)
{
	
	if(showShotDistance)
	{
		
		m_shotDistText->SetValue(m_ballShotDist);
		m_shotPowerText->SetValue(m_swingPower * 100.0f);
		
		if(m_swingAngle > 0.0f)
			m_shotAngleText->SetFormat(kIntValue, "%d %% Slice");
		else
			m_shotAngleText->SetFormat(kIntValue, "%d %% Hook");
		
		float displayangle = m_swingAngle;
		if(displayangle < 0.0f)
			displayangle = -displayangle;
		
		m_shotAngleText->SetValue(displayangle * 100.0f);
		
		m_shotDistText->Render();
		m_shotPowerText->Render();
		m_shotAngleText->Render();
	}
	
	if(showClubInfo)
	{
		RBGolfClub *club = RBGolfClub::GetClub(m_curClub);
		m_clubDistText->SetValue(club->m_dist);
		m_clubDistText->Render();
		
		m_powerRangeText->Render();
	}
	
	m_parText->SetValue((float) m_par);
	m_remainingDistText->SetValue(m_ballToHoleDist);
	
	if(m_state == kStateBallInHole)
	{
		m_scoreText->SetValue((float) GetScoreTracker(m_curPlayer)->GetScore(m_holeSet, m_holeNum, true));
		m_strokeText->SetValue((float) GetScoreTracker(m_curPlayer)->GetNumStrokes(m_holeNum));
	}
	else
	{
		m_scoreText->SetValue((float) GetScoreTracker(m_curPlayer)->GetScore(m_holeSet, m_holeNum, false));
		m_strokeText->SetValue((float) GetScoreTracker(m_curPlayer)->GetNumStrokes(m_holeNum) + 1);
	}
	
	if(m_holeSet != kCourseDrivingRange)
	{
		m_holeText->Render();
		m_strokeText->Render();
		m_scoreText->Render();
		m_parText->Render();
		m_remainingDistText->Render();
	}
	

}

void RBTGame::Render(float width, float height, int camera)
{
	RUDE_PERF_START(kPerfRBTGameRender1);
	
	RGL.SetViewport(0, 0, (int) height, (int) width);

	float aspect = width / height;
	m_curCamera->SetView(aspect, camera);
	RGL.LoadIdentity();
	
	AdjustGuide();
	
	m_skybox.Render();
	
	RGL.Enable(kDepthTest, true);
	m_terrain.Render();
	
	if(gDebugCamera)
	{
		m_ballRecorder.RenderRecords();
	}
	
	RUDE_PERF_STOP(kPerfRBTGameRender1);
	
	
	RGL.Enable(kDepthTest, true);
	m_golfer.Render();
	
	
	RUDE_PERF_START(kPerfRBTGameRender2);
	
	//m_ballRecorder.RenderRecords();
	
	
	//RGL.Enable(kDepthTest, false);
	if(m_state != kStateBallInHole)
		m_ball.Render();
	

	if(m_state == kStateFollowBall)
	{
		m_ballRecorder.RenderTracers();
	}
	
	if(m_state == kStatePositionSwing2 || m_state == kStatePositionSwing3)
	{
		RGL.LoadIdentity();
		m_ballGuide.Render();
	}
	
	RGL.LoadIdentity();
	
	if(!m_terrain.GetPutting() && m_holeSet != kCourseDrivingRange)
		m_pin.Render();
	
	RenderCalcOrthoDrawPositions();
	
	
	
	if(gDebugCamera)
		return;
	
	
	RGL.Enable(kDepthTest, false);
	RGLD.RenderDebug();
	
	bool renderingWind = false;
	if(m_state == kStatePositionSwing
	   || m_state == kStatePositionSwing2
	   || m_state == kStatePositionSwing3
	   || m_state == kStateExecuteSwing
	   || m_state == kStateTerrainView)
	{
		renderingWind = true;
		if(!m_terrain.GetPutting())
		{
			m_windControl->Render();
		}
	}
	
	RUDE_PERF_STOP(kPerfRBTGameRender2);
	
	
	RUDE_PERF_START(kPerfRBTGameRenderUI);
	
	
	if(RGL.GetLandscape())
	{
		RGL.SetViewport(0, 0, (int) height, (int) width);
		RGL.Ortho(0.0f, 0.0f, 0.0f, width, height, 100.0f);
	}
	else
	{
		RGL.SetViewport(0, 0, (int) height, (int) width);
		RGL.Ortho(0.0f, 0.0f, 0.0f, width, height, 100.0f);
	}
		
	RGL.LoadIdentity();
	RGL.Enable(kBackfaceCull, false);
	RGL.Enable(kDepthTest, false);
	
	if(renderingWind)
	{
		if(!m_terrain.GetPutting())
		{
			m_windText->Render();
		}
	}
	
	
	if(gRenderUI)
	{
		
		switch(m_state)
		{
			case kStatePositionSwing:
			case kStatePositionSwing2:
			case kStatePositionSwing3:
				RenderGuide(aspect);
				
				m_botBarBg->Render();
				m_menuButton->Render();
				m_swingButton->Render();
				m_nextClubButton->Render();
				m_prevClubButton->Render();
				m_clubButton->Render();
				m_cameraButton->Render();

				m_terrainSliceControl->Render();

				m_helpButton->Render();

				m_terrainButton->Render();
				
				RenderShotInfo(false, true);
				
				if(m_encouragementTimer > 0.0f)
				{
					m_shotEncouragementText->Render();
				}
				break;
			case kStateMenu:
				m_menu.Render(width, height, camera);
				break;
				
			case kStateExecuteSwing:
			case kStateWaitForSwing:
				m_botBarBg->Render();
				m_moveButton->Render();
				m_swingControl->Render();
				m_clubButton->Render();

				m_helpButton->Render();
				
				RenderShotInfo(false, true);
				
				break;
				
			case kStateHitBall:
			case kStateFollowBall:
				//RenderBallFollowInfo(false);
				RenderShotInfo(true, false);
				
				m_swingControl->Render();
				
				break;
				
			case kStateRegardBall:
				//RenderBallFollowInfo(true);
				RenderShotInfo(true, false);
				
				if(m_encouragementTimer > 0.0f)
				{
					m_shotEncouragementText->Render();
				}
				
				break;
				
			case kStateBallInHole:
				m_scoreControl->Render();
				RenderShotInfo(false, false);
				break;

			case kStateTerrainView:
				
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				m_terrainui.Render(width, height, camera);
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();

				RenderShotInfo(false, true);
				m_terrainButton->Render();
				break;
            default:
                break;
		}
	}
	
#ifndef NO_DECO_EDITOR
	m_dropDecoText.Render();
	m_dumpDecoText.Render();
#endif
	
	m_help.Render(width, height);
	
	RUDE_PERF_STOP(kPerfRBTGameRenderUI);
	
}

void RBTGame::LaunchBall()
{
	btVector3 &eye = m_curCamera->GetPos();	
	btVector3 &lookAt = m_curCamera->GetLookAt();
	
	btVector3 lookdir = lookAt - eye;
	lookdir.normalize();
	
	m_ball.SetPosition(eye);
	//m_ball.SetForce(lookdir * 1000);
}


void RBTGame::KeyDown(RBKey k)
{

}

void RBTGame::KeyUp(RBKey k)
{
}

void RBTGame::StylusDown(RudeScreenVertex &p)
{

}

void RBTGame::StylusUp(RudeScreenVertex &p)
{

}

void RBTGame::StylusMove(RudeScreenVertex &p)
{

}


void RBTGame::TouchDown(RudeTouch *rbt)
{
	if(gDebugCamera)
	{
		m_debugCamera.TouchDown(rbt);
		return;
	}
	
	if(m_help.TouchDown() == false)
		return;
	
	eSoundEffect sfx = kSoundNone;
	
	switch(m_state)
	{
		case kStatePositionSwing:
		case kStatePositionSwing2:
			
			if(m_state == kStatePositionSwing2)
			{
				if(m_guideAdjust->TouchDown(rbt))
				{
					SetState(kStatePositionSwing3);
					break;
				}
			}
			
			m_moveGuide = false;
			m_moveHeight = false;
			m_swingButton->TouchDown(rbt);
			m_swingCamAdjust->TouchDown(rbt);
			m_menuButton->TouchDown(rbt);
			if(m_prevClubButton->TouchDown(rbt))
			{
				NextClub(-1);
				sfx = kSoundUIClickLow;
			}
			if(m_nextClubButton->TouchDown(rbt))
			{
				NextClub(1);
				sfx = kSoundUIClickHi;
			}
			if(m_cameraButton->TouchDown(rbt))
			{
				if(m_state == kStatePositionSwing)
				{
					SetState(kStatePositionSwing2);
					sfx = kSoundUIClickHi;
				}
				else
				{
					SetState(kStatePositionSwing);
					sfx = kSoundUIClickLow;
				}
			
			}
			
			if(m_helpButton->TouchDown(rbt))
			{
				if(m_state == kStatePositionSwing)
					m_help.SetHelpMode(kHelpAim);
				else
					m_help.SetHelpMode(kHelpPlacementCamera);
				
				sfx = kSoundUIClickHi;
			}

			if(m_terrainButton->TouchDown(rbt))
			{
				SetState(kStateTerrainView);

				m_terrainui.StartDisplay();

				sfx = kSoundUIClickHi;
			}
			
			#ifndef NO_DECO_EDITOR
				m_dropDecoText.TouchDown(rbt);
				m_dumpDecoText.TouchDown(rbt);
			#endif
			
			break;
		case kStateMenu:
			m_menu.TouchDown(rbt);
			break;
		case kStateExecuteSwing:
			if(m_helpButton->TouchDown(rbt))
			{
				m_help.SetHelpMode(kHelpSwing);
				sfx = kSoundUIClickHi;
			}
			else
			{
				m_swingControl->TouchDown(rbt);
				m_moveButton->TouchDown(rbt);
				
			}
			
			
			break;
		case kStateFollowBall:
			m_swingButton->TouchDown(rbt);
			break;
		case kStateTerrainView:
			
			if(m_terrainButton->TouchDown(rbt))
			{
				SetState(kStatePositionSwing);
				sfx = kSoundUIClickLow;
			}
			else
				m_terrainui.TouchDown(rbt);

			break;
        default:
            break;
	}
	
	RudeSound::GetInstance()->PlayWave(sfx);

	
}

void RBTGame::TouchMove(RudeTouch *rbt)
{
	if(gDebugCamera)
	{
		m_debugCamera.TouchMove(rbt);
		return;
	}
	
	RUDE_PERF_START(kPerfTouchMove);
	
	switch(m_state)
	{
		case kStatePositionSwing:
			if(m_swingCamAdjust->TouchMove(rbt))
				MovePosition(m_swingCamAdjust->GetMoveDelta(), m_swingCamAdjust->GetDistanceTraveled());
			break;
		case kStatePositionSwing2:
			if(m_swingCamAdjust->TouchMove(rbt))
				MoveAimCamera(m_swingCamAdjust->GetMoveDelta(), m_swingCamAdjust->GetDistanceTraveled());
			break;
		case kStatePositionSwing3:
			if(m_guideAdjust->TouchMove(rbt))
			{
				m_guideScreenPoint = m_guideAdjust->GetHitMove();
				m_guideScreenCalc = true;
				//AdjustGuide(m_guideAdjust.GetHitMove());
			}
			break;
		case kStateMenu:
			m_menu.TouchMove(rbt);
			break;
		case kStateTerrainView:
			m_terrainui.TouchMove(rbt);
			break;
		case kStateExecuteSwing:
			m_swingControl->TouchMove(rbt);
			break;
        default:
            break;
	}
	
	RUDE_PERF_STOP(kPerfTouchMove);
}

void RBTGame::TouchUp(RudeTouch *rbt)
{
	if(gDebugCamera)
	{
		//if(rbt->m_location.m_y < 240)
		//	LaunchBall();
		
		m_debugCamera.TouchUp(rbt);
		
		return;
	}
	
	eSoundEffect sfx = kSoundNone;
	
	switch(m_state)
	{
		case kStatePositionSwing:
		case kStatePositionSwing2:
			if(m_swingButton->TouchUp(rbt))
			{
				SetState(kStateExecuteSwing);
				sfx = kSoundUIClickHi;
			}
			m_swingCamAdjust->TouchUp(rbt);
			m_nextClubButton->TouchUp(rbt);
			m_prevClubButton->TouchUp(rbt);
			
			if(m_menuButton->TouchUp(rbt))
			{
				SetState(kStateMenu);
				sfx = kSoundUIClickHi;
			}
			
			#ifndef NO_DECO_EDITOR
				if(m_dropDecoText.TouchUp(rbt))
				{
					gDecoDrop = (float) (((rand() % 8) * 3) + 50);
				}
				if(m_dumpDecoText.TouchUp(rbt))
				{
					m_terrain.DumpDecorator();
				}
			#endif
			
			break;
		case kStatePositionSwing3:
			m_guideAdjust->TouchUp(rbt);
			SetState(kStatePositionSwing2);
			break;
		case kStateMenu:
			m_menu.TouchUp(rbt);
			break;
		case kStateExecuteSwing:
			if(m_swingControl->TouchUp(rbt))
			{
				
				
			}
			if(m_moveButton->TouchUp(rbt))
			{
				SetState(kStatePositionSwing);
				sfx = kSoundUIClickHi;
			}
			break;
		case kStateFollowBall:
			if(m_swingButton->TouchUp(rbt))
			{
				if(m_followTimer > 0.0f)
					m_followTimer = kFollowTimerThreshold;
			}
			break;
		case kStateRegardBall:
			SetState(kStatePositionSwing);
			break;
		case kStateBallInHole:
			//SetState(kStateTeePosition);
			m_result = kResultComplete;
			m_done = true;
			break;
		case kStateTerrainView:
			m_terrainui.TouchUp(rbt);
			break;
        default:
            break;
	}
	
	RudeSound::GetInstance()->PlayWave(sfx);
}

void RBTGame::ScrollWheel(RudeScreenVertex &d)
{
    const float kYawDamping = 0.003f;
    const float kHeightDamping = 0.005f;
    
    const float kAimYawDamping = 0.006f;
    
    switch(m_state)
	{
		case kStatePositionSwing:
    		MoveSwingYaw(float(d.m_x) * kYawDamping);
            MoveSwingHeight(float(d.m_y) * kHeightDamping);
			break;
		case kStatePositionSwing2:
            MoveAimYaw(float(d.m_x) * kAimYawDamping);
            MoveAimHeight(float(d.m_y) * kHeightDamping);
			break;
        default:
            break;
	}
}

void RBTGame::Resize()
{
	m_menu.Resize();
	m_terrainui.Resize();

	if(RUDE_IPAD)
	{
		int width = (int) RGL.GetDeviceWidth();
		int center = width / 2;
		int offset = 768 / 2;
		m_ui.SetFileRect(RudeRect(0, center - offset, -1, center + offset));
	}
	m_ui.UpdateDrawRect();
    
    RudeRect guideRect(int(RGL.GetDeviceHeight()) / 2 - 32,
					   int(RGL.GetDeviceWidth()) / 2 - 32,
					   int(RGL.GetDeviceHeight()) / 2 + 32,
					   int(RGL.GetDeviceWidth()) / 2 + 32);
    m_guideAdjust->SetDrawRect(guideRect);
}

void RBTGame::Pause()
{

}



