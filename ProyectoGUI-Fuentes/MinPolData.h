#ifndef MINPOL_DATA_H
#define MINPOL_DATA_H

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace std;

struct MinPolData {
    int n = 0;
    int m = 0;
    vector<int> p;
    vector<double> v;
    vector<double> ce;
    vector<vector<double>> c;
    double ct = 0.0;
    int maxM = 0;
    bool valid = false;
};

class MinPolParser {
private:
    static vector<double> parseDoubleList(string s) {
        for (char& ch : s) if (ch == ',') ch = ' ';
        stringstream ss(s);
        vector<double> res;
        double val;
        while (ss >> val) {
            res.push_back(val);
        }
        return res;
    }

    static vector<int> parseIntList(string s) {
        for (char& ch : s) if (ch == ',') ch = ' ';
        stringstream ss(s);
        vector<int> res;
        int val;
        while (ss >> val) {
            res.push_back(val);
        }
        return res;
    }

    static size_t findVariable(const string& content, const string& varName) {
        size_t pos = 0;
        while (true) {
            pos = content.find(varName, pos);
            if (pos == string::npos) return string::npos;
            
            bool prevOk = (pos == 0 || isspace(content[pos - 1]) || content[pos - 1] == ';');
            bool nextOk = (pos + varName.length() == content.length() || 
                           isspace(content[pos + varName.length()]) || 
                           content[pos + varName.length()] == '=');
            
            if (prevOk && nextOk) {
                size_t eqPos = content.find('=', pos + varName.length());
                if (eqPos != string::npos) {
                    return eqPos + 1;
                }
            }
            pos += varName.length();
        }
    }

    static string getVarValueString(const string& content, const string& varName) {
        size_t startPos = findVariable(content, varName);
        if (startPos == string::npos) return "";
        size_t endPos = content.find(';', startPos);
        if (endPos == string::npos) return "";
        return content.substr(startPos, endPos - startPos);
    }

    static double parseSimpleDouble(const string& s) {
        stringstream ss(s);
        double val = 0.0;
        ss >> val;
        return val;
    }

    static int parseSimpleInt(const string& s) {
        stringstream ss(s);
        int val = 0;
        ss >> val;
        return val;
    }

    static vector<double> parseDoubleArray(const string& s) {
        vector<double> res;
        size_t openB = s.find('[');
        size_t closeB = s.find(']');
        if (openB == string::npos || closeB == string::npos || closeB < openB) return res;
        return parseDoubleList(s.substr(openB + 1, closeB - openB - 1));
    }

    static vector<int> parseIntArray(const string& s) {
        vector<int> res;
        size_t openB = s.find('[');
        size_t closeB = s.find(']');
        if (openB == string::npos || closeB == string::npos || closeB < openB) return res;
        return parseIntList(s.substr(openB + 1, closeB - openB - 1));
    }

    static vector<vector<double>> parse2DDoubleArray(const string& s) {
        vector<vector<double>> res;
        size_t openB = s.find('[');
        size_t closeB = s.find(']');
        if (openB == string::npos || closeB == string::npos || closeB < openB) return res;
        string inner = s.substr(openB + 1, closeB - openB - 1);
        
        size_t pos = 0;
        while (true) {
            size_t nextPipe = inner.find('|', pos);
            if (nextPipe == string::npos) {
                string rowStr = inner.substr(pos);
                vector<double> row = parseDoubleList(rowStr);
                if (!row.empty()) res.push_back(row);
                break;
            }
            string rowStr = inner.substr(pos, nextPipe - pos);
            vector<double> row = parseDoubleList(rowStr);
            if (!row.empty()) res.push_back(row);
            pos = nextPipe + 1;
        }
        return res;
    }

public:
    static bool parseDZN(const string& rawContent, MinPolData& data) {
        // Remove comments
        string content;
        stringstream ss(rawContent);
        string line;
        while (getline(ss, line)) {
            size_t commentPos = line.find('%');
            if (commentPos != string::npos) {
                line = line.substr(0, commentPos);
            }
            content += line + " ";
        }

        string val_n = getVarValueString(content, "n");
        string val_m = getVarValueString(content, "m");
        string val_p = getVarValueString(content, "p");
        string val_v = getVarValueString(content, "v");
        string val_ce = getVarValueString(content, "ce");
        string val_c = getVarValueString(content, "c");
        string val_ct = getVarValueString(content, "ct");
        string val_maxM = getVarValueString(content, "maxM");

        if (val_n.empty() || val_m.empty()) return false;

        data.n = parseSimpleInt(val_n);
        data.m = parseSimpleInt(val_m);
        data.p = parseIntArray(val_p);
        data.v = parseDoubleArray(val_v);
        data.ce = parseDoubleArray(val_ce);
        data.c = parse2DDoubleArray(val_c);
        data.ct = parseSimpleDouble(val_ct);
        data.maxM = parseSimpleInt(val_maxM);

        data.valid = (data.n > 0 && data.m > 0 && 
                      (int)data.p.size() == data.m && 
                      (int)data.v.size() == data.m && 
                      (int)data.ce.size() == data.m && 
                      (int)data.c.size() == data.m);
        return data.valid;
    }

    static bool parseMPL(const string& rawContent, MinPolData& data) {
        stringstream ss(rawContent);
        string line;
        
        auto getNextLine = [&](string& l) -> bool {
            while (getline(ss, l)) {
                while (!l.empty() && isspace((unsigned char)l.front())) l.erase(l.begin());
                while (!l.empty() && isspace((unsigned char)l.back())) l.pop_back();
                if (l.empty() || l[0] == '%') continue;
                return true;
            }
            return false;
        };

        if (!getNextLine(line)) return false;
        data.n = stoi(line);

        if (!getNextLine(line)) return false;
        data.m = stoi(line);

        if (!getNextLine(line)) return false;
        data.p = parseIntList(line);

        if (!getNextLine(line)) return false;
        data.v = parseDoubleList(line);

        if (!getNextLine(line)) return false;
        data.ce = parseDoubleList(line);

        data.c.clear();
        for (int i = 0; i < data.m; i++) {
            if (!getNextLine(line)) return false;
            vector<double> row = parseDoubleList(line);
            data.c.push_back(row);
        }

        if (!getNextLine(line)) return false;
        data.ct = stod(line);

        if (!getNextLine(line)) return false;
        data.maxM = stoi(line);

        data.valid = (data.n > 0 && data.m > 0 && 
                      (int)data.p.size() == data.m && 
                      (int)data.v.size() == data.m && 
                      (int)data.ce.size() == data.m && 
                      (int)data.c.size() == data.m);
        return data.valid;
    }

    static string toDZN(const MinPolData& data) {
        stringstream ss;
        ss << "n = " << data.n << ";\n";
        ss << "m = " << data.m << ";\n";
        
        ss << "p = [";
        for (size_t i = 0; i < data.p.size(); i++) {
            ss << data.p[i] << (i + 1 < data.p.size() ? "," : "");
        }
        ss << "];\n";

        ss << "v = [";
        for (size_t i = 0; i < data.v.size(); i++) {
            ss << data.v[i] << (i + 1 < data.v.size() ? "," : "");
        }
        ss << "];\n";

        ss << "ce = [";
        for (size_t i = 0; i < data.ce.size(); i++) {
            ss << data.ce[i] << (i + 1 < data.ce.size() ? "," : "");
        }
        ss << "];\n";

        ss << "c = [|";
        for (size_t r = 0; r < data.c.size(); r++) {
            for (size_t col = 0; col < data.c[r].size(); col++) {
                ss << data.c[r][col] << (col + 1 < data.c[r].size() ? "," : "");
            }
            ss << (r + 1 < data.c.size() ? "|\n" : "|];\n");
        }

        ss << "ct = " << data.ct << ";\n";
        ss << "maxM = " << data.maxM << ";\n";

        return ss.str();
    }
};

#endif // MINPOL_DATA_H
