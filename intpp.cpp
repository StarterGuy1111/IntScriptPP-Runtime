#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>
#include <math.h>
#include <cstring>

bool is_marker_at(const std::vector<uint8_t>& code, size_t index, const std::vector<uint8_t>& marker) {
    if (index + marker.size() > code.size()) return false;
    for (size_t j = 0; j < marker.size(); ++j) {
        if (code[index + j] != marker[j]) return false;
    }
    return true;
}

std::vector<uint8_t> read_until_marker(const std::vector<uint8_t>& code, size_t& index, const std::vector<uint8_t>& marker = {0x02,'Z', 'F'}) {
    std::vector<uint8_t> result;

    // Helper to check if marker is at index
    auto is_marker_at = [&](size_t pos) {
        if (pos + marker.size() > code.size()) return false;
        for (size_t j = 0; j < marker.size(); ++j) {
            if (code[pos + j] != marker[j]) return false;
        }
        return true;
    };

    while (index < code.size() && !is_marker_at(index)) {
        result.push_back(code[index++]);
    }
    if (is_marker_at(index)) {
        index += marker.size(); // skip full marker
    }
    return result;
}


void mkerror(std::string msg, std::string errorcd, std::string file="nofile", size_t i = 0) {
    std::cout << "\nCode Tradeback:\n";
    if (file != "nofile") {
        std::cout << "  File '"+file+"', line "+std::to_string(i)+", in file\n";
        std::cout << "    "+errorcd+": "+msg;
    } else {
        std::cout << "  File [unknown], "+std::to_string(i)+", in file\n";
        std::cout << "    "+errorcd+": "+msg;
    }
    std::exit(EXIT_FAILURE);
}

bool is_xfmark_at(const std::vector<uint8_t>& code, size_t index, const std::vector<uint8_t>& marker) {
    if (index + marker.size() > code.size()) return false;
    for (size_t j = 0; j < marker.size(); ++j) {
        if (code[index + j] != marker[j]) return false;
    }
    return true;
}


void execute(std::vector<uint8_t> code) {
    const std::vector<uint8_t> xfmark = {0x02,'X', 'F'};
    const std::vector<uint8_t> zfmark = {0x02,'Z', 'F'};
    std::unordered_map<std::string, std::string> variables;
    std::unordered_map<std::string, std::vector<uint8_t>> functions;
    size_t i = 0;

    while (i < code.size()) {
        uint8_t opcode = code[i++];
        switch (opcode) {
            case 0x01:
            {
                uint8_t dp = code[i++];  

                if (dp== 0x01) {
                    // int
                    if (i >= code.size()) break;  
                    std::cout << static_cast<int>(code[i++]) << std::endl;
                } else if (dp== 0x02) {
                    // str
                    std::vector<uint8_t> toprint = read_until_marker(code, i);
                    std::string str(toprint.begin(), toprint.end());
                    std::cout << str << std::endl;
                } else if (dp == 0x03) {
                    std::vector<uint8_t> varusen = read_until_marker(code, i);
                    std::string str(varusen.begin(), varusen.end());

                    if (variables.find(str) == variables.end()) {
                        mkerror("undefined variable '" + str + "'", "NameError", "nofile", i);
                    }

                    std::cout << variables[str] << std::endl;
                }
                break;
            }
            case 0x02:
            {
                std::vector<uint8_t> varname = read_until_marker(code, i);
                std::string strvar(varname.begin(), varname.end());
                
                
                if (code[i++] == 0x01) {
                    // int value stored as string
                    int val = static_cast<int>(code[i++]);
                    variables[strvar] = std::to_string(val);
                } else if (code[i] == 0x02) {
                    // string value
                    std::vector<uint8_t> varval = read_until_marker(code, i);
                    std::string varvalue(varval.begin(), varval.end());
                    variables[strvar] = varvalue;
                }
                break;
            }
            case 0x10:
                {
                    std::vector<uint8_t> fname_bytes = read_until_marker(code, i);
                    std::string fname(fname_bytes.begin(), fname_bytes.end());

                    std::vector<uint8_t> body;
                    while (i < code.size() && !is_xfmark_at(code, i, xfmark)) {  // check full marker sequence
                        body.push_back(code[i++]);
                    }
                    if (is_xfmark_at(code, i, xfmark)) i += xfmark.size();  // skip full marker

                    functions[fname] = body;
                    break;
                }
                
            case 0x11:
                {
                    std::vector<uint8_t> fname = read_until_marker(code, i);
                    std::string fn(fname.begin(), fname.end());

                    if (functions.find(fn) != functions.end()) {
                        execute(functions[fn]); // yes... recursive execution
                    } else {
                        mkerror("Unknown function instance: "+fn, "NameError", "nofile", i);
                    }
                    break;
                }
            

            // alu warm up

            case 0x20:
                {
                    // add
                    int a = code[i++];
                    int b = code[i++];

                    int res = a+b;
                    
                    std::vector<uint8_t> outputvar = read_until_marker(code, i);
                    std::string output(outputvar.begin(), outputvar.end());

                    if (variables.find(output) != variables.end()) {
                        variables[output] = std::to_string(res);
                    }
                    break; // give me a break ._.
                }
            case 0x21:
                {
                    // sub
                    int a = code[i++];
                    int b = code[i++];

                    int res;
                    if (a>=b) {
                        res = a-b;
                    } else {
                        res = b-a;
                    }
                    
                    std::vector<uint8_t> outputvar = read_until_marker(code, i);
                    std::string output(outputvar.begin(), outputvar.end());

                    if (variables.find(output) != variables.end()) {
                        variables[output] = std::to_string(res);
                    }
                    break; // give me a break ._.
                }
            case 0x22:
                {
                    // mul
                    int a = code[i++];
                    int b = code[i++];

                    int res = a*b;
                    
                    std::vector<uint8_t> outputvar = read_until_marker(code, i);
                    std::string output(outputvar.begin(), outputvar.end());

                    if (variables.find(output) != variables.end()) {
                        variables[output] = std::to_string(res);
                    }
                    break; // give me a break ._.
                }
            case 0x23:
                {
                    // div
                    int a = code[i++];
                    int b = code[i++];

                    int res = (int) round(a/b);
                    
                    std::vector<uint8_t> outputvar = read_until_marker(code, i);
                    std::string output(outputvar.begin(), outputvar.end());

                    if (variables.find(output) != variables.end()) {
                        variables[output] = std::to_string(res);
                    }
                    break; // give me a break ._.
                }
            // yes alu is copy pasted from add
        }
    }
}

int main(int argc, char *argv[]) {
    const std::vector<uint8_t> magic = {0x69, 0x78, 0x25, 0x05, 0x05};

    if (argc == 2) {
        std::ifstream fin(argv[1], std::ios::binary);

        if (!fin) {
            std::cerr << "err reading file";
            return -1;
        }

        std::vector<char> TEMP((std::istreambuf_iterator<char>(fin)),
                         std::istreambuf_iterator<char>());
        
        std::vector<uint8_t> code(TEMP.begin(), TEMP.end());

        fin.close();
        int magiccheck = std::memcmp(TEMP.data(), magic.data(), 5);
        if (magiccheck==0) {
            execute(code);
        } else {
            std::cerr << "sorry but as of build-1-0-4, now have to follow magic bytes:\n0x69, 0x78, 0x25, 0x05, 0x05";
        }
        return 0;
        
    } else {
        std::cerr << "incorrect argc count. correct usage: build.exe <filename.ix>";
        return -1;
    }
}
