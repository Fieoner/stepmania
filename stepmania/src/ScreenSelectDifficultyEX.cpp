#include "global.h"
/*
-----------------------------------------------------------------------------
 Class: ScreenSelectDifficultyEX

 Desc: Difficulty select style from Extreme

 Copyright (c) 2001-2003 by the person(s) listed below.  All rights reserved.
	Chris Danford
	Kevin Slaughter
-----------------------------------------------------------------------------
*/

#include "ScreenSelectDifficultyEX.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "RageLog.h"
#include "GameConstantsAndTypes.h"
#include "AnnouncerManager.h"
#include "GameState.h"
#include "RageSoundManager.h"
#include "ThemeManager.h"


const float LOCK_INPUT_TIME = 0.30f;	// lock input while waiting for tweening to complete

#define DIFFICULTY_X( d )			THEME->GetMetricF("ScreenSelectDifficultyEX",ssprintf("DifficultyP%dX",d+1))
#define DIFFICULTY_Y( e )			THEME->GetMetricF("ScreenSelectDifficultyEX",ssprintf("DifficultyP%dY",e+1))
//#define DIFFICULTY_SINGLE_PLAYER_X	THEME->GetMetricF("ScreenSelectDifficultyEX","DifficultySinglePlayerX")
//#define DIFFICULTY_SINGLE_PLAYER_Y	THEME->GetMetricF("ScreenSelectDifficultyEX","DifficultySinglePlayerY")
#define CURSOR_OFFSET_EX_X( p )		THEME->GetMetricF("ScreenSelectDifficultyEX",ssprintf("CursorOffsetP%dX",p+1))
#define CURSOR_OFFSET_EX_Y( i )		THEME->GetMetricF("ScreenSelectDifficultyEX",ssprintf("CursorOffsetP%dY",i+1))
#define CURSOR_SHADOW_LENGTH_X		THEME->GetMetricF("ScreenSelectDifficultyEX","CursorShadowLengthX")
#define CURSOR_SHADOW_LENGTH_Y		THEME->GetMetricF("ScreenSelectDifficultyEX","CursorShadowLengthY")
#define INITIAL_CHOICE				THEME->GetMetricI("ScreenSelectDifficultyEX","InitialChoice")
#define HELP_TEXT					THEME->GetMetric("ScreenSelectDifficultyEX","HelpText")
#define TIMER_SECONDS				THEME->GetMetricI("ScreenSelectDifficultyEX","TimerSeconds")
#define NEXT_SCREEN_ARCADE			THEME->GetMetric("ScreenSelectDifficultyEX","NextScreenArcade")
#define NEXT_SCREEN_ONI				THEME->GetMetric("ScreenSelectDifficultyEX","NextScreenOni")
//int NUM_ENABLED_PLAYERS = 0;

const CString CHOICE_TEXT[ScreenSelectDifficultyEX::NUM_CHOICES_EX] = 
{
	"beginner",
	"easy",
	"medium",
	"hard",
	"nonstop",
	"oni",
	"endless"
};

float CURSOR_EX_X( int p ) { return DIFFICULTY_X(p) + CURSOR_OFFSET_EX_X(p); }
float CURSOR_EX_Y( int p ) { return DIFFICULTY_Y(p) + CURSOR_OFFSET_EX_Y(p); }
/*
float CURSOR_EX_SINGLE_X( int p ) { return DIFFICULTY_SINGLE_PLAYER_X + CURSOR_OFFSET_EX_X(p); }
float CURSOR_EX_SINGLE_Y( int p ) { return DIFFICULTY_SINGLE_PLAYER_Y + CURSOR_OFFSET_EX_Y(p); }
*/


const ScreenMessage SM_StartTweeningOffScreen	= ScreenMessage(SM_User + 3);
const ScreenMessage SM_StartFadingOut			= ScreenMessage(SM_User + 4);


ScreenSelectDifficultyEX::ScreenSelectDifficultyEX()
{
	LOG->Trace( "ScreenSelectDifficultyEX::ScreenSelectDifficultyEX()" );

	// Reset the current PlayMode
	GAMESTATE->m_PlayMode = PLAY_MODE_INVALID;

	
	m_Menu.Load(
		THEME->GetPathTo("BGAnimations","select difficulty ex"), 
		THEME->GetPathTo("Graphics","select difficulty ex top edge"),
		HELP_TEXT, true, true, TIMER_SECONDS
		);
	this->AddChild( &m_Menu );

/*	for( unsigned n=0; n<NUM_PLAYERS; n++ )
	{
		if( GAMESTATE->IsPlayerEnabled(n) == true )
		{
			NUM_ENABLED_PLAYERS++;
		}

	}
*/

	for( unsigned c=0; c<NUM_PLAYERS; c++ )
	{
		if( GAMESTATE->IsPlayerEnabled(c) == true )
		{
			CString sHeaderFile = ssprintf( "select difficulty ex header %s", CHOICE_TEXT[INITIAL_CHOICE].c_str() );
			CString sPictureFile = ssprintf( "select difficulty ex picture %s", CHOICE_TEXT[INITIAL_CHOICE].c_str() );

			m_sprPicture[c].Load( THEME->GetPathTo("Graphics",sPictureFile) );
/*			if( NUM_ENABLED_PLAYERS == 1 )
			{
				m_sprPicture[c].SetXY( DIFFICULTY_SINGLE_PLAYER_X, DIFFICULTY_SINGLE_PLAYER_Y );
			}
			else
			{
*/				m_sprPicture[c].SetXY( DIFFICULTY_X(c), DIFFICULTY_Y(c) );				
//			}
			m_sprPicture[c].SetVertAlign( align_top );
			m_framePages.AddChild( &m_sprPicture[c] );


			m_sprHeader[c].Load( THEME->GetPathTo("Graphics",sHeaderFile) );
/*			if( NUM_ENABLED_PLAYERS == 1 )
			{
				m_sprHeader[c].SetXY( DIFFICULTY_SINGLE_PLAYER_X, DIFFICULTY_SINGLE_PLAYER_Y );
			}
			else 
			{
*/				m_sprHeader[c].SetXY( DIFFICULTY_X(c), DIFFICULTY_Y(c) );
//			}
			m_sprHeader[c].SetVertAlign( align_bottom );
			m_framePages.AddChild( &m_sprHeader[c] );
		}
	}
	
	for( unsigned p=0; p<NUM_PLAYERS; p++ )
	{
		m_Choice[p] = (ChoiceEx)(INITIAL_CHOICE);
		CLAMP( m_Choice[p], (ChoiceEx)0, (ChoiceEx)(NUM_CHOICES_EX-1) );
		m_bChosen[p] = false;

		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;

		m_sprJoinMessageShadow[p].Load( THEME->GetPathTo("Graphics", "select difficulty ex cursor 2x1") );
		m_sprJoinMessageShadow[p].StopAnimating();
		m_sprJoinMessageShadow[p].SetState( p );
		m_sprJoinMessageShadow[p].TurnShadowOff();
		m_sprJoinMessageShadow[p].SetDiffuse( RageColor(0,0,0,0.6f) );
		m_framePages.AddChild( &m_sprJoinMessageShadow[p] );

		m_sprCursor[p].Load( THEME->GetPathTo("Graphics", "select difficulty ex cursor 2x1") );
		m_sprCursor[p].StopAnimating();
		m_sprCursor[p].SetState( p );
		m_sprCursor[p].TurnShadowOff();
		m_sprCursor[p].SetEffectGlowShift();
		m_framePages.AddChild( &m_sprCursor[p] );

		m_sprOK[p].Load( THEME->GetPathTo("Graphics", "select difficulty ex ok 2x1") );
		m_sprOK[p].SetState( p );
		m_sprOK[p].StopAnimating();
		m_sprOK[p].SetDiffuse( RageColor(1,1,1,0) );
		m_framePages.AddChild( &m_sprOK[p] );
	}

	this->AddChild( &m_framePages );
	
	m_soundChange.Load( THEME->GetPathTo("Sounds", "select difficulty change") );
	m_soundSelect.Load( THEME->GetPathTo("Sounds", "menu start") );
	m_soundDifficult.Load( ANNOUNCER->GetPathTo("select difficulty challenge") );

	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty intro") );

	m_Menu.TweenOnScreenFromMenu( SM_None );
	TweenOnScreen();

	SOUNDMAN->PlayMusic( THEME->GetPathTo("Sounds","select difficulty music") );

	m_fLockInputTime = LOCK_INPUT_TIME;
}


ScreenSelectDifficultyEX::~ScreenSelectDifficultyEX()
{
	LOG->Trace( "ScreenSelectDifficultyEX::~ScreenSelectDifficultyEX()" );
}


void ScreenSelectDifficultyEX::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	LOG->Trace( "ScreenSelectDifficultyEX::Input()" );

	if( m_Menu.IsClosing() )
		return;

	if( m_fLockInputTime > 0 )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default input handler
}

void ScreenSelectDifficultyEX::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
	m_fLockInputTime = max( m_fLockInputTime-fDeltaTime, 0 );
}

void ScreenSelectDifficultyEX::DrawPrimitives()
{
	m_Menu.DrawBottomLayer();
	Screen::DrawPrimitives();
	m_Menu.DrawTopLayer();
}

void ScreenSelectDifficultyEX::HandleScreenMessage( const ScreenMessage SM )
{
	int p;

	switch( SM )
	{
	case SM_MenuTimer:
		for( p=0; p<NUM_PLAYERS; p++ )
			if( GAMESTATE->IsPlayerEnabled(p) )
				MenuStart( (PlayerNumber)p );
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenTitleMenu" );
		break;
	case SM_GoToNextScreen:
		for( p=0; p<NUM_PLAYERS; p++ )
		{
			switch( m_Choice[p] )
			{
			case CHOICE_EX_BEGINNER:	GAMESTATE->m_PreferredDifficulty[p] = DIFFICULTY_BEGINNER;	break;
			case CHOICE_EX_EASY:		GAMESTATE->m_PreferredDifficulty[p] = DIFFICULTY_EASY;		break;
			case CHOICE_EX_NONSTOP:	// need to set preferred difficulty even for courses
			case CHOICE_EX_ONI:
			case CHOICE_EX_ENDLESS:
			case CHOICE_EX_MEDIUM:		GAMESTATE->m_PreferredDifficulty[p] = DIFFICULTY_MEDIUM;	break;
			case CHOICE_EX_HARD:		GAMESTATE->m_PreferredDifficulty[p] = DIFFICULTY_HARD;		break;
			default:	ASSERT(0);
			}
		}

		switch( m_Choice[GAMESTATE->m_MasterPlayerNumber] )
		{
		case CHOICE_EX_BEGINNER:
		case CHOICE_EX_EASY:
		case CHOICE_EX_MEDIUM:
		case CHOICE_EX_HARD:		GAMESTATE->m_PlayMode = PLAY_MODE_ARCADE;	break;
		case CHOICE_EX_NONSTOP:		GAMESTATE->m_PlayMode = PLAY_MODE_NONSTOP;	break;
		case CHOICE_EX_ONI:			GAMESTATE->m_PlayMode = PLAY_MODE_ONI;		break;
		case CHOICE_EX_ENDLESS:		GAMESTATE->m_PlayMode = PLAY_MODE_ENDLESS;	break;
		default:	ASSERT(0);	// bad selection
		}

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_ARCADE:	SCREENMAN->SetNewScreen( NEXT_SCREEN_ARCADE );	break;
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:	SCREENMAN->SetNewScreen( NEXT_SCREEN_ONI );		break;
		default:	ASSERT(0);
		}
		break;
	case SM_StartTweeningOffScreen:
		TweenOffScreen();
		this->SendScreenMessage( SM_StartFadingOut, 0.8f );
		break;
	case SM_StartFadingOut:
		m_Menu.TweenOffScreenToMenu( SM_GoToNextScreen );
		break;
	}
}

void ScreenSelectDifficultyEX::MenuLeft( PlayerNumber pn )
{
	if( m_Choice[pn] == 0 )	// can't go left any more
		return;
	if( m_bChosen[pn] )
		return;

	ChangeTo( pn, m_Choice[pn], m_Choice[pn]-1 );
}


void ScreenSelectDifficultyEX::MenuRight( PlayerNumber pn )
{
	if( m_Choice[pn] == NUM_CHOICES_EX-1 )	// can't go right any more
		return;
	if( m_bChosen[pn] )
		return;

	ChangeTo( pn, m_Choice[pn], m_Choice[pn]+1 );
}

bool ScreenSelectDifficultyEX::AnyPlayerIsOnCourse()
{	
	int pl;
	for( pl=0; pl<NUM_PLAYERS; pl++ )
	{
		switch( m_Choice[pl] )
		{
			case CHOICE_EX_NONSTOP:
			case CHOICE_EX_ONI:
			case CHOICE_EX_ENDLESS: return true; break;
		}
	}

	return false;
}

bool ScreenSelectDifficultyEX::PlayerIsOnCourse( PlayerNumber pl )
{
	switch( m_Choice[pl] )
	{
		case CHOICE_EX_NONSTOP:
		case CHOICE_EX_ONI:
		case CHOICE_EX_ENDLESS: return true; break;
	}
	return false;
}

void ScreenSelectDifficultyEX::ChangeTo( PlayerNumber pn, int iOldChoice, int iNewChoice )
{
	bool bSomeoneMadeAChoice = false;
	int p;
	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( GAMESTATE->IsPlayerEnabled(p) && m_bChosen[p] )
		{
			bSomeoneMadeAChoice = true;
		}
	}

	// change only the player who pressed the button
	m_Choice[pn] = (ChoiceEx)iNewChoice;

	if( bSomeoneMadeAChoice && AnyPlayerIsOnCourse() )
	{
		m_Choice[pn] = (ChoiceEx)iOldChoice; // Put it back on the old difficulty
		return;	// don't allow changing to courses after one player has chosen
	}
	
	if( PlayerIsOnCourse(pn) ) // If one player is on a course, all are on the same one
	{
		for( unsigned f=0; f<NUM_PLAYERS; f++ )
		{
			m_Choice[f] = (ChoiceEx)m_Choice[pn];
		}
	}
	
	bool bOtherWasOnCourse = false;
	if( !PlayerIsOnCourse(pn) && AnyPlayerIsOnCourse() )
	{	// Current player is not on a course, but another player IS.. fix this
		for( unsigned s=0; s<NUM_PLAYERS;s++)
		{
			m_Choice[s] = (ChoiceEx)m_Choice[pn];
			bOtherWasOnCourse = true;
		}
	}


	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( AnyPlayerIsOnCourse() == true || bOtherWasOnCourse == true )
		{
			if( m_Choice[p] == CHOICE_EX_NONSTOP && iOldChoice != CHOICE_EX_ONI || m_Choice[p] == CHOICE_EX_HARD && iOldChoice != CHOICE_EX_MEDIUM )
			{
				// Lock any changes during this, like arcade does
				m_fLockInputTime = 0.5f;
				
				// roll up, only if this is HEAVY-2-NONSTOP, or NONSTOP-2-HEAVY
				m_sprPicture[p].StopTweening();
				m_sprPicture[p].BeginTweening( 0.01f, TWEEN_BOUNCE_BEGIN );
				m_sprPicture[p].SetTweenZoomY( 0 );
				m_sprHeader[p].StopTweening();
				m_sprHeader[p].BeginTweening( 0.01f, TWEEN_BOUNCE_BEGIN );
				m_sprHeader[p].SetTweenZoomY( 0 );
			}
		}


		CString sPictureFile = ssprintf( "select difficulty ex picture %s", CHOICE_TEXT[m_Choice[p]].c_str() );
		m_sprPicture[p].Load( THEME->GetPathTo("Graphics",sPictureFile) );
		
		sPictureFile = ssprintf( "select difficulty ex header %s", CHOICE_TEXT[m_Choice[p]].c_str() );
		m_sprHeader[p].Load( THEME->GetPathTo("Graphics",sPictureFile) );		


		if( AnyPlayerIsOnCourse() == true || bOtherWasOnCourse == true )
		{
			if( m_Choice[p] == CHOICE_EX_NONSTOP && iOldChoice != CHOICE_EX_ONI || m_Choice[p] == CHOICE_EX_HARD && iOldChoice != CHOICE_EX_MEDIUM )
			{
				/* See above.. This executes on the same conditions. Those conditions
				   should --NOT-- have changed by this time */
				m_sprPicture[p].BeginTweening( 0.3f, TWEEN_BOUNCE_END );
				m_sprPicture[p].SetTweenZoomY( 1 );

				m_sprHeader[p].BeginTweening( 0.3f, TWEEN_BOUNCE_END );
				m_sprHeader[p].SetTweenZoomY( 1 );
			}
		}


		m_sprJoinMessageShadow[p].StopTweening();
		m_sprJoinMessageShadow[p].BeginTweening( 0.2f, TWEEN_LINEAR );
/*		if( NUM_ENABLED_PLAYERS == 1 )
		{
			m_sprJoinMessageShadow[p].SetTweenX( CURSOR_EX_SINGLE_X((PlayerNumber)p) );
			m_sprJoinMessageShadow[p].SetTweenY( CURSOR_EX_SINGLE_Y((PlayerNumber)p) );
		}
		else
		{
*/			m_sprJoinMessageShadow[p].SetTweenX( CURSOR_EX_X((PlayerNumber)p) );
			m_sprJoinMessageShadow[p].SetTweenY( CURSOR_EX_Y((PlayerNumber)p) );			
//		}
	}

	m_soundChange.Play();
}

void ScreenSelectDifficultyEX::MenuStart( PlayerNumber pn )
{
	bool bOKAllPlayers = false; // This keeps OK from being displayed again, if the player has already chosen
	if( m_bChosen[pn] == true )
		return;
	
	if( AnyPlayerIsOnCourse() == true ) // If a course is selected, all players get the same thing
	{
		for( int p=0; p<NUM_PLAYERS; p++ )
		{
			m_bChosen[p] = true;
		}
		bOKAllPlayers = true;
	}
	else
	{
		m_bChosen[pn] = true;
	}

	m_soundSelect.Play();
	int iSelection = m_Choice[pn];

	switch( iSelection )
	{
		case 0:	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty comment beginner") );	break;
		case 1:	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty comment easy") );		break;
		case 2:	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty comment medium") );		break;
		case 3:	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty comment hard") );		break;
		case 4:	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty comment nonstop") );		break;
		case 5:	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty comment oni") );			break;
		case 6:	SOUNDMAN->PlayOnceFromDir( ANNOUNCER->GetPathTo("select difficulty comment endless") );		break;
	}
	if( bOKAllPlayers == true ) // Show OK Icon
	{
		for( unsigned pk=0; pk<NUM_PLAYERS; pk++ )
		{
			ShowSelected((PlayerNumber)pk);
		}
	}
	else
	{
		ShowSelected((PlayerNumber)pn);
	}


	// check to see if everyone has chosen
	for( int p=0; p<NUM_PLAYERS; p++ )
	{
		if( GAMESTATE->IsPlayerEnabled((PlayerNumber)p)  &&  m_bChosen[p] == false )
			return;
	}
	m_Menu.StopTimer();
	this->SendScreenMessage( SM_StartTweeningOffScreen, 0.7f );
}

void ScreenSelectDifficultyEX::ShowSelected( PlayerNumber pv )
{

	m_sprCursor[pv].BeginTweening( 0.2f );
	m_sprCursor[pv].BeginTweening( 0.2f );
	
	
/*	if( NUM_ENABLED_PLAYERS == 1 )
	{
		m_sprCursor[pv].SetTweenX( CURSOR_EX_SINGLE_X(pv) );
		m_sprCursor[pv].SetTweenY( CURSOR_EX_SINGLE_Y(pv) );
		m_sprOK[pv].SetX( CURSOR_EX_SINGLE_X(pv) );
		m_sprOK[pv].SetY( CURSOR_EX_SINGLE_Y(pv) );
	}
	else
	{
*/		m_sprCursor[pv].SetTweenX( CURSOR_EX_X(pv) );
		m_sprCursor[pv].SetTweenY( CURSOR_EX_Y(pv) );
		m_sprOK[pv].SetX( CURSOR_EX_X(pv) );
		m_sprOK[pv].SetY( CURSOR_EX_Y(pv) );
//	}


	m_sprOK[pv].SetDiffuse( RageColor(1,1,1,0) );
	m_sprOK[pv].SetZoom( 2 );

	m_sprOK[pv].BeginTweening( 0.2f );
	m_sprOK[pv].SetTweenZoom( 1 );
	m_sprOK[pv].SetTweenDiffuse( RageColor(1,1,1,1) );

	m_sprJoinMessageShadow[pv].BeginTweening( 0.2f );
	m_sprJoinMessageShadow[pv].BeginTweening( 0.2f );
	m_sprJoinMessageShadow[pv].SetDiffuse( RageColor(0,0,0,0) );
}

void ScreenSelectDifficultyEX::MenuBack( PlayerNumber pn )
{
	m_Menu.TweenOffScreenToBlack( SM_GoToPrevScreen, true );
}

void ScreenSelectDifficultyEX::TweenOffScreen()
{
	unsigned p;

	for( p=0; p < m_SubActors.size(); p++ )
		m_SubActors[p]->StopTweening();

	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;

		m_sprCursor[p].BeginTweening( 0.3f );
		m_sprCursor[p].SetTweenZoom( 0 );

		m_sprOK[p].BeginTweening( 0.3f );
		m_sprOK[p].SetTweenZoom( 0 );

		m_sprJoinMessageShadow[p].BeginTweening( 0.3f );
		m_sprJoinMessageShadow[p].SetTweenDiffuse( RageColor(0,0,0,0) );
	}

	for( unsigned c=0; c<NUM_PLAYERS; c++ )
	{
		m_sprHeader[c].FadeOff( 0, "fadeout", 0.3f );
		m_sprPicture[c].FadeOff( 0, "fadeout", 0.3f );
	}
}

void ScreenSelectDifficultyEX::TweenOnScreen() 
{
	unsigned p;
	for( p=0; p<NUM_PLAYERS; p++ )
	{
		if( !GAMESTATE->IsPlayerEnabled((PlayerNumber)p) )
			continue;

//		int iSelection = m_Choice[p];
/*		if( NUM_ENABLED_PLAYERS == 1 )
		{
			m_sprCursor[p].SetXY( CURSOR_EX_SINGLE_X((PlayerNumber)p), CURSOR_EX_SINGLE_Y((PlayerNumber)p) );
		}
		else
		{
*/			m_sprCursor[p].SetXY( CURSOR_EX_X((PlayerNumber)p), CURSOR_EX_Y((PlayerNumber)p) );
//		}


		m_sprCursor[p].FadeOn( 0, "SpinZ ZoomX ZoomY Fade", 0.3f );
/*		if( NUM_ENABLED_PLAYERS == 1 )
		{
			m_sprJoinMessageShadow[p].SetXY( CURSOR_EX_SINGLE_X((PlayerNumber)p), CURSOR_EX_SINGLE_Y((PlayerNumber)p) );
		}
		else
		{
*/			m_sprJoinMessageShadow[p].SetXY( CURSOR_EX_X((PlayerNumber)p), CURSOR_EX_Y((PlayerNumber)p) );
//		}
		RageColor colorOriginal = m_sprJoinMessageShadow[p].GetDiffuse();
		m_sprJoinMessageShadow[p].SetDiffuse( RageColor(0,0,0,0) );
		m_sprJoinMessageShadow[p].BeginTweening( 0.3f );
		m_sprJoinMessageShadow[p].SetTweenDiffuse( colorOriginal );
	}

	for( unsigned d=0; d<NUM_PLAYERS; d++ )
	{
		// fade in and bring onscreen
/*		if( NUM_ENABLED_PLAYERS == 1 )
		{
			m_sprHeader[d].SetXY( DIFFICULTY_SINGLE_PLAYER_X, DIFFICULTY_SINGLE_PLAYER_Y );
			m_sprPicture[d].SetXY( DIFFICULTY_SINGLE_PLAYER_X, DIFFICULTY_SINGLE_PLAYER_Y );
		}
		else
		{
*/			m_sprHeader[d].SetXY( DIFFICULTY_X(d), DIFFICULTY_Y(d) );
			m_sprPicture[d].SetXY( DIFFICULTY_X(d), DIFFICULTY_Y(d) );			
//		}
		m_sprHeader[d].FadeOn( 0, "fadeon",0.3f );
		m_sprPicture[d].FadeOn( 0, "fadeon",0.3f );
	}
}
