// [cs2auto.cpp]
// Copyright (C) [2025] [IQ Online Studio / PerryDing]
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <windows.h>
#include <cctype>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <string>
#include <regex>
#define i_input _color(14);printf(">");_color();

using namespace std;
namespace fs = filesystem;
bool showtips = true;
string Cs2autoVer = "1.1";
string Cs2auto = "// CS2 Auto Message Sender " + Cs2autoVer + " by IQ Online Studio, github.com/iqonli/cs2auto";

// 将宽字符串转换为UTF-8字节流
vector<BYTE> WideToUTF8(const wstring& wstr)
{
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::vector<BYTE> utf8Bytes(bufferSize);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, (LPSTR)utf8Bytes.data(), bufferSize, nullptr, nullptr);
	utf8Bytes.pop_back(); // 移除自动添加的null终止符
	return utf8Bytes;
}

// 将普通字符串转换为UTF-8字节流
vector<BYTE> StringToUTF8(const string& str)
{
	// 先转换为宽字符串
	int wideSize = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
	std::vector<WCHAR> wideStr(wideSize);
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wideStr.data(), wideSize);
	
	// 再转换为UTF-8
	return WideToUTF8(wstring(wideStr.data()));
}

void _color(int __c=7)//着色
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), __c);
}

// 组信息结构体
struct Group
{
	string name;
	vector<string> lines;
	char bindKey = '\0';
	bool isRandom = false;
	bool isTeam = false;
};

// 字符串分割工具
vector<string> Split(const string& s, char delim)
{
	vector<string> res;
	stringstream ss(s);
	string part;
	while (getline(ss, part, delim))
	{
		if (!part.empty()) res.push_back(part);
	}
	return res;
}

string EscapeMessage(const string& msg, const string& groupName, int lineNumber, bool escapeEnabled)
{
	if (!escapeEnabled)
	{
		return msg;
	}
	
	string escaped;
	bool hasDoubleQuote = false;
	bool hasBackslash = false;
	
	for (char c : msg)
	{
		if (c == '"')
		{
			escaped += "“";  // 全角前引号
			hasDoubleQuote = true;
		}
		else if (c == '\\')
		{
			escaped += "/";   // 正斜杠
			hasBackslash = true;
		}
		else
		{
			escaped += c;
		}
	}
	
	// 分别提示存在双引号和反斜杠并已转换
	if (hasDoubleQuote)
	{
		_color(11);
		cout << "组[" << groupName << "].line " << lineNumber << "=[" << msg << "]中存在非法字符\",已转换。" << endl;
		_color();
	}
	
	if (hasBackslash)
	{
		_color(11);
		cout << "组[" << groupName << "].line " << lineNumber << "=[" << msg << "]中存在非法字符\\,已转换。" << endl;
		_color();
	}
	
	return escaped;
}

// 从同目录txt读取组信息
vector<Group> ReadGroups(const string& filename)
{
	vector<Group> groups;
	ifstream file(filename);
	if (!file.is_open()) return groups;
	
	Group current;
	string line;
	while (getline(file, line))
	{
		if (line.empty() || line[0] == ';') continue;
		if (line[0] == ':')
		{
			if (!current.name.empty())
			{
				groups.push_back(current);
				current = Group();
			}
			vector<string> parts = Split(line.substr(1), ',');
			
			// 校验部分数量
			if (parts.size() < 3)
			{
				_color(78);
				cerr << "错误:组定义必须包含三个部分 - 组名,order/random,team/all\n请手动修改groups.txt" << endl;
				_color();
				system("pause");
				exit(1);
			}
			
			// 校验组名
			string name = parts[0];
			if (name.empty())
			{
				_color(78);
				cerr << "错误:组名不能为空\n请手动修改groups.txt" << endl;
				_color();
				system("pause");
				exit(1);
			}
			for (char c : name)
			{
				if (!isalnum(c) && c != '_')
				{
					_color(78);
					cerr << "错误:组名[" << name << "]包含非法字符,只允许包含大小写字母,数字和下划线\n请手动修改groups.txt" << endl;
					_color();
					system("pause");
					exit(1);
				}
			}
			
			// 校验随机属性
			string randomAttr = parts[1];
			for (char& c : randomAttr) c = tolower(c);
			if (randomAttr != "random" && randomAttr != "order")
			{
				_color(78);
				cerr << "错误:组["<<name<<"]的第一个属性[" << parts[1] << "]无效,只允许为order/random\n请手动修改groups.txt" << endl;
				_color();
				system("pause");
				exit(1);
			}
			
			// 校验团队属性
			string teamAttr = parts[2];
			for (char& c : teamAttr) c = tolower(c);
			if (teamAttr != "team" && teamAttr != "all")
			{
				_color(78);
				cerr << "错误:组["<<name<<"]的第二个属性[" << parts[2] << "]无效,只允许为team/all\n请手动修改groups.txt" << endl;
				_color();
				system("pause");
				exit(1);
			}
			
			// 赋值
			current.name = name;
			current.isRandom = (randomAttr == "random");
			current.isTeam = (teamAttr == "team");
		}
		else
		{
			current.lines.push_back(line);
		}
	}
	if (!current.name.empty()) groups.push_back(current);
	return groups;
}

// 自动检测CS2路径
string FindCS2Path()
{
	string steamPath;
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		char buf[MAX_PATH] = {0};
		DWORD bufSize = sizeof(buf);
		if (RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr, (LPBYTE)buf, &bufSize) == ERROR_SUCCESS)
		{
			steamPath = buf;
		}
		RegCloseKey(hKey);
	}
	
	if (steamPath.empty()) return "";
	
	vector<string> libraries = {steamPath};
	fs::path libPath = fs::path(steamPath) / "steamapps" / "libraryfolders.vdf";
	if (fs::exists(libPath))
	{
		ifstream libFile(libPath);
		string line;
		while (getline(libFile, line))
		{
			if (line.find("\"path\"") != string::npos)
			{
				size_t start = line.find('"', line.find("\"path\"") + 6) + 1;
				size_t end = line.find('"', start);
				if (start < end)
				{
					string path = line.substr(start, end - start);
					if (fs::exists(path)) libraries.push_back(path);
				}
			}
		}
	}
	
	const vector<string> csPaths =
	{
		"steamapps/common/Counter-Strike Global Offensive",
		"steamapps/common/Counter-Strike 2"
	};
	for (const auto& lib : libraries)
	{
		for (const auto& sub : csPaths)
		{
			fs::path full = fs::path(lib) / sub;
			if (fs::exists(full) && fs::exists(full / "game" / "csgo" / "pak01_dir.vpk"))
			{
				return full.string();
			}
		}
	}
	return "";
}

// 分开方法：生成组对应的CFG文件（单独句子文件+选择器）
void GenerateGroupCFGsSeparate(const Group& group, const fs::path& cfgDir, const string& progName, bool escapeEnabled)
{
	if (group.lines.empty()) return;
	string sayCmd = group.isTeam ? "say_team" : "say";
	
	vector<string> useLines = group.lines;  // 复制原始句子
	if (group.isRandom)
	{
		// 用当前时间做种子，确保每次生成打乱顺序不同
		static mt19937 rng(time(nullptr));
		shuffle(useLines.begin(), useLines.end(), rng);
	}
	size_t count = useLines.size();  // 使用处理后的句子列表
	
	// 生成句子文件
	for (size_t i = 0; i < count; ++i)
	{
		string filename = progName + "_" + group.name + "_" + to_string(i + 1) + ".cfg";
		ofstream f(cfgDir / filename, ios::binary);  // 二进制模式写入
		if (f.is_open())
		{
			// 转换为UTF-8字节流并写入
			string content = "// cs2auto - group [" + group.name + "].line " + to_string(i + 1) + "\n"
			+ sayCmd + " \"" + EscapeMessage(useLines[i], group.name, i + 1, escapeEnabled) + "\"\n"
			+ Cs2auto + "\n";
			auto utf8Data = StringToUTF8(content);
			f.write((const char*)utf8Data.data(), utf8Data.size());
		}
	}
	
	// 生成选择器文件
	string selectorName = progName + "_" + group.name + "_selector.cfg";
	ofstream sel(cfgDir / selectorName, ios::binary);  // 二进制模式写入
	if (sel.is_open())
	{
		string content = "// cs2auto selector for group [" + group.name
		+ "], [" + (group.isRandom ? "random" : "order")
		+ "][" + (group.isTeam ? "team" : "all") + "]\n";
		
		// 顺序模式的alias链
		for (size_t i = 0; i < count; ++i)
		{
			size_t next = (i + 1) % count;
			content += "alias " + progName + "_" + group.name + "_" + to_string(i)
			+ " \"exec " + progName + "_" + group.name + "_" + to_string(i + 1)
			+ "; alias " + progName + "_" + group.name + "_next " + progName + "_" + group.name + "_" + to_string(next) + "\"\n";
		}
		content += "alias " + progName + "_" + group.name + "_next " + progName + "_" + group.name + "_0\n"
//		+ progName + "_" + group.name + "_next\n"
		+ Cs2auto + "\n";
		
		auto utf8Data = StringToUTF8(content);
		sel.write((const char*)utf8Data.data(), utf8Data.size());
	}
}

// 整合方法：生成组对应的CFG文件（直接嵌入指令）
void GenerateGroupCFGsIntegrated(const Group& group, const fs::path& cfgDir, const string& progName, bool escapeEnabled)
{
	if (group.lines.empty()) return;
	string sayCmd = group.isTeam ? "say_team" : "say";
	
	vector<string> useLines = group.lines;  // 复制原始句子
	if (group.isRandom)
	{
		// 用当前时间做种子，确保每次生成打乱顺序不同
		static mt19937 rng(time(nullptr));
		shuffle(useLines.begin(), useLines.end(), rng);
	}
	size_t count = useLines.size();  // 使用处理后的句子列表
	
	// 生成选择器文件（直接包含say指令）
	string selectorName = progName + "_" + group.name + "_selector.cfg";
	ofstream sel(cfgDir / selectorName, ios::binary);  // 二进制模式写入
	if (sel.is_open())
	{
		string content = "// cs2auto selector for group [" + group.name
		+ "], [" + (group.isRandom ? "random" : "order")
		+ "][" + (group.isTeam ? "team" : "all") + "]\n";
		
		// 直接在alias中包含say指令
		for (size_t i = 0; i < count; ++i)
		{
			size_t next = (i + 1) % count;
			content += "alias " + progName + "_" + group.name + "_" + to_string(i)
			+ " \"" + sayCmd + " \"" + EscapeMessage(useLines[i], group.name, i + 1, escapeEnabled) + "\""
			+ "; alias " + progName + "_" + group.name + "_next " + progName + "_" + group.name + "_" + to_string(next) + "\"\n";
		}
		content += "alias " + progName + "_" + group.name + "_next " + progName + "_" + group.name + "_0\n"
//		+ progName + "_" + group.name + "_next\n"
		+ Cs2auto + "\n";
		
		auto utf8Data = StringToUTF8(content);
		sel.write((const char*)utf8Data.data(), utf8Data.size());
	}
}

// 生成管理器和autoexec.cfg
void GenerateManagerAndAutoexec(const vector<Group>& groups, const fs::path& cfgDir, const string& progName)
{
	string managerName = progName + "_manager.cfg";
	ofstream manager(cfgDir / managerName, ios::binary);  // 二进制模式写入
	if (manager.is_open())
	{
		string content = "// cs2auto key bindings and alias definitions\n";
		
		// 先执行每个组的selector.cfg，注册alias
		for (const auto& g : groups)
		{
			if (!g.lines.empty())  // 只处理有内容的组
			{
				string selector = progName + "_" + g.name + "_selector.cfg";
				content += "exec " + selector + "\n";  // 执行selector文件，加载alias
			}
		}
		
		// 再绑定键位（此时alias已注册）
		content += "\n// Key bindings\n";
		for (const auto& g : groups)
		{
			if (g.bindKey != '\0')
			{
				string trigger = progName + "_" + g.name + "_next";
				content += "bind \"" + string(1, g.bindKey) + "\" \"" + trigger + "\"\n";
			}
		}
		content += Cs2auto + "\n";
		
		auto utf8Data = StringToUTF8(content);
		manager.write((const char*)utf8Data.data(), utf8Data.size());
	}
	
	fs::path autoexec = cfgDir / "autoexec.cfg";
	vector<string> lines;
	if (fs::exists(autoexec))
	{
		ifstream in(autoexec);
		string line;
		while (getline(in, line)) lines.push_back(line);
	}
	
	size_t start = -1, end = -1;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if (lines[i] == "// cs2auto start") start = i;
		if (lines[i] == "// cs2auto end" && start != -1)
		{
			end = i;
			break;
		}
	}
	if (start != -1 && end != -1)
	{
		lines.erase(lines.begin() + start, lines.begin() + end + 1);
	}
	lines.push_back("// cs2auto start");
	lines.push_back("exec " + managerName);
	lines.push_back("// cs2auto end");
	// 添加提示信息到autoexec.cfg
	lines.push_back(Cs2auto);
	
	ofstream out(autoexec, ios::binary);  // 二进制模式写入
	if (out.is_open())
	{
		string content;
		for (const auto& line : lines)
		{
			content += line + "\n";
		}
		auto utf8Data = StringToUTF8(content);
		out.write((const char*)utf8Data.data(), utf8Data.size());
	}
}

// 清空生成的CFG文件
void ClearCFGs(const fs::path& cfgDir, const string& progName)
{
	if (!fs::exists(cfgDir)) return;
	
	string prefix = progName + "_";
	string suffix = ".cfg";
	size_t prefixLen = prefix.size();
	size_t suffixLen = suffix.size();
	
	for (const auto& entry : fs::directory_iterator(cfgDir))
	{
		string name = entry.path().filename().string();
		bool hasPrefix = (name.size() >= prefixLen) && (name.substr(0, prefixLen) == prefix);
		bool hasSuffix = (name.size() >= suffixLen) && (name.substr(name.size() - suffixLen) == suffix);
		if (hasPrefix && hasSuffix)
		{
			fs::remove(entry);
		}
	}
	
	fs::remove(cfgDir / (progName + "_manager.cfg"));
	
	fs::path autoexec = cfgDir / "autoexec.cfg";
	if (!fs::exists(autoexec)) return;
	
	vector<string> lines;
	ifstream in(autoexec);
	string line;
	while (getline(in, line)) lines.push_back(line);
	
	size_t start = -1, end = -1;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if (lines[i] == "// cs2auto start") start = i;
		if (lines[i] == "// cs2auto end" && start != -1)
		{
			end = i;
			break;
		}
	}
	if (start != -1 && end != -1)
	{
		lines.erase(lines.begin() + start, lines.begin() + end + 1);
	}
	
	// 使用正则表达式匹配所有相关标识行
	const regex pattern(R"(^// CS2 Auto Message Sender.*)");
	
	// 循环删除所有匹配的行
	for (size_t i = 0; i < lines.size(); )
	{
		if (regex_match(lines[i], pattern))
		{
			lines.erase(lines.begin() + i);
		}
		else
		{
			i++;
		}
	}
	
	ofstream out(autoexec, ios::binary);  // 二进制模式写入
	if (out.is_open())
	{
		string content;
		for (const auto& l : lines)
		{
			content += l + "\n";
		}
		auto utf8Data = StringToUTF8(content);
		out.write((const char*)utf8Data.data(), utf8Data.size());
	}
}

// 手动设置CS2路径
string SetCS2PathManual()
{
	string path;
	cout << "输入CS2根目录(如：C:\\Steam\\steamapps\\common\\Counter-Strike 2)";i_input
	getline(cin, path);
	fs::path p(path);
	if (fs::exists(p) && fs::exists(p / "game" / "csgo" / "pak01_dir.vpk"))
	{
		return path;
	}
	_color(78);
	cout << "无效路径(未找到CS2特征文件)\n";
	_color();
	return "";
}

// 绑定组键位
void BindGroupKeys(vector<Group>& groups)
{
	for (auto& g : groups)
	{
		cout << "\n为组[" << g.name << "]("
		<< (g.isRandom ? "随机" : "顺序") << ","
		<< (g.isTeam ? "TEAM" : "ALL") << ")绑定键位(字母/数字,不区分大小写,留空不绑定)\n";i_input
		string input;
		getline(cin, input);
		if (input.size() == 1 && isalnum(input[0]))
		{
			g.bindKey = tolower(input[0]);
		}
		else
		{
			g.bindKey = '\0';
		}
	}
	_color(160);
	cout << "键位绑定完成\n";
	_color();
}

void NormalizePathSeparators(fs::path& path)
{
	std::string pathStr = path.string();
	std::string normalized;
	
	for (size_t i = 0; i < pathStr.size(); ++i) {
		// 当前字符是路径分隔符
		if (pathStr[i] == '/' || pathStr[i] == '\\') {
			// 添加单个斜杠
			normalized += '/';
			// 跳过所有连续的分隔符
			while (i + 1 < pathStr.size() && 
				   (pathStr[i+1] == '/' || pathStr[i+1] == '\\')) {
				++i;
			}
		} else {
			// 非分隔符字符直接添加
			normalized += pathStr[i];
		}
	}
	
	// 更新路径
	path = fs::path(normalized);
}

int main()
{
	SetConsoleTitle(("CS2 Auto Message Sender " + Cs2autoVer).c_str());
	_color(10);
	cout << "欢迎来到 CS2 Auto Message Sender " + Cs2autoVer + "\n";
	_color(11);
	cout << "by IQ Online Studio, github.com/iqonli/cs2auto; used AI.\n";
	_color();
	cout << "新手请查看README.md,本程序不支持程序内编辑句子,仅支持在本地使用文本编辑器编辑groups.txt,编辑完成请重启程序\n";
	
	const string progName = "cs2auto";
	string cs2Path = FindCS2Path();
	
	if (cs2Path.empty())
	{
		_color(12);
		cout << "未自动找到CS2路径,需手动设置\n";
		_color();
		cs2Path = SetCS2PathManual();
		if (cs2Path.empty()) return 1;
	}
	fs::path cfgDir = fs::path(cs2Path) / "game" / "csgo" / "cfg";
	NormalizePathSeparators(cfgDir);
	if (!fs::exists(cfgDir)) fs::create_directories(cfgDir);
	
	vector<Group> groups = ReadGroups("groups.txt");
	if (groups.empty())
	{
		_color(12);
		cout << "未找到有效组信息,请按下面的提示修改groups.txt\n\n";
		_color(176);
		ifstream file("groups-e.g..txt");
		string content(
					   (istreambuf_iterator<char>(file)),
					   istreambuf_iterator<char>()
					   );
		
		cout << content;
		_color();
		system("pause");
		return 1;
	}
	
	BindGroupKeys(groups);
	
	while (true)
	{
		cout << "\n======== MENU ========\n";
		cout << "cfg文件夹路径:" << cfgDir << "\n";
		cout << "1=(立即应用键位)清空并重新生成所有CFG/每句话一个cfg,不易出错,方便修改\n";
		cout << "2=(立即应用键位)清空并重新生成所有CFG/所有句子整合到一个cfg,文件清爽\n";
		cout << "3=(删除键位)清空生成的CFG\n";
		cout << "4=修改CS2路径\n";
		cout << "5=重新绑定键位\n";
		cout << "6=退出\n";
		cout << "\n选择操作(1-6)";i_input
		
		int choice;
		cin >> choice;
		cin.ignore();
		
		switch (choice)
		{
			case 1:
			{
				cout << "是否转义句子中存在的危险字符?(1=启用,0=不启用)";i_input
				int escapeChoice;
				cin >> escapeChoice;
				cin.ignore();
				bool escapeEnabled = (escapeChoice == 1);
				
				ClearCFGs(cfgDir, progName);
				for (const auto& g : groups) GenerateGroupCFGsSeparate(g, cfgDir, progName, escapeEnabled);
				GenerateManagerAndAutoexec(groups, cfgDir, progName);
				_color(160);
				cout << "\n分开模式CFG已生成,随机组已打乱顺序\n";
				_color(11);
				cout << "在游戏控制台输入'exec autoexec.cfg'以刷新,注意在CS2>设置>游戏设置>游戏中启用控制台。\n";
				if (showtips)
				{
					_color(10);
					cout << "如果想每次启动CS2时自动加载功能,需要在Steam>库>CS2右键>属性>启动选项中输入:\n";
					_color(11);
					cout << "+exec autoexec.cfg\n";
					showtips=false;
				}
				_color();
				break;
			}
			case 2:
			{
				cout << "是否转义句子中存在的危险字符?(1=启用,0=不启用)";i_input
				int escapeChoice;
				cin >> escapeChoice;
				cin.ignore();
				bool escapeEnabled = (escapeChoice == 1);
				
				ClearCFGs(cfgDir, progName);
				for (const auto& g : groups) GenerateGroupCFGsIntegrated(g, cfgDir, progName, escapeEnabled);
				GenerateManagerAndAutoexec(groups, cfgDir, progName);
				_color(160);
				cout << "\n整合模式CFG已生成,随机组已打乱顺序\n";
				_color(11);
				cout << "在游戏控制台输入'exec autoexec.cfg'以刷新,注意在CS2>设置>游戏设置>游戏中启用控制台。\n";
				if (showtips)
				{
					_color(10);
					cout << "如果想每次启动CS2时自动加载功能,需要在Steam>库>CS2右键>属性>启动选项中输入:\n";
					_color(11);
					cout << "+exec autoexec.cfg\n";
					showtips=false;
				}
				_color();
				break;
			}
			case 3:
			{
				ClearCFGs(cfgDir, progName);
				_color(160);
				cout << "\n所有程序生成的CFG已清空\n";
				_color();
				break;
			}
			case 4:
			{
				cs2Path = SetCS2PathManual();
				if (!cs2Path.empty())
				{
					cfgDir = fs::path(cs2Path) / "game" / "csgo" / "cfg";
					fs::create_directories(cfgDir);
					ClearCFGs(cfgDir, progName);
					_color(160);
					cout << "\n路径已更新,请重新选择生成模式\n";
					_color();
				}
				break;
			}
			case 5:
			{
				cout << "\n重新为各组绑定键位...\n";
				BindGroupKeys(groups);
				break;
			}
			case 6:
			{
				return 0;
			}
			default:
			{
				_color(12);
				cout << "\n无效选择\n";
				_color();
			}
		}
	}
}

