#ifndef _INC_GAMEWORLD_
#define _INC_GAMEWORLD_
//////////////////////////////////////////////////////////////////////////
#include "ObjectEngine.h"
#include "Struct.h"
#include <map>
#include <mutex>
#include "../../CommonModule/ByteBuffer.h"
#include "../../CommonModule/cron/CronSchedule.h"
#include "../common/shared.h"
#include "DBThread.h"
#include "LuaServerEngine.h"
#include "GameDbBuffer.h"
#include "WorldEventDispatcher.h"
#include "MSGStack.h"
#include "WeightCalc.h"
//////////////////////////////////////////////////////////////////////////
//	For encrypt
#ifdef _WIN32
#ifdef _THEMIDA_

#include "../Themida/ThemidaSDK.h"

#define PROTECT_START_VM	VM_START
#define PROTECT_END_VM		VM_END

#else

	#ifdef _SHIELDEN_

		#include "../Shielden/C/SESDK.h"

		#define PROTECT_START_VM	SE_PROTECT_START_VIRTUALIZATION
		#define PROTECT_END_VM		SE_PROTECT_END

	#else
		#define PROTECT_START_VM	
		#define PROTECT_END_VM
	#endif

#endif
#else
#define PROTECT_START_VM	
#define PROTECT_END_VM
#endif
//////////////////////////////////////////////////////////////////////////
#define THREAD_CALL __stdcall
//////////////////////////////////////////////////////////////////////////
typedef std::map<int, GameObject*> ObjectMap;
//////////////////////////////////////////////////////////////////////////
#define MAX_COMMAND		80960

#define ALERT_MSGBOX(T)		::MessageBox(NULL, T, "ALERT", MB_ICONERROR | MB_TASKMODAL)

#define ITEMTAG_INQUERY	0xFFFFFFFF

#define MAX_CONNECTIONS	500

enum {
	STOP_DUMMY,
	STOP_HEROSPDERR,
	STOP_HEROATTRIBERR,
	STOP_EXETOOLONG,
	STOP_HPZERONOTDEAD,
	STOP_MONSNOTDEAD,
};
//////////////////////////////////////////////////////////////////////////
extern HeroObject* g_pxHeros[MAX_CONNECTIONS + 1];
extern std::vector<MagicInfo> g_xMagicInfoTable;
extern std::map<int, int> g_xShowDonateTimeMap;
//////////////////////////////////////////////////////////////////////////
enum WORLD_STATE
{
	WS_STOP,
	WS_WORKING,
	WS_PAUSE,
};

enum BUFFER_OPERATE
{
	BO_NEWOBJECT = 0,
	BO_DELOBJECT,
};

struct BlackListItem
{
	std::string xName;
	unsigned char bJob;
	int nLevel;
	unsigned char bSex;
};

typedef std::list<BlackListItem> BlackList;
//////////////////////////////////////////////////////////////////////////
class GameWorld
{
public:
	GameWorld();
	~GameWorld();

public:
	static GameWorld* GetInstancePtr();
	static GameWorld& GetInstance()
	{
		return *GetInstancePtr();
	}
	static void DestroyInstance();

private:
	static unsigned int THREAD_CALL WorkThread(void* _pData);
	static int __cronActive(int _nJobId, int _nArg);

public:
	//	timer process
	unsigned int WorldRun();
	unsigned int ProcessThreadMsg();
	int Init();
	unsigned int Run();
	void Stop()
	{
		if(m_eState == WS_STOP)
		{
			return;
		}
		m_bTerminate = true;
	}
	void Stop(int _ret)
	{
		return;
		if(m_eState == WS_STOP)
		{
			return;
		}
		m_nRetCode = _ret;
		m_bTerminate = true;
	}
	void Terminate(int _nRet)
	{
		if(m_eState == WS_STOP)
		{
			return;
		}
		m_nRetCode = _nRet;
		m_bTerminate = true;
	}
	void Pause()
	{
		if(m_eState == WS_WORKING)
		{
			m_bPause = true;
		}
	}
	void Resume()
	{
		if(m_eState == WS_PAUSE)
		{
			m_bPause = false;
		}
	}
	inline WORLD_STATE GetWorldState()
	{
		return m_eState;
	}
	void Join();

	//	get the id
	inline unsigned int GenerateObjectID()
	{
		static unsigned int s_uID = 0;
		return ++s_uID;
	}
	inline unsigned int GenerateItemTag()
	{
		static unsigned int s_uID = 0;
		return ++s_uID;
	}
	inline unsigned int GenerateMapID()
	{
		static unsigned int s_uID = 0;
		return ++s_uID;
	}

	inline bool GetThreadRunMode() {
		return m_bThreadRunMode;
	}

	//	sync object
	inline void LockProcess()
	{
		m_stCsProcess.lock();
	}
	inline void UnlockProcess()
	{
		m_stCsProcess.unlock();
	}

	//	Only for main thread
	void OnMessage(unsigned int _uId, const void* _pData, unsigned int _uLen);
	void OnMessage(unsigned int _dwIndex, unsigned int _uId, ByteBuffer* _xBuffer);
	void AddDelayedProcess(const DelayedProcess* _pDp);
	//	can't call on main thread
	GameObject* GetPlayer(unsigned int _uId);
	GameObject* GetNPC(unsigned int _uId);

	//	Broadcast msg
	unsigned int Broadcast(ByteBuffer* _pBuf);
	unsigned int BroadcastExcept(GameObject* _pObj, ByteBuffer* _pBuf);
	unsigned int BroadcastRange(ByteBuffer* _pBuf, unsigned int _dwSrcX, unsigned int _dwSrcY, unsigned int _dwOftX = 10, unsigned int _dwOftY = 10);
	unsigned int BroadcastRangeExcept(GameObject* _pObj, ByteBuffer* _pBuf, unsigned int _dwSrcX, unsigned int _dwSrcY, unsigned int _dwOftX = 10, unsigned int _dwOftY = 10);

	//	script function
	/*inline ScriptEngine* GetScript()
	{
		return &m_xScript;
	}*/
	inline LuaServerEngine* GetLuaEngine()
	{
		return &m_xScript;
	}
	inline lua_State* GetLuaState()
	{
		return m_xScript.GetVM();
	}
	inline void DispatchLuaEvent(int _nEvtId, void* _pEvt)
	{
		m_xScript.DispatchEvent(_nEvtId, _pEvt);
	}
	bool LoadScript();
	bool ReloadScript();

	inline const char* GetRankListData()
	{
		return m_pRankListData;
	}

	void PostRunMessage(const MSG* _pMsg);

	void SetSchedule(int _nEventId, const char* _pszCronExpr);
	void ResetSchedule(int _nEventId);

	//	sync process
	int SyncOnHeroDisconnected(HeroObject* _pHero);
	int SyncOnHeroConnected(HeroObject* _pHero, bool _bNew);
	int SyncOnHeroMsg(HeroObject* _pHero, ByteBuffer& _refBuf);
	int SyncIsHeroExists(LoginQueryInfo* _pQuery);
	int SyncGetPlayerIPCount(const std::string& _xIP);

	// addition point calc
	bool LoadAdditionPointCalcDataFromScript();
	WeightCalc& GetAdditionPointCalc();
	void SetAdditionPointWeight(int _nPoint, int _nWeight);
	void AddAdditionPointWeight(int _nWeight, int _nPoint);

public:
	//	working process
	void DoWork_Objects(unsigned int _dwTick);
	void DoWork_System(unsigned int _dwTick);
	void DoWork_DelayedProcess(unsigned int _dwTick);
	unsigned int Thread_ProcessMessage(const MSG* _pMsg);

private:
	bool NewPlayer(GameObject* _pObj);
	bool RemovePlayer(unsigned int _uId);
	bool NewNPC(unsigned int _uId);
	void RemoveNPC(unsigned int _uId);

private:
	void DoDelayGame(const DelayedProcess& _dp);
	void DoDelayDatabase(const DelayedProcess& _dp);
	void DoDelaySystem(const DelayedProcess& _dp);

public:
	bool LoadMagicInfo();
	void UpgradeItems(ItemAttrib* _pItem);
	void UpgradeItems(ItemAttrib* _pItem, int _nProb);
	bool UpgradeAttrib(ItemAttrib* _pItem, int _index, int _value);
	void UpgradeItemsWithAddition(ItemAttrib* _pItem, int _nAddition);
	void SetItemHideAttrib(ItemAttrib* _pItem);
	void SetMaterialItemQuality(ItemAttrib* _pItem);

public:
	inline int GetOnlinePlayerCount()
	{
		return m_nOnlinePlayers;
	}
	inline int GetExprMultiple()	{return m_nExprMultiple;}
	inline int GetDropMultiple()	{return m_nDropMultiple;}
	inline void SetExprMultiple(int _nMult)			{m_nExprMultiple = _nMult;}
	inline void SetDropMultiple(int _nMult)			{m_nDropMultiple = _nMult;}
	inline void EnableAutoReset()					{m_bEnableAutoReset = true;}
	inline void DisableAutoReset()					{m_bEnableAutoReset = false;}
	inline bool GetAutoReset()						{return m_bEnableAutoReset;}

	inline bool IsGenElitMons()						{return m_bGenElitMons;}
	inline void SetGenElitMons(bool _b)				{m_bGenElitMons = _b;}

	inline bool IsEnableOfflineSell()				{return m_bEnableOfflineSell;}
	inline void SetEnableOfflineSell(bool _b)		{m_bEnableOfflineSell = _b;}

	inline bool IsEnableWorldNotify()				{return m_bEnableWorldNotify;}
	inline void SetEnableWorldNotify(bool _b)		{m_bEnableWorldNotify = _b;}

	inline void SetFinnalExprMulti(int _nMulti)		{m_nFinnalExprMulti = _nMulti;}
	inline int GetFinnalExprMulti()					{return m_nFinnalExprMulti;}

	bool IsInBlackList(HeroObject* _pHero);

	inline std::string& GetExpFireworkUserName()	{return m_xExpFireworkUserName;}
	inline std::string& GetBurstFireworkUserName()	{return m_xBurstFireworkUserName;}
	inline std::string& GetMagicDropFireworkUserName()	{return m_xMagicDropFireworkUserName;}
	inline void SetExpFireworkUserName(const char* _pszUserName){
		m_xExpFireworkUserName = _pszUserName;
	}
	inline void SetBurstFireworkUserName(const char* _pszUserName){
		m_xBurstFireworkUserName = _pszUserName;
	}
	inline void SetMagicDropFireworkUserName(const char* _pszUserName){
		m_xMagicDropFireworkUserName = _pszUserName;
	}
	inline unsigned int GetExpFireworkTime()				{ return m_dwLastExpFireworkTime; }
	inline void SetExpFireworkTime(unsigned int _dwTime)	{ m_dwLastExpFireworkTime = _dwTime; }
	inline unsigned int GetBurstFireworkTime()				{ return m_dwLastBurstFireworkTime; }
	inline void SetBurstFireworkTime(unsigned int _dwTime)	{ m_dwLastBurstFireworkTime = _dwTime; }
	inline unsigned int GetMagicDropFireworkTime()			{ return m_dwLastMagicDropFireworkTime; }
	inline void SetMagicDropFireworkTime(unsigned int _dwTime)	{ m_dwLastMagicDropFireworkTime = _dwTime; }

	inline int GetDifficultyLevel()					{return m_nDifficultyLevel;}
	inline void SetDifficultyLevel(int _nLevel)		{m_nDifficultyLevel = _nLevel;}

private:
	bool m_bTerminate;
	bool m_bPause;
	WORLD_STATE m_eState;

	//	Monsters and NPC
	ObjectMap m_xNPC;
	//	Players
	ObjectMap m_xPlayers;

	//	For working thread
	unsigned int m_dwThreadID;
	HANDLE m_hThread;
	unsigned int m_dwWorkTotalTime;

	//	delayed command buffer
	ByteBuffer m_xProcessD;

	//	Database thread
	DBThread* m_xDatabase;
	//	World script
	LuaServerEngine m_xScript;

	static int m_nRetCode;

	//	Expr multiple and Drop multiple
	int m_nDropMultiple;
	int m_nExprMultiple;
	//	Special map support resetting?
	bool m_bEnableAutoReset;

	BlackList m_xBlackList;
	//	Last receive watcher msg time
	unsigned int m_dwLastRecWatcherMsgTime;

	bool m_bGenElitMons;
	bool m_bEnableOfflineSell;
	bool m_bEnableWorldNotify;

	int m_nFinnalExprMulti;

	//	排行榜的字符串指针 new出来 需要delete
	char* m_pRankListData;

	//	放烟花产生的状态
	unsigned int m_dwLastExpFireworkTime;
	std::string m_xExpFireworkUserName;
	int m_nExpFireworkUID;

	unsigned int m_dwLastBurstFireworkTime;
	std::string m_xBurstFireworkUserName;
	int m_nBurstFireworkUID;

	unsigned int m_dwLastMagicDropFireworkTime;
	std::string m_xMagicDropFireworkUserName;
	int m_nMagicDropFireworkUID;

	//	更新脚本引擎的时间
	unsigned int m_dwLastUpdateScriptEngineTime;

	//	游戏难度
	int m_nDifficultyLevel;

	//	是否运行于线程模式
	bool m_bThreadRunMode;
	bool m_bWorldInit;

	//	消息队列
	MSGStack m_xMsgStack;
	std::mutex m_csMsgStack;

	// 极品权重
	WeightCalc m_xAdditionPointCalc;

protected:
	std::mutex m_stCsProcess;

public:
	//	For interlock use
	int m_nOnlinePlayers;

	//	cron调度器
	CronJobScheduler m_xCronScheduler;
};
//////////////////////////////////////////////////////////////////////////
#endif