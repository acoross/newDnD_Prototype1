#include "SceneDB.h"

#include "Scene.h"
#include "SceneFunctions.h"

#include "Parser.h"

namespace Scene
{
	class Token2
	{
	public:
		Token2(const wchar_t* const token, int nLen, const wchar_t* next) : wcsToken(token, nLen), nextInput(next)
		{}

		std::wstring wcsToken;
		const wchar_t* nextInput;
	};

	bool find(std::vector<wchar_t>& lst, wchar_t data)
	{
		for (auto it = lst.begin(); it != lst.end(); ++it)
		{
			if (data == *it)
			{
				return true;
			}
		}
		return false;
	}

	class CLineTokenizer	// 왼쪽에서부터 순서대로 진행하여, 구분자 또는 terminal 까지를 token 으로 얻어낸다.
	{
	public:
		std::vector<wchar_t> m_Space;
		std::vector<wchar_t> m_Terminal;
		std::vector<wchar_t> m_Seperator;

		template<std::size_t N1, std::size_t N2, std::size_t N3>
		CLineTokenizer(const wchar_t(&space)[N1], const wchar_t (&terminal)[N2], const wchar_t (&seperator)[N3])
			: m_Space(&space[0], &space[N1 - 1]), m_Terminal(&terminal[0], &terminal[N2 - 1]), m_Seperator(&seperator[0], &seperator[N3 - 1])
		{
			;
		}

		Token2 GetToken(const wchar_t* const input)
		{
			if (input == nullptr)
			{
				return Token2(L"", 0, nullptr);
			}

			int nPos = 0;	// space 제거
			while (find(m_Space, input[nPos]))
			{
				++nPos;
			} 
			
			int nStartPos = nPos;
			const wchar_t* next = nullptr;
			while (true)
			{
				if (find(m_Terminal, input[nPos]))
				{
					break;
				}

				if (find(m_Seperator, input[nPos]))
				{
					next = &input[nPos];
					break;
				}
				
				++nPos;
			}

			return Token2(&input[nStartPos], nPos - nStartPos, next);	// L'\0' 까지 갔으면 next = nullptr;
		}
	};

	class CLRTokenizer	// 시작~종료 구분자 사이의 문자를 모두 얻어냄.
	{
	public:
		wchar_t m_StartChar;
		wchar_t m_EndChar;
		std::vector<wchar_t> m_Space;
		std::vector<wchar_t> m_Terminal;
		std::vector<wchar_t> m_Seperator;

		template<std::size_t N1, std::size_t N2, std::size_t N3>
		CLRTokenizer(const wchar_t(&space)[N1], const wchar_t(&terminal)[N2], const wchar_t(&seperator)[N3])
			: m_Space(&space[0], &space[N1-1]), m_Terminal(&terminal[0], &terminal[N2 - 1]), m_Seperator(&seperator[0], &seperator[N3 - 1])
		{
		}

		Token2 GetToken(const wchar_t* const input)
		{
			if (input == nullptr)
			{
				return Token2(L"", 0, nullptr);
			}

			int nPos = 0;	// space 제거
			while (find(m_Space, input[nPos]))
			{
				++nPos;
			}

			if (input[nPos] != m_StartChar)
			{
				return Token2(L"", 0, nullptr);
			}
			++nPos;

			int nStartPos = nPos;
			int nEndPos = nPos;
			const wchar_t* next = nullptr;

			int nStartCount = 1;
			int nEndCount = 0;
			while (true)
			{
				if (find(m_Terminal, input[nPos]))
				{
					return Token2(L"", 0, nullptr);	// 중간에 terminal 나왔으므로.
				}

				if (input[nPos] == m_StartChar)
				{
					++nStartCount;
				}
				else if (input[nPos] == m_EndChar)
				{
					++nEndCount;
				}

				if (nStartCount == nEndCount)
				{					
					nEndPos = nPos;
					next = &input[nPos];
					break;
				}
				++nPos;
			}

			return Token2(&input[nStartPos], nEndPos - nStartPos, next);
		}
	};

	int ParseInt(const wchar_t* const wline)
	{
		return std::wcstol(wline, nullptr, 10);
	}

	Scene::SceneFunc ParseFunction(const wchar_t* const input);

	typedef Scene::SceneFunc(*ParserFunc)(const std::wstring&);
	
	Scene::SceneFunc Parser_RunScript(const std::wstring& input)
	{
		int nArg = ParseInt(input.c_str());
		return Scene::Func_RunScript(nArg);
	}

	Scene::SceneFunc Parser_RunBattle(const std::wstring& input)
	{
		int nArg = ParseInt(input.c_str());
		return Scene::Func_RunBattle(nArg);
	}

	Scene::SceneFunc Parser_CondCheckResist(const std::wstring& input)
	{
		CLineTokenizer lt(L" \t,", L"\0/", L",");

		Token2 arg1 = lt.GetToken(input.c_str());
		Token2 arg2 = lt.GetToken(arg1.nextInput);
		Token2 arg3 = lt.GetToken(arg2.nextInput);

		int nArg1 = ParseInt(arg1.wcsToken.c_str());
		SceneFunc fArg2 = ParseFunction(arg2.wcsToken.c_str());
		SceneFunc fArg3 = ParseFunction(arg3.wcsToken.c_str());

		return Scene::Cond_CheckPCResistance(ResistanceType(nArg1), fArg2, fArg3);
	}

////
	typedef std::unordered_map<std::wstring, ParserFunc> ParserFuncMap;

	ParserFuncMap g_ParserFuncMap;

	void InitParserFuncMap()
	{
		g_ParserFuncMap.clear();

		g_ParserFuncMap[L"runscript"] = Parser_RunScript;
		g_ParserFuncMap[L"runbattle"] = Parser_RunBattle;
		g_ParserFuncMap[L"cond_check_pc_resistance"] = Parser_CondCheckResist;
	}
////

	Scene::SceneFunc ParseFunction(const wchar_t* const input)
	{
		CLineTokenizer lt(L" \t", L"\0/", L"(");

		CLRTokenizer lrt(L" \t", L"\0/", L"");
		lrt.m_StartChar = L'(';
		lrt.m_EndChar = L')';

		Token2 nameToken = lt.GetToken(input);
		Token2 args = lrt.GetToken(nameToken.nextInput);

		int a = 1;

		auto it = g_ParserFuncMap.find(nameToken.wcsToken);
		if (it != g_ParserFuncMap.end())
		{
			return (it->second)(args.wcsToken);
		}
		else
		{
			_ASSERT(0);
			return SceneFunc(DefaultFunc);
		}
	}



	SceneDB g_SceneDB;

	SceneDB::SceneDB()
	{
	}

	SceneDB::~SceneDB()
	{
	}

	bool SceneDB::Load(const wchar_t* const filename)
	{
		InitParserFuncMap();

		using namespace Parser;

		CMultilineParser<CScene>::ParserFunc func = [](CScene& scene, std::wstring wline)
		{
			//wprintf_s(L"%s\n", wline.c_str());
			scene.m_SceneFuncList.push_back(ParseFunction(wline.c_str()));
		};

		CMultilineParser<CScene> parser;
		return parser.Load(filename, m_SceneMap, func);
	}

	void InitSceneDB_test()
	{
		/*using namespace Scene;
		CScene* pScene = new CScene();

		pScene->m_nID = 1;
		pScene->m_SceneFuncList.push_back(Func_RunScript(1));
		pScene->m_SceneFuncList.push_back(Func_RunBattle(1));
		pScene->m_SceneFuncList.push_back(Func_RunScript(7));
		pScene->m_SceneFuncList.push_back(Func_RunScript(2));
		pScene->m_SceneFuncList.push_back(Func_RunBattle(2));
		pScene->m_SceneFuncList.push_back(Func_RunScript(4));
		pScene->m_SceneFuncList.push_back(Func_RunBattle(3));
		pScene->m_SceneFuncList.push_back(Func_RunScript(11));
		pScene->m_SceneFuncList.push_back(Cond_CheckPCResistance(RT_MAGIC, Func_RunScript(6), Func_RunScript(5)));

		g_SceneDB.m_SceneMap.insert(SceneMap::value_type(pScene->m_nID, pScene));*/
	}
}