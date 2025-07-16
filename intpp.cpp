#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_map>
#include <string>
#include <math.h>

bool is_marker_at(const std::vector<uint8_t>& code, size_t index, const std::vector<uint8_t>& marker) {
    if (index + marker.size() > code.size()) return false;
    for (size_t j = 0; j < marker.size(); ++j) {
        if (code[index + j] != marker[j]) return false;
    }
    return true;
}

std::vector<uint8_t> read_until_marker(const std::vector<uint8_t>& code, size_t& index, const std::vector<uint8_t>& marker = {'Z', 'F', 0x02}) {
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
    const std::vector<uint8_t> xfmark = {'X', 'F', 0x02};
    const std::vector<uint8_t> zfmark = {'Z', 'F', 0x02};
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

int main() {
    std::vector<uint8_t> code = {
        0x10,
        't','e','s','t','Z','F',0x02,
        // content
        0x01, 0x02, 'h', 'e', 'l', 'l', 'o',
        'Z','F',0x02,
        'X','F',0x02,
        0x11, 't','e','s','t',
        'Z','F',0x02,

        0x01,0x02,'t','Z','F',0x02
    };
    execute(code);
    return 0;
}
