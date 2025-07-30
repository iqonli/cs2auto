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
#define i_input _color(14);printf(">");_color();

bool showtips = true;

using namespace std;
namespace fs = filesystem;

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

// 转义消息中的双引号
string EscapeMessage(const string& msg)
{
	string escaped;
	for (char c : msg)
	{
		if (c == '"') escaped += "\"\"";
		else escaped += c;
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
					cerr << "错误:组名 '" << name << "' 包含非法字符,只允许包含大小写字母,数字和下划线\n请手动修改groups.txt" << endl;
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
				cerr << "错误:组 '"<<name<<"' 的属性 '" << parts[1] << "' 无效,只允许为order/random\n请手动修改groups.txt" << endl;
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
				cerr << "错误:组 '"<<name<<"' 的属性 '" << parts[2] << "' 无效,只允许为team/all\n请手动修改groups.txt" << endl;
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

// 生成组对应的CFG文件
void GenerateGroupCFGs(const Group& group, const fs::path& cfgDir, const string& progName)
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
	
	// 生成句子文件（改用打乱后的useLines）
	for (size_t i = 0; i < count; ++i)
	{
		string filename = progName + "_" + group.name + "_" + to_string(i + 1) + ".cfg";
		ofstream f(cfgDir / filename);
		if (f.is_open())
		{
			f << "// cs2auto generated - " << group.name << " line " << i + 1 << "\n";
			f << sayCmd << " \"" << EscapeMessage(useLines[i]) << "\"\n";
			// 添加提示信息
			f << "// CS2 Auto Message Sender by IQ Online Studio, github.com/iqonli/cs2auto\n";
		}
	}
	
	// 生成选择器文件（随机组复用顺序模式的alias逻辑）
	string selectorName = progName + "_" + group.name + "_selector.cfg";
	ofstream sel(cfgDir / selectorName);
	if (sel.is_open())
	{
		sel << "// cs2auto selector for group: " << group.name
		<< " (mode: " << (group.isRandom ? "random" : "order")
		<< ", target: " << (group.isTeam ? "team" : "all") << ")\n";
		
		// 统一使用顺序模式的alias链（因为随机组已提前打乱顺序）
		for (size_t i = 0; i < count; ++i)
		{
			size_t next = (i + 1) % count;
			sel << "alias " << progName << "_" << group.name << "_" << i
			<< " \"exec " << progName << "_" << group.name << "_" << i + 1
			<< "; alias " << progName << "_" << group.name << "_next " << progName << "_" << group.name << "_" << next << "\"\n";
		}
		sel << "alias " << progName << "_" << group.name << "_next " << progName << "_" << group.name << "_0\n";
		sel << progName << "_" << group.name << "_next\n";
		// 添加提示信息
		sel << "// CS2 Auto Message Sender by IQ Online Studio, github.com/iqonli/cs2auto\n";
	}
}

// 生成管理器和autoexec.cfg
void GenerateManagerAndAutoexec(const vector<Group>& groups, const fs::path& cfgDir, const string& progName)
{
	string managerName = progName + "_manager.cfg";
	ofstream manager(cfgDir / managerName);
	if (manager.is_open())
	{
		manager << "// cs2auto key bindings and alias definitions\n";
		
		// 先执行每个组的selector.cfg，注册alias
		for (const auto& g : groups)
		{
			if (!g.lines.empty())  // 只处理有内容的组
			{
				string selector = progName + "_" + g.name + "_selector.cfg";
				manager << "exec " << selector << "\n";  // 执行selector文件，加载alias
			}
		}
		
		// 再绑定键位（此时alias已注册）
		manager << "\n// Key bindings\n";
		for (const auto& g : groups)
		{
			if (g.bindKey != '\0')
			{
				string trigger = progName + "_" + g.name + "_next";
				manager << "bind \"" << g.bindKey << "\" \"" << trigger << "\"\n";
			}
		}
		// 添加提示信息
		manager << "// CS2 Auto Message Sender by IQ Online Studio, github.com/iqonli/cs2auto\n";
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
	lines.push_back("// CS2 Auto Message Sender by IQ Online Studio, github.com/iqonli/cs2auto");
	
	ofstream out(autoexec);
	for (const auto& line : lines) out << line << "\n";
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
	
	for (size_t i = 0; i < lines.size(); ++i)
	{
		if (lines[i] == "// CS2 Auto Message Sender by IQ Online Studio, github.com/iqonli/cs2auto")
		{
			lines.erase(lines.begin() + i);
			break;
		}
	}
	
	ofstream out(autoexec);
	for (const auto& l : lines) out << l << "\n";
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
		cout << "为组[" << g.name << "] ("
		<< (g.isRandom ? "随机" : "顺序") << ","
		<< (g.isTeam ? "TEAM" : "ALL") << ") 绑定键位(字母/数字,留空不绑定)";i_input
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
	SetConsoleTitle("CS2 Auto Message Sender");
	_color(10);
	cout << "欢迎来到 CS2 Auto Message Sender\n";
	_color(11);
	cout << "by IQ Online Studio, github.com/iqonli; used AI.\n";
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
		cout << "1=(立即应用键位)清空并重新生成所有CFG\n";
		cout << "2=(删除键位)清空生成的CFG\n";
		cout << "3=修改CS2路径\n";
		cout << "4=重新绑定键位\n";
		cout << "5=退出\n";
		cout << "选择操作(1-5)";i_input
		
		int choice;
		cin >> choice;
		cin.ignore();
		
		switch (choice)
		{
		case 1:
			ClearCFGs(cfgDir, progName);
			for (const auto& g : groups) GenerateGroupCFGs(g, cfgDir, progName);
			GenerateManagerAndAutoexec(groups, cfgDir, progName);
			_color(160);
			cout << "CFG已重新生成,随机组已打乱顺序\n";
			_color(11);
			cout << "在游戏控制台输入 'exec autoexec.cfg' 以刷新,注意在CS2>设置>游戏设置>游戏中启用控制台。\n";
			if (showtips)
			{
				_color(10);
				cout << "如果想每次启动CS2时自动加载功能,需要在Steam>库>CS2右键>属性>高级用户可以选择输入对启动选项的修改的框里面输入:\n";
				_color(11);
				cout << "+exec autoexec.cfg\n";
				showtips=false;
			}
			_color();
			break;
		case 2:
			ClearCFGs(cfgDir, progName);
			_color(160);
			cout << "所有程序生成的CFG已清空\n";
			_color();
			break;
		case 3:
			cs2Path = SetCS2PathManual();
			if (!cs2Path.empty())
			{
				cfgDir = fs::path(cs2Path) / "game" / "csgo" / "cfg";
				fs::create_directories(cfgDir);
				ClearCFGs(cfgDir, progName);
				for (const auto& g : groups) GenerateGroupCFGs(g, cfgDir, progName);
				GenerateManagerAndAutoexec(groups, cfgDir, progName);
				_color(160);
				cout << "路径已更新,CFG已重新生成,随机组已打乱顺序\n";
				_color();
			}
			break;
		case 4:
			cout << "\n重新为各组绑定键位...\n";
			BindGroupKeys(groups);
			break;
		case 5:
			return 0;
		default:
			_color(12);
			cout << "无效选择\n";
			_color();
		}
	}
}
