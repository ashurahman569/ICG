#include <iostream>
#include <fstream>
#include "antlr4-runtime.h"
#include "CSubsetLexer.h"
#include "CSubsetParser.h"
#include "Visitor.h"
using namespace antlr4;
using namespace std;
using namespace tree;

ofstream lexLogFile;

string trim(const string& s) {
    int start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    int end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool isLabel(const string& line) {
    string t = trim(line);
    return !t.empty() && t.back() == ':' && t.find(' ') == string::npos && t.find('\t') == string::npos;
}

void optimizeAssembly(const string& inputFile, const string& outputFile) {
    ifstream in(inputFile);
    ofstream out(outputFile);
    if (!in.is_open() || !out.is_open()) return;

    vector<string> lines;
    string line;
    while (getline(in, line)) {
        lines.push_back(line);
    }
    in.close();

    bool changed = true;
    while (changed) {
        changed = false;
        vector<string> newLines;

        for (int i = 0; i < lines.size(); ++i) {
            string current = trim(lines[i]);

            if (current.empty()) {
                newLines.push_back(lines[i]);
                continue;
            }

            if (current.find("ADD ") == 0 || current.find("SUB ") == 0) {
                if (current.rfind(", 0") == current.length() - 3) {
                    changed = true;
                    continue; 
                }
            }
            if (current.find("MUL ") == 0 || current.find("IMUL ") == 0) {
                if (current.rfind(", 1") == current.length() - 3) {
                    changed = true;
                    continue; 
                }
            }

            if (i + 1 < lines.size()) {
                string next = trim(lines[i + 1]);

                if (current.find("PUSH ", 0) == 0 && next.find("POP ", 0) == 0) {
                    string pushVal = trim(current.substr(5));
                    string popVal = trim(next.substr(4));
                    if (pushVal == popVal) {
                        changed = true;
                        i++;
                        continue;
                    }
                }

                if (current.find("MOV ", 0) == 0 && next.find("MOV ", 0) == 0) {
                    string cArgs = trim(current.substr(4));
                    string nArgs = trim(next.substr(4));
                    int cComma = cArgs.find(',');
                    int nComma = nArgs.find(',');

                    if (cComma != string::npos && nComma != string::npos) {
                        string a1 = trim(cArgs.substr(0, cComma));
                        string a2 = trim(cArgs.substr(cComma + 1));
                        string b1 = trim(nArgs.substr(0, nComma));
                        string b2 = trim(nArgs.substr(nComma + 1));

                        if (a1 == b2 && a2 == b1) {
                            newLines.push_back(lines[i]); // Keep first mov
                            changed = true;
                            i++; 
                            continue;
                        }
                    }
                }

                if (isLabel(current) && isLabel(next)) {
                    string labelToKeep = current.substr(0, current.length() - 1);
                    string labelToRemove = next.substr(0, next.length() - 1);

                    newLines.push_back(lines[i]);
                    changed = true;
                    i++; 

                    for (auto& l : lines) {
                        int pos = 0;
                        while ((pos = l.find(labelToRemove, pos)) != string::npos) {
                            bool prevChar = (pos == 0 || !isalnum(l[pos - 1]));
                            bool nextChar = (pos + labelToRemove.length() >= l.length() || !isalnum(l[pos + labelToRemove.length()]));
                            if (prevChar && nextChar) {
                                l.replace(pos, labelToRemove.length(), labelToKeep);
                            }
                            pos += labelToKeep.length();
                        }
                    }
                    continue;
                }
            }

            newLines.push_back(lines[i]);
        }
        lines.clear();
        lines = newLines;
    }

    for (const auto& l : lines) {
        out << l << "\n";
    }
    out.close();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile) {
        cerr << "Error opening input file " << argv[1] << endl;
        return 1;
    }

    ANTLRInputStream input(inputFile);
    CSubsetLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    CSubsetParser parser(&tokens);
    parser.removeErrorListeners(); 
    ParseTree *tree = parser.start();

    Visitor visitor;
    
    visitor.visit(tree);

    ifstream asmFile("code.asm");
    if (!asmFile) {
        cerr << "Error opening output file code.asm" << endl;
        return 1;
    }
    asmFile.close();
    optimizeAssembly("code.asm", "optimized_code.asm");

    return 0;
}