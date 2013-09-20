#include "pch.h"
#include "common/Def_Str.h"
#include "../vdrift/pathmanager.h"
#include "../vdrift/game.h"
#include "../road/Road.h"
#include "CGame.h"
#include "CHud.h"
#include "CGui.h"
#include "common/Gui_Def.h"
#include "common/GraphView.h"
#include "common/Slider.h"
#include "FollowCamera.h"
#include <boost/algorithm/string.hpp>

#include "../sdl4ogre/sdlinputwrapper.hpp"

using namespace std;
using namespace Ogre;
using namespace MyGUI;


//-----------------------------------------------------------------------------------------------------------
//  Key pressed
//-----------------------------------------------------------------------------------------------------------

bool App::keyPressed(const SDL_KeyboardEvent &arg)
{	
	if (!mInputCtrl->keyPressed(arg))
		return true;
	for (int i=0; i<4; ++i)
	{
		if (!mInputCtrlPlayer[i]->keyPressed(arg))
			return true;
	}

	SDL_Scancode skey = arg.keysym.scancode;
	#define key(a)  SDL_SCANCODE_##a
	bool tweak = isTweak();

	//  main menu keys
	if (pSet->isMain && isFocGui)
	{
		switch (skey)
		{
		case key(UP):  case key(KP_8):
			pSet->inMenu = (pSet->inMenu-1 + ciMainBtns) % ciMainBtns;
			gui->toggleGui(false);  return true;

		case key(DOWN):  case key(KP_2):
			pSet->inMenu = (pSet->inMenu+1) % ciMainBtns;
			gui->toggleGui(false);  return true;

		case key(RETURN):
			pSet->isMain = false;
			gui->toggleGui(false);  return true;
		}
	}

	//  esc
	if (skey == key(ESCAPE))
	{
		if (pSet->escquit && !bAssignKey)
			mShutDown = true;	// quit
		else
			if (mWndChampStage->getVisible())  ///  close champ wnds
				gui->btnChampStageStart(0);
			else
			if (mWndChallStage->getVisible())  ///  chall
				gui->btnChallStageStart(0);
			else
				gui->toggleGui(true);	// gui on/off
		return true;
	}


	//  shortcut keys for gui access (alt-Q,C,S,G,V,.. )
	if (alt)
		switch (skey)
		{
			case key(Z):  // alt-Z Tweak (alt-shift-Z save&reload)
				gui->TweakToggle();	return true;

			case key(Q):	gui->GuiShortcut(MNU_Single, TAB_Track);	return true;  // Q Track
			case key(C):	gui->GuiShortcut(MNU_Single, TAB_Car);	return true;  // C Car

			case key(T):	gui->GuiShortcut(MNU_Single, TAB_Setup);	return true;  // T Car Setup
			case key(W):	gui->GuiShortcut(MNU_Single, TAB_Game);	return true;  // W Game Setup

			case key(J):	gui->GuiShortcut(MNU_Tutorial, TAB_Champs);	return true;  // J Tutorials
			case key(H):	gui->GuiShortcut(MNU_Champ,    TAB_Champs);	return true;  // H Champs
			case key(L):	gui->GuiShortcut(MNU_Challenge,TAB_Champs);	return true;  // L Challenges

			case key(U):	gui->GuiShortcut(MNU_Single, TAB_Multi);	return true;	// U Multiplayer
			case key(R):	gui->GuiShortcut(MNU_Replays, 1);		return true;		// R Replays

			case key(S):	gui->GuiShortcut(MNU_Options, 1);		return true;  // S Screen
			 case key(E):	gui->GuiShortcut(MNU_Options, 1,1);	return true;  // E -Effects
			case key(G):	gui->GuiShortcut(MNU_Options, 2);		return true;  // G Graphics
			 case key(N):	gui->GuiShortcut(MNU_Options, 2,3);	return true;  // N -Vegetation

			case key(V):	gui->GuiShortcut(MNU_Options, 3);		return true;  // V View
			 case key(M):	gui->GuiShortcut(MNU_Options, 3,1);	return true;  // M -Minimap
			 case key(O):	gui->GuiShortcut(MNU_Options, 3,3);	return true;  // O -Other
			case key(I):	gui->GuiShortcut(MNU_Options, 4);		return true;  // I Input
			case key(P):	gui->GuiShortcut(MNU_Options, 5);		return true;  // P Sound
		}


	//>--  dev shortcuts, alt-shift numbers, start test track
	if (pSet->dev_keys && alt && shift && !mClient)
	{
		string t;
		switch (skey)
		{
			case key(1): t = "Test1-Flat";  break;
			case key(2): t = "Test11-Jumps";  break;
			case key(3): t = "TestC4-ow";  break;
			case key(4): t = "Test7-FluidsSmall";  break;
			case key(5): t = "TestC6-temp";  break;
			case key(6): t = "Test10-FlatPerf";  break;
		}
		if (!t.empty())
		{
			gui->BackFromChs();
			pSet->gui.track = t;  bPerfTest = false;
			pSet->gui.track_user = false;
			NewGame();  return true;
		}
	}


	/// tire edit
	if (pSet->graphs_type == Gh_TireEdit && !tweak)
	{
		int& iCL = iEdTire==1 ? iCurLong : (iEdTire==0 ? iCurLat : iCurAlign);
		int iCnt = iEdTire==1 ? 11 : (iEdTire==0 ? 15 : 18);
		switch (skey)
		{
			case key(HOME):  case key(KP_7):  // mode long/lat
			if (ctrl)
				iTireLoad = 1-iTireLoad;
			else
				iEdTire = iEdTire==1 ? 0 : 1;  iUpdTireGr=1;  return true;

			case key(END):  case key(KP_1):  // mode align
				iEdTire = iEdTire==2 ? 0 : 2;  iUpdTireGr=1;  return true;

			case key(PAGEUP):  case key(KP_9):   // prev val
				iCL = (iCL-1 +iCnt)%iCnt;  iUpdTireGr=1;  return true;

			case key(PAGEDOWN):  case key(KP_3):   // next val
				iCL = (iCL+1)%iCnt;  iUpdTireGr=1;  return true;
		}
	}


	//  not main menus
	//--------------------------------------------------------------------------------------------------------------
	typedef CGui::WP WP;
	if (!tweak)
	{
		Widget* wf = MyGUI::InputManager::getInstance().getKeyFocusWidget();
		bool edFoc = wf && wf->getTypeName() == "EditBox";
		//if (wf)  LogO(wf->getTypeName()+" " +toStr(edFoc));
		switch (skey)
		{
			case key(BACKSPACE):
				if (mWndChampStage->getVisible())	// back from champs stage wnd
				{	gui->btnChampStageBack(0);  return true;  }
				else
				if (mWndChallStage->getVisible())	// chall
				{	gui->btnChallStageBack(0);  return true;  }

				if (pSet->isMain)  break;
				if (isFocGui)
				{	if (edFoc)  break;
					pSet->isMain = true;  gui->toggleGui(false);  }
				else
					if (mWndRpl && !isFocGui)	bRplWnd = !bRplWnd;  // replay controls
				return true;

			case key(P):	// replay play/pause
				if (bRplPlay && !isFocGui)
				{	bRplPause = !bRplPause;  gui->UpdRplPlayBtn();
					return true;  }
				break;

			case key(K):	// replay car ofs
				if (bRplPlay && !isFocGui)	{	--iRplCarOfs;  return true;  }
				break;
			case key(L):	// replay car ofs
				if (bRplPlay && !isFocGui)	{	++iRplCarOfs;  return true;  }
				break;

			case key(F):	// focus on find edit
				if (ctrl && gui->edFind && (pSet->dev_keys || isFocGui &&
					!pSet->isMain && pSet->inMenu == MNU_Single && mWndTabsGame->getIndexSelected() == TAB_Track))
				{
					if (pSet->dev_keys)
						gui->GuiShortcut(MNU_Single, 1);	// Track tab
					MyGUI::InputManager::getInstance().resetKeyFocusWidget();
					MyGUI::InputManager::getInstance().setKeyFocusWidget(gui->edFind);
					return true;
				}	break;


			case key(F7):		// Times
				if (shift)
				{	WP wp = gui->chOpponents;  ChkEv(show_opponents);  hud->Show();  }
				else if (!ctrl)
				{	WP wp = gui->chTimes;  ChkEv(show_times);  hud->Show();  }
				return false;

			case key(F8):		// car debug bars
				if (ctrl)
				{	WP wp = gui->chDbgB;  ChkEv(car_dbgbars);   hud->Show();  }
				else		// Minimap
				if (!shift)
				{	WP wp = gui->chMinimp;  ChkEv(trackmap);
					for (int c=0; c < hud->hud.size(); ++c)
						if (hud->hud[c].ndMap)  hud->hud[c].ndMap->setVisible(pSet->trackmap);
				}	return false;

			case key(F9):
				if (ctrl)
				{	WP wp = gui->chTireVis;  ChkEv(car_tirevis);  hud->Destroy();  hud->Create();  }
				else
				if (alt)	// car debug surfaces
				{	WP wp = gui->chDbgS;  ChkEv(car_dbgsurf);  hud->Show();  }
				else
				if (shift)	// car debug text
				{	WP wp = gui->chDbgT;  ChkEv(car_dbgtxt);  hud->Show();  }
				else		// graphs
				{	WP wp = gui->chGraphs;  ChkEv(show_graphs);
					for (int i=0; i < graphs.size(); ++i)
						graphs[i]->SetVisible(pSet->show_graphs);
				}
				return true;

			case key(F11):
				if (shift)	// profiler times
				{	WP wp = gui->chProfTxt;  ChkEv(profilerTxt);  hud->Show();  }
				else
				if (!ctrl)  // Fps
				{	WP wp = gui->chFps;  ChkEv(show_fps);
					return false;
				}	break;

			case key(F10):	//  blt debug, txt
				if (shift)
				{	WP wp = gui->chBltTxt;  ChkEv(bltProfilerTxt);  return false;  }
				else if (ctrl)
				{	WP wp = gui->chBlt;  ChkEv(bltDebug);  return false;  }
				else		// wireframe
					gui->toggleWireframe();
				return false;


			case key(RETURN):		///  close champ wnds
				if (mWndChampStage->getVisible())
					gui->btnChampStageStart(0);
				else				///  chall
				if (mWndChallStage->getVisible())
					gui->btnChallStageStart(0);
				else				//  new game  after up/dn
				if (isFocGui && !pSet->isMain)
					switch (pSet->inMenu)
					{
					case MNU_Replays:	gui->btnRplLoad(0);  break;
					default:
					{	if (mWndGame->getVisible())
						switch (mWndTabsGame->getIndexSelected())
						{
						case TAB_Track:	 gui->changeTrack();	gui->btnNewGame(0);  break;
						case TAB_Car:	 gui->changeCar();	gui->btnNewGame(0);  break;
						case TAB_Multi:	 gui->chatSendMsg();  break;
						case TAB_Champs:
							if (gui->isChallGui())
								  gui->btnChallStart(0);
							else  gui->btnChampStart(0);  break;
					}	break;
				}	}
				else
				if (mClient && !isFocGui)  // show/hide players net wnd
				{	mWndNetEnd->setVisible(!mWndNetEnd->getVisible());  return true;  }
				break;
		}
	}

	if (skey != key(RCTRL) && skey != key(LCTRL))
		MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum( mInputWrapper->sdl2OISKeyCode(arg.keysym.sym)), 0);
	return true;
}


void App::channelChanged(ICS::Channel *channel, float currentValue, float previousValue)
{
	if (currentValue != 1.f)
		return;

	#define action(a) (channel->getNumber() == a)

	//  change tweak tabs
	//----------------------------------------------------------------------------------------
	if (mWndTweak->getVisible())
	{
		TabPtr tab = gui->tabTweak;
		if (!shift && tab->getIndexSelected() == 0)
			tab = gui->tabEdCar;  // car edit sections

		if (action(A_PrevTab)) {  // prev gui subtab
			int num = tab->getItemCount();
			tab->setIndexSelected( (tab->getIndexSelected() - 1 + num) % num );  }
		else if (action(A_NextTab)) {  // next gui subtab
			int num = tab->getItemCount();
			tab->setIndexSelected( (tab->getIndexSelected() + 1) % num );  }

		if (tab == gui->tabEdCar)  // focus ed
		{	pSet->car_ed_tab = tab->getIndexSelected();
			MyGUI::InputManager::getInstance().resetKeyFocusWidget();
			MyGUI::InputManager::getInstance().setKeyFocusWidget(gui->edCar[tab->getIndexSelected()]);  }
	}else
	//  change gui tabs
	if (isFocGui && !pSet->isMain)
	{
		MyGUI::TabPtr tab = 0;  MyGUI::TabControl* sub = 0;
		switch (pSet->inMenu)
		{	case MNU_Replays:  tab = mWndTabsRpl;  break;
			case MNU_Help:     tab = mWndTabsHelp;  break;
			case MNU_Options:  tab = mWndTabsOpts;  sub = gui->vSubTabsOpts[tab->getIndexSelected()];  break;
			default:           tab = mWndTabsGame;  sub = gui->vSubTabsGame[tab->getIndexSelected()];  break;
		}
		if (tab)
		if (shift)
		{	if (sub)  // prev, next subtab
			{	bool chng = false;
				if (action(A_PrevTab))
				{	int num = sub->getItemCount();  chng = true;
					sub->setIndexSelected( (sub->getIndexSelected() - 1 + num) % num );
				}else if (action(A_NextTab))
				{	int num = sub->getItemCount();  chng = true;
						sub->setIndexSelected( (sub->getIndexSelected() + 1) % num );
				}
				if (chng && !sub->eventTabChangeSelect.empty())
					sub->eventTabChangeSelect(sub, sub->getIndexSelected());
		}	}
		else  // prev, next tab
		{	int num = tab->getItemCount()-1, i = tab->getIndexSelected(), n = 0;
			bool chng = false;
			if (action(A_PrevTab))
			{	do{  if (i==1)  i = num;  else  --i;  ++n;  }
				while (n < num && tab->getButtonWidthAt(i) == 1);
				chng = true;
			}else
			if (action(A_NextTab))
			{	do{  if (i==num)  i = 1;  else  ++i;  ++n;  }
				while (n < num && tab->getButtonWidthAt(i) == 1);
				chng = true;
			}
			if (chng)
			{	tab->setIndexSelected(i);  gui->MenuTabChg(tab,i);  return;  }
		}
	}
	else if (!isFocGui && pSet->show_graphs)  // change graphs type
	{
		int& v = (int&)pSet->graphs_type;  int vo = v;
		if (action(A_PrevTab))  v = (v-1 + Gh_ALL) % Gh_ALL;
		if (action(A_NextTab))	v = (v+1) % Gh_ALL;
		if (vo != v)
		{
			gui->cmbGraphs->setIndexSelected(v);
			gui->comboGraphs(gui->cmbGraphs, v);
			if (v == 4)  iUpdTireGr = 1;  //upd now
		}
	}
	//----------------------------------------------------------------------------------------


	//  Gui on/off  or close wnds
	if (action(A_ShowOptions) && !alt)
	{
		if (mWndNetEnd && mWndNetEnd->getVisible())  {  mWndNetEnd->setVisible(false);  // hide netw end
			return;	}
		else
		{
			if (mWndChampEnd && mWndChampEnd->getVisible())  mWndChampEnd->setVisible(false);  // hide champs end
			if (mWndChallEnd && mWndChallEnd->getVisible())  mWndChallEnd->setVisible(false);  // chall
			gui->toggleGui(true);  return;
		}
	}

	//  new game - Reload   not in multiplayer
	if (action(A_RestartGame) && !mClient)
	{
		bPerfTest = ctrl;  // ctrl-F5 start perf test
		if (bPerfTest)
		{	pSet->gui.track = "Test10-FlatPerf";
			pSet->gui.track_user = false;  }
		iPerfTestStage = PT_StartWait;
		NewGame();  return;
	}

	//  new game - fast (same track & cars)
	if (action(A_ResetGame) && !mClient)
	{
		for (int c=0; c < carModels.size(); ++c)
		{
			CarModel* cm = carModels[c];
			if (cm->pCar)  cm->pCar->bResetPos = true;
			cm->First();
			cm->ResetChecks();
			cm->iWonPlace = 0;  cm->iWonPlaceOld = 0;
			cm->iWonMsgTime = 0.f;
		}
		pGame->timer.Reset(-1);
		pGame->timer.pretime = mClient ? 2.0f : pSet->game.pre_time;  // same for all multi players
		carIdWin = 1;  //
		ghost.Clear(); //
	}
	
	//  Screen shot
	if (action(A_Screenshot))
		mWindow->writeContentsToTimestampedFile(PATHMANAGER::Screenshots() + "/", ".jpg");
	
}
