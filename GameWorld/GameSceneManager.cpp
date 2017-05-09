#include "../CMainServer/CMainServer.h"
#include "GameSceneManager.h"
#include "ObjectEngine.h"
#include "MonsterObject.h"
#include <io.h>
#include <Shlwapi.h>
#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>
#include "DBThread.h"
#include "../Helper.h"
#include "GameWorld.h"
#include "struct.h"
#include "ObjectValid.h"
#include "TeammateControl.h"
#include "ExceptionHandler.h"
#include "../../CommonModule/SettingLoader.h"
#include "../../CommonModule/ExecuteTimer.h"
#include "GameInstanceScene.h"
#include "MonsterTemplateObject.h"
#include "../../CommonModule/ProtoType.h"
#include "../../CommonModule/loginsvr.pb.h"
//////////////////////////////////////////////////////////////////////////
extern HWND g_hServerDlg;
//////////////////////////////////////////////////////////////////////////
const int g_nSearchPoint[] =
{
	0, 0,
	-1, 0,
	1,0,
	0,-1,
	-1,-1,
	1,-1,
	0,1,
	-1,1,
	1,1,
	-2,-2,
	-1,-2,
	0,-2,
	1,-2,
	2,-2,
	-2,-1,
	2,-1,
	-2,0,
	2,0,
	-2,1,
	2,1,
	-2,2,
	-1,2,
	0,2,
	1,2,
	2,2


	//////////////////////////////////////////////////////////////////////////
	,
	-3, -3,
	-2, -3,
	-1, -3,
	0, -3,
	1, -3,
	2, -3,
	3, -3,
	3, -2,
	3, -1,
	3, 0,
	3, 1,
	3, 2,
	3, 3,
	2, 3,
	1, 3,
	0, 3,
	-1, 3,
	-2, 3,
	-3, 3,
	-3, 2,
	-3, 1,
	-3, 0,
	-3, -1,
	-3, -2
};

#define DEFAULT_MAX_COUNT_INSTANCE	10
#ifdef _DEBUG
#define DEFAULT_INSTANCE_MAP_FREE_TIME 2 * 1000
#else
#define DEFAULT_INSTANCE_MAP_FREE_TIME 10 * 60 * 1000
#endif
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
/************************************************************************/
/* Class GameSceneManager
/************************************************************************/
//////////////////////////////////////////////////////////////////////////
//	static variable
static GameSceneManager* s_pGameSceneManager = NULL;
//////////////////////////////////////////////////////////////////////////
GameSceneManager::GameSceneManager()
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		m_pScenes[i] = NULL;
	}
	m_dwLoadedMap = 0;

	/*if(!GetRunMapDataEx(m_xRunMapData))
	{
		LOG(ERROR) << "Can't load the run map information.";
	}
	if(!GetInstanceMapDataEx(m_xInsMapData))
	{
		LOG(ERROR) << "Can't load the instance map information.";
	}*/

	m_dwLastNotifyPlayerCount = 0;
	m_dwFixedMapIDSeed = FIXED_MAPID_BEGIN;
	m_dwInstanceMapIDSeed = INSTANCE_MAPID_BEGIN;
	m_nMaxInstanceScenesCount = DEFAULT_MAX_COUNT_INSTANCE;
}

GameSceneManager::~GameSceneManager()
{
	ReleaseAllScene();
}

GameSceneManager* GameSceneManager::GetInstance()
{
	if(NULL == s_pGameSceneManager)
	{
		s_pGameSceneManager = new GameSceneManager;
	}
	return s_pGameSceneManager;
}

void GameSceneManager::DestroyInstance()
{
	if(NULL != s_pGameSceneManager)
	{
		delete s_pGameSceneManager;
		s_pGameSceneManager = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////
bool GameSceneManager::CreateAllScene()
{
	lua_State* pWorldState = GameWorld::GetInstance().GetLuaEngine()->GetVM();
	if (!m_xMapConfigManager.Init(pWorldState))
	{
		LOG(ERROR) << "初始化地图信息失败";
		return false;
	}

	const std::vector<int>& refFixedMaps = m_xMapConfigManager.GetFixedMaps();
	for (size_t i = 0; i < refFixedMaps.size(); ++i)
	{
		GameScene* pNewScene = new GameScene;
		const LuaMapInfo* pMapInfo = m_xMapConfigManager.GetLuaMapInfo(refFixedMaps[i]);
		if (NULL == pMapInfo)
		{
			LOG(ERROR) << "Can't find map info of map [" << refFixedMaps[i] << "]";
			return false;
		}
		if (0 != pMapInfo->nMapType)
		{
			LOG(ERROR) << "Trying to create fix map with unmatched map type [" << pMapInfo->nMapType << "]";
			return false;
		}
		// key : map id
		m_pScenes[i] = pNewScene;
		if (!pNewScene->Initialize(pMapInfo->nResID, m_dwFixedMapIDSeed))
		{
			LOG(ERROR) << "初始化地图 ResId:" << pMapInfo->nResID << " MapId:" << m_dwFixedMapIDSeed << " 失败";
			return false;
		}

		++m_dwFixedMapIDSeed;
	}

	m_dwLoadedMap = m_dwFixedMapIDSeed;
	LOG(INFO) << "载入场景数量[" << m_dwFixedMapIDSeed << "]个";
	return true;

	/*if(m_xRunMapData.empty())
	{
		return false;
	}
	DWORD dwTotal = 0;
	bool bSuc = true;
	dwTotal = m_xRunMapData.size();

	if(dwTotal > 0)
	{
		for(DWORD i = 0; i < dwTotal; ++i)
		{
			m_pScenes[i] = new GameScene;
			bSuc = bSuc && m_pScenes[i]->Initialize(i);
			if(!bSuc)
			{
				break;
			}
			++m_dwLoadedMap;
		}
	}
	LOG(INFO) << "载入场景数量[" << m_dwLoadedMap << "]个";

	//	读入地图名配置
	char szPath[MAX_PATH];
	sprintf(szPath, "%s\\Config\\map.ini",
		GetRootPath());
	m_xIniMapName.LoadFile(szPath);

	return bSuc;*/
}
//////////////////////////////////////////////////////////////////////////
void GameSceneManager::ReleaseAllScene()
{
	for(DWORD i = 0; i < m_dwLoadedMap; ++i)
	{
		m_pScenes[i]->Release();
		delete m_pScenes[i];
		m_pScenes[i] = NULL;
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pScene->Release();
			delete pScene;
			pScene = NULL;
		}
	}
	m_dwLoadedMap = 0;
}
//////////////////////////////////////////////////////////////////////////
bool GameSceneManager::InsertPlayer(GameObject* _pObj)
{
	/*DWORD wMapID = _pObj->GetMapID();
	if(wMapID >= MAX_SCENE_NUMBER)
	{
		return false;
	}
	if(m_pScenes[wMapID])
	{
		return m_pScenes[wMapID]->InsertPlayer(_pObj);
	}
	return false;*/
	GameScene* pScene = GetScene(_pObj->GetMapID());
	if (NULL == pScene)
	{
		return false;
	}
	return pScene->InsertPlayer(_pObj);
}
//////////////////////////////////////////////////////////////////////////
bool GameSceneManager::InsertNPC(GameObject* _pObj)
{
	/*DWORD wMapID = _pObj->GetMapID();
	if(wMapID >= MAX_SCENE_NUMBER)
	{
		return false;
	}
	if(m_pScenes[wMapID])
	{
		return m_pScenes[wMapID]->InsertNPC(_pObj);
	}
	return false*/
	GameScene* pScene = GetScene(_pObj->GetMapID());
	if (NULL == pScene)
	{
		return false;
	}
	return pScene->InsertNPC(_pObj);
}
//////////////////////////////////////////////////////////////////////////
bool GameSceneManager::RemovePlayer(DWORD _dwID)
{
	for(DWORD i = 0; i < m_dwLoadedMap; ++i)
	{
		if(m_pScenes[i])
		{
			if(m_pScenes[i]->RemovePlayer(_dwID))
			{
				return true;
			}
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			if (pScene->RemovePlayer(_dwID))
			{
				return true;
			}
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
GameObject* GameSceneManager::GetPlayer(WORD _wMapID, DWORD _dwID)
{
	/*if(_wMapID >= MAX_SCENE_NUMBER)
	{
		return NULL;
	}
	if(m_pScenes[_wMapID])
	{
		std::map<DWORD, GameObject*>::iterator iter = m_pScenes[_wMapID]->m_xPlayers.find(_dwID);
		if(iter != m_pScenes[_wMapID]->m_xPlayers.end())
		{
			return iter->second;
		}
	}*/
	GameScene* pScene = GetScene(_wMapID);
	if (NULL == pScene)
	{
		return NULL;
	}
	std::map<DWORD, GameObject*>::iterator iter = pScene->m_xPlayers.find(_dwID);
	if(iter != pScene->m_xPlayers.end())
	{
		return iter->second;
	}
	return NULL;
}
//////////////////////////////////////////////////////////////////////////
void GameSceneManager::GetPlayerByUid(int _nUid, GameObjectList& _refList)
{
	for(DWORD i = 0; i < m_dwLoadedMap; ++i)
	{
		if(i >= MAX_SCENE_NUMBER)
		{
			break;
		}

		GameScene* pScene = m_pScenes[i];

		if(pScene)
		{
			std::map<DWORD, GameObject*>& refPlayers = pScene->m_xPlayers;
			std::map<DWORD, GameObject*>::iterator iter = refPlayers.begin();

			for(iter;
				iter != refPlayers.end();
				++iter)
			{
				GameObject* pObj = iter->second;
				if(NULL == pObj)
				{
					continue;
				}
				if(pObj->GetType() != SOT_HERO)
				{
					continue;
				}

				HeroObject* pHero = (HeroObject*)pObj;
				if(pHero->GetUID() == _nUid)
				{
					_refList.push_back(pHero);
				}
			}
		}
	}

	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			std::map<DWORD, GameObject*>& refPlayers = pScene->m_xPlayers;
			std::map<DWORD, GameObject*>::iterator iter = refPlayers.begin();

			for(iter;
				iter != refPlayers.end();
				++iter)
			{
				GameObject* pObj = iter->second;
				if(NULL == pObj)
				{
					continue;
				}
				if(pObj->GetType() != SOT_HERO)
				{
					continue;
				}

				HeroObject* pHero = (HeroObject*)pObj;
				if(pHero->GetUID() == _nUid)
				{
					_refList.push_back(pHero);
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
GameObject* GameSceneManager::GetPlayer(DWORD _dwID)
{
	GameObject* pObj = NULL;

	for(DWORD i = 0; i < m_dwLoadedMap; ++i)
	{
		pObj = GetPlayer(i, _dwID);
		if(pObj)
		{
			return pObj;
		}
	}

	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pObj = pScene->GetPlayer(_dwID);
			if (NULL != pObj)
			{
				return pObj;
			}
		}
	}
	return pObj;
}
//////////////////////////////////////////////////////////////////////////
void GameSceneManager::ReloadScript()
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			if(m_pScenes[i]->ReloadScript())
			{
				LOG(INFO) << "重新载入脚本[scene" << m_pScenes[i]->GetMapResID() << ".lua]成功";
			}
		}
		else
		{
			break;
		}
	}

	if(!m_xInsMaps.empty())
	{
		INSTANCEMAPLIST::const_iterator begIter = m_xInsMaps.begin();
		INSTANCEMAPLIST::const_iterator endIter = m_xInsMaps.end();

		for(begIter;
			begIter != endIter;
			++begIter)
		{
			if((*begIter)->ReloadScript())
			{
				LOG(INFO) << "重新载入脚本[scene" << (*begIter)->GetMapResID() << ".lua]成功";
			}
		}
	}

	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			if (pScene->ReloadScript())
			{
				LOG(INFO) << "重新载入脚本[scene" << pScene->GetMapResID() << ".lua]成功";
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
void GameSceneManager::Update(DWORD _dwTick)
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			m_pScenes[i]->Update(_dwTick);
		}
		else
		{
			break;
		}
	}

	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			m_pScenes[i]->ProcessWaitInsert();
			m_pScenes[i]->ProcessWaitRemove();
			m_pScenes[i]->ProcessWaitDelete();
		}
		else
		{
			break;
		}
	}

	if(!m_xInsMaps.empty())
	{
		INSTANCEMAPLIST::const_iterator begIter = m_xInsMaps.begin();
		INSTANCEMAPLIST::const_iterator endIter = m_xInsMaps.end();

		for(begIter;
			begIter != endIter;
			++begIter)
		{
			(*begIter)->Update(_dwTick);
		}
	}

	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pScene->Update(_dwTick);

			// should be free ?
		}

		// process wait events
		it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			)
		{
			GameScene* pScene = *it;
			pScene->ProcessWaitInsert();
			pScene->ProcessWaitRemove();
			pScene->ProcessWaitDelete();

			// TODO free some unused scene ?
			if (pScene->GetPlayerCount() == 0 &&
				pScene->GetSlaveSum() == 0)
			{
				if (_dwTick - pScene->m_dwInstanceMapFreeTime > DEFAULT_INSTANCE_MAP_FREE_TIME)
				{
					//	free the instance map
					int nMapID = pScene->GetMapID();
					int nMapResID = pScene->GetMapResID();
					LOG(INFO) << "Delete instance map ResID:" << nMapResID << " MapID:" << nMapID;
					pScene->DeleteAllItem();
					pScene->DeleteAllMonster();
					pScene->DeleteAllNPC();
					assert(pScene->m_xNPCs.empty());
					pScene->Release();
					pScene->GetLuaEngine()->Destroy();
					delete pScene;
					pScene = NULL;
					it = m_xInstanceScenes.erase(it);
					assert(_CrtCheckMemory());
					LOG(INFO) << "Delete instance map ResID:" << nMapResID << " MapID:" << nMapID << " done";
					continue;
				}
			}
			else
			{
				pScene->m_dwInstanceMapFreeTime = GetTickCount();
			}

			++it;
		}
	}

	if(GetTickCount() - m_dwLastNotifyPlayerCount > UPDATEPLAYERCOUNT_INTERVAL)
	{
		m_dwLastNotifyPlayerCount = GetTickCount();

		int nCounter = CountPlayer();
		int nMonsterCounter = CountMonster();

		PostMessage(g_hServerDlg, WM_PLAYERCOUNT, nCounter, nMonsterCounter);
	}

	assert(_CrtCheckMemory());
}
//////////////////////////////////////////////////////////////////////////
GameInstanceScene* GameSceneManager::GetFreeInstanceScene(int _id)
{
	if(_id < 100)
	{
		return NULL;
	}

	GameInstanceScene* pInsScene = NULL;

	if(!m_xInsMaps.empty())
	{
		INSTANCEMAPLIST::const_iterator begIter = m_xInsMaps.begin();
		INSTANCEMAPLIST::const_iterator endIter = m_xInsMaps.end();

		for(begIter;
			begIter != endIter;
			++begIter)
		{
			pInsScene = *begIter;
			if(pInsScene->GetMapID() == _id &&
				pInsScene->IsFree())
			{
				return pInsScene;
			}
		}
	}

	if(pInsScene == NULL)
	{
		pInsScene = new GameInstanceScene;
		if(pInsScene->Initialize(_id))
		{
			m_xInsMaps.push_back(pInsScene);
		}
		else
		{
			LOG(ERROR) << "Initialize failed when loading the instance map[" << _id << "]";
			SAFE_DELETE(pInsScene);
		}
	}

	return pInsScene;
}

const char* GameSceneManager::GetMapChName(int _id)
{
	/*const char* pszRunmapName = GetRunMap(_id);
	if(NULL == pszRunmapName)
	{
		return "未知地图";
	}
	else
	{
		const char* pszChMapName = m_xIniMapName.GetValue("MapNameInfo", pszRunmapName);
		if(NULL == pszChMapName)
		{
			return "未知地图";
		}
		if(strlen(pszChMapName) == 0)
		{
			return "未知地图";
		}
		return pszChMapName;
	}*/
	GameScene* pScene = GetScene(_id);
	if (NULL == pScene)
	{
		return "未知地图";
	}
	const LuaMapInfo* pMapInfo = m_xMapConfigManager.GetLuaMapInfo(pScene->GetMapResID());
	if (NULL == pMapInfo ||
		strlen(pMapInfo->szMapChName) == 0)
	{
		return "未知地图";
	}
	return pMapInfo->szMapChName;
}

GameObject* GameSceneManager::GetPlayerByName(const char* _pszName)
{
	GameObject* pObj = NULL;

	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			pObj = m_pScenes[i]->GetPlayerByName(_pszName);
			if(pObj)
			{
				return pObj;
			}
		}
		else
		{
			break;
		}
	}

	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pObj = pScene->GetPlayerByName(_pszName);
			if (NULL != pObj)
			{
				return pObj;
			}
		}
	}

	return NULL;
}

int GameSceneManager::CountPlayer()
{
	int nPlayerCounter = 0;

	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			nPlayerCounter += m_pScenes[i]->GetPlayerCount();
		}
		else
		{
			break;
		}
	}

	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			nPlayerCounter += pScene->GetPlayerCount();
		}
	}

	return nPlayerCounter;
}

int GameSceneManager::CountMonster()
{
	int nMonsterCounter = 0;

	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			nMonsterCounter += m_pScenes[i]->GetMonsterCount();
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			nMonsterCounter += pScene->GetMonsterCount();
		}
	}

	return nMonsterCounter;
}

int GameSceneManager::GetMonsterSum(int _id)
{
	int nMonsterCounter = 0;

	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			nMonsterCounter += m_pScenes[i]->GetMonsterSum(_id);
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			nMonsterCounter += pScene->GetMonsterSum(_id);
		}
	}

	return nMonsterCounter;
}

void GameSceneManager::AllMonsterHPToFull()
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			m_pScenes[i]->AllMonsterHPToFull();
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pScene->AllMonsterHPToFull();
		}
	}
}

void GameSceneManager::BroadcastPacketAllScene(ByteBuffer* _xBuf, DWORD _dwIgnore/* = 0*/)
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			m_pScenes[i]->BroadcastPacket(_xBuf, _dwIgnore);
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pScene->BroadcastPacket(_xBuf, _dwIgnore);
		}
	}
}

void GameSceneManager::SendRawSystemMessageAllScene(const char* _pszMsg)
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			m_pScenes[i]->BroadcastChatMessage(_pszMsg, 1);
			//m_pScenes[i]->BroadcastSceneSystemMessage(_pszMsg);
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pScene->BroadcastChatMessage(_pszMsg, 1);
		}
	}
}

void GameSceneManager::SendSystemMessageAllScene(const char* _pszMsg)
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			//m_pScenes[i]->BroadcastChatMessage(_pszMsg, 1);
			m_pScenes[i]->BroadcastSceneSystemMessage(_pszMsg);
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pScene->BroadcastSceneSystemMessage(_pszMsg);
		}
	}
}

void GameSceneManager::SendSystemNotifyAllScene(const char* _pszMsg)
{
	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			//m_pScenes[i]->BroadcastChatMessage(_pszMsg, 1);
			m_pScenes[i]->BroadcastSystemNotify(_pszMsg);
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			pScene->BroadcastSystemNotify(_pszMsg);
		}
	}
}

GameScene* GameSceneManager::GetScene(DWORD _dwMapID)
{
	if (_dwMapID >= 0 &&
		_dwMapID < MAX_SCENE_NUMBER)
	{
		return m_pScenes[_dwMapID];
	}
	if (_dwMapID >= INSTANCE_MAPID_BEGIN)
	{
		// find scene in instance maps
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it; it != m_xInstanceScenes.end(); ++it)
		{
			GameScene* pScene = *it;
			if (pScene->GetMapID() == _dwMapID)
			{
				return pScene;
			}
		}
	}
	return NULL;
}

bool GameSceneManager::IsUserNameExist(const char* _pszName)
{
	bool bExist = false;

	for(int i = 0; i < MAX_SCENE_NUMBER; ++i)
	{
		if(m_pScenes[i])
		{
			if(m_pScenes[i]->IsUserNameExist(_pszName))
			{
				bExist = true;
				break;
			}
		}
		else
		{
			break;
		}
	}
	if (!m_xInstanceScenes.empty())
	{
		GameSceneList::const_iterator it = m_xInstanceScenes.begin();
		for (it;
			it != m_xInstanceScenes.end();
			++it)
		{
			GameScene* pScene = *it;
			if (pScene->IsUserNameExist(_pszName))
			{
				bExist = true;
				break;
			}
		}
	}

	return bExist;
}

GameScene* GameSceneManager::CreateInstanceScene(int _nResID)
{
	// get base map info
	const LuaMapInfo* pMapInfo = m_xMapConfigManager.GetLuaMapInfo(_nResID);
	if (NULL == pMapInfo)
	{
		LOG(ERROR) << "Can't find map info : " << _nResID;
		return NULL;
	}
	if (pMapInfo->nMapType == 0)
	{
		// normal map
		LOG(ERROR) << "Trying to create instance map with fixed map type : " << _nResID;
		return NULL;
	}
	// loop all instance map
	int nSceneCount = 0;
	GameSceneList::const_iterator it = m_xInstanceScenes.begin();
	for (it; it != m_xInstanceScenes.end(); ++it)
	{
		GameScene* pScene = *it;
		if (pScene->GetMapResID() == _nResID)
		{
			if (pScene->GetPlayerSum() == 0 &&
				pScene->GetSlaveSum() == 0)
			{
				// available scene
				return pScene;
			}
			++nSceneCount;
		}
	}

	// create a new scene
	if (nSceneCount > DEFAULT_MAX_COUNT_INSTANCE)
	{
		// out of limit
		return NULL;
	}

	GameScene* pScene = new GameScene;
	m_xInstanceScenes.push_back(pScene);
	if (!pScene->Initialize(_nResID, m_dwInstanceMapIDSeed++))
	{
		delete pScene;
		pScene = NULL;
		m_xInstanceScenes.pop_back();
	}
	return pScene;
}